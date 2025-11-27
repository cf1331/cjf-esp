#include "cjf/web_server/multipart_stream_middleware.h"
#include <cjf/error_handling.h>
#include <cjf/timeout.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <string>
#include <vector>

namespace cjf
{

  const char *MULTIPART_STREAM_MIDDLEWARE = "middleware:multipart_stream";

  constexpr std::string part_boundary(const char *boundary)
  {
    return std::string("\r\n--") + boundary + "\r\n";
  }

  constexpr std::string part_content_type(const char *content_type)
  {
    return std::string("Content-Type: ") + content_type + "\r\n";
  }

  constexpr std::string stream_content_type(const char *boundary)
  {
    return std::string("multipart/x-mixed-replace;boundary=") + boundary;
  }

  void multipart_stream::context::deleter(context *ctx)
  {
    if (ctx)
    {
      // Clean up all subscriber queues
      if (ctx->mutex && xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE)
      {
        for (auto queue : ctx->subscribers)
        {
          if (queue)
          {
            // Drain the queue first
            part_data *pd = nullptr;
            while (xQueueReceive(queue, &pd, 0) == pdTRUE)
            {
              if (pd)
              {
                delete pd;
              }
            }
            vQueueDelete(queue);
          }
        }
        xSemaphoreGive(ctx->mutex);
      }
      if (ctx->mutex)
      {
        vSemaphoreDelete(ctx->mutex);
      }
      delete ctx;
    }
  }

  multipart_stream::multipart_stream(const config_type &config)
      : multipart_stream(std::shared_ptr<context>(
            new context{
                .mutex = xSemaphoreCreateMutex(),
                .subscribers = {},
                .queue_depth = config.queue_depth,
                .part_boundary = part_boundary(config.boundary),
                .part_content_type = part_content_type(config.part_content_type),
                .stream_content_type = stream_content_type(config.boundary),
                .on_stream_start = config.on_stream_start,
                .on_stream_end = config.on_stream_end},
            context::deleter)) {}

  multipart_stream::multipart_stream(std::shared_ptr<context> ctx)
      : _ctx(ctx)
  {
    // Capture ctx in the middleware handler lambda
    handler = [ctx](httpd_req_t *req, web_server::next_func next)
    {
      return send_part(req, ctx);
    };
    // This middleware needs async handling to allow other requests while streaming
    is_async = true;
  }

  esp_err_t multipart_stream::write(std::shared_ptr<const void> data, const size_t size, TickType_t ticks_to_wait)
  {
    if (!data)
    {
      ESP_RETURN_ON_ERROR(ESP_ERR_INVALID_ARG, MULTIPART_STREAM_MIDDLEWARE, "Data pointer is null");
    }

    if (!_ctx || !_ctx->mutex)
    {
      ESP_RETURN_ON_ERROR(ESP_ERR_INVALID_STATE, MULTIPART_STREAM_MIDDLEWARE, "Invalid context");
    }

    // Broadcast to all subscribers
    if (xSemaphoreTake(_ctx->mutex, portMAX_DELAY) != pdTRUE)
    {
      ESP_RETURN_ON_ERROR(ESP_ERR_TIMEOUT, MULTIPART_STREAM_MIDDLEWARE, "Failed to acquire mutex");
    }

    esp_err_t result = ESP_OK;
    size_t sent_count = 0;
    size_t failed_count = 0;

    for (auto queue : _ctx->subscribers)
    {
      // Dynamically allocate part_data for each subscriber
      part_data *pd = new part_data{data, size};

      if (xQueueSend(queue, &pd, ticks_to_wait) != pdTRUE)
      {
        // Failed to send, clean up
        delete pd;
        failed_count++;
        result = ESP_ERR_TIMEOUT;
      }
      else
      {
        sent_count++;
      }
    }

    xSemaphoreGive(_ctx->mutex);

    if (failed_count > 0)
    {
      ESP_LOGW(MULTIPART_STREAM_MIDDLEWARE, "Broadcast: sent to %zu/%zu subscribers (%zu dropped)",
               sent_count, sent_count + failed_count, failed_count);
    }

    return result;
  }

  esp_err_t multipart_stream::end(TickType_t ticks_to_wait)
  {
    if (!_ctx || !_ctx->mutex)
    {
      ESP_RETURN_ON_ERROR(ESP_ERR_INVALID_STATE, MULTIPART_STREAM_MIDDLEWARE, "Invalid context");
    }

    // Broadcast end signal to all subscribers
    if (xSemaphoreTake(_ctx->mutex, portMAX_DELAY) != pdTRUE)
    {
      ESP_RETURN_ON_ERROR(ESP_ERR_TIMEOUT, MULTIPART_STREAM_MIDDLEWARE, "Failed to acquire mutex");
    }

    for (auto queue : _ctx->subscribers)
    {
      part_data *pd = nullptr;
      xQueueSend(queue, &pd, ticks_to_wait);
    }

    xSemaphoreGive(_ctx->mutex);
    return ESP_OK;
  }

  esp_err_t multipart_stream::send_part(httpd_req_t *req, std::shared_ptr<context> ctx)
  {
    esp_err_t res = ESP_OK;

    if (!ctx || !ctx->mutex)
    {
      ESP_RETURN_ON_ERROR(ESP_ERR_INVALID_STATE, MULTIPART_STREAM_MIDDLEWARE, "Missing context or mutex");
    }

    // Create a queue for this client
    QueueHandle_t client_queue = xQueueCreate(ctx->queue_depth, sizeof(part_data *));
    if (!client_queue)
    {
      ESP_RETURN_ON_ERROR(ESP_ERR_NO_MEM, MULTIPART_STREAM_MIDDLEWARE, "Failed to create client queue");
    }

    // Subscribe this client to the broadcast
    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) != pdTRUE)
    {
      vQueueDelete(client_queue);
      ESP_RETURN_ON_ERROR(ESP_ERR_TIMEOUT, MULTIPART_STREAM_MIDDLEWARE, "Failed to acquire mutex");
    }

    ctx->subscribers.push_back(client_queue);
    size_t client_id = ctx->subscribers.size();

    xSemaphoreGive(ctx->mutex);

    ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "Client %zu connected (%zu total)", client_id, client_id);

    if (ctx->on_stream_start)
    {
      ctx->on_stream_start();
    }

    // Note the start time so we can calculate the frame rate
    int64_t send_time = esp_timer_get_time();

    const char *stream_content_type = ctx->stream_content_type.c_str();
    ESP_LOGD(MULTIPART_STREAM_MIDDLEWARE, "Setting content type: %s", stream_content_type);
    res = httpd_resp_set_type(req, stream_content_type);
    if (res != ESP_OK)
    {
      ESP_LOGE(MULTIPART_STREAM_MIDDLEWARE, "Failed to set content type");
      goto cleanup;
    }

    while (true)
    {
      part_data *part_ptr = nullptr;
      ESP_LOGD(MULTIPART_STREAM_MIDDLEWARE, "Client %zu: Waiting for part", client_id);
      if (xQueueReceive(client_queue, &part_ptr, portMAX_DELAY) != pdTRUE)
      {
        ESP_LOGD(MULTIPART_STREAM_MIDDLEWARE, "Client %zu: Part receive timeout", client_id);
        continue;
      }

      // Check for nullptr sentinel value (end of stream signal)
      if (!part_ptr)
      {
        ESP_LOGD(MULTIPART_STREAM_MIDDLEWARE, "Client %zu: End of stream signal received", client_id);
        break;
      }

      // Use unique_ptr for RAII cleanup of dynamically allocated part_data
      std::unique_ptr<part_data> part(part_ptr);

      ESP_LOGD(MULTIPART_STREAM_MIDDLEWARE, "Client %zu: Sending part boundary", client_id);
      res = httpd_resp_send_chunk(req, ctx->part_boundary.c_str(), ctx->part_boundary.size());
      BREAK_ON_ERROR(res, MULTIPART_STREAM_MIDDLEWARE, "Failed to send boundary");

      ESP_LOGD(MULTIPART_STREAM_MIDDLEWARE, "Client %zu: Sending part headers", client_id);
      std::string part_headers =
          ctx->part_content_type +
          "Content-Length: " + std::to_string(part->size) + "\r\n\r\n";
      res = httpd_resp_send_chunk(req, part_headers.c_str(), part_headers.size());
      BREAK_ON_ERROR(res, MULTIPART_STREAM_MIDDLEWARE, "Failed to send part headers");

      ESP_LOGD(MULTIPART_STREAM_MIDDLEWARE, "Client %zu: Sending part", client_id);
      res = httpd_resp_send_chunk(req, reinterpret_cast<const char *>(part->buffer.get()), part->size);
      BREAK_ON_ERROR(res, MULTIPART_STREAM_MIDDLEWARE, "Failed to send part data");

      int64_t now = esp_timer_get_time();
      uint32_t part_time_ms = (now - send_time) / 1000;
      float fps = 1000.0 / part_time_ms;
      uint32_t part_size_kb = part->size / 1024;
      send_time = now;
      ESP_LOGD(MULTIPART_STREAM_MIDDLEWARE, "Client %zu: Part: %luKB %lums (%.1f part/s)",
               client_id, part_size_kb, part_time_ms, fps);
    }

cleanup:
    // Unsubscribe this client
    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE)
    {
      auto it = std::find(ctx->subscribers.begin(), ctx->subscribers.end(), client_queue);
      if (it != ctx->subscribers.end())
      {
        ctx->subscribers.erase(it);
      }
      xSemaphoreGive(ctx->mutex);
    }

    // Clean up the client queue
    part_data *part_ptr = nullptr;
    while (xQueueReceive(client_queue, &part_ptr, 0) == pdTRUE)
    {
      if (part_ptr)
      {
        delete part_ptr;
      }
    }
    vQueueDelete(client_queue);

    ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "Client %zu disconnected", client_id);

    // Send final empty chunk to close the response stream
    res = httpd_resp_send_chunk(req, nullptr, 0);
    LOG_IF_ERROR(res, MULTIPART_STREAM_MIDDLEWARE, "Failed to send final chunk");

    if (ctx->on_stream_end)
    {
      ctx->on_stream_end();
    }

    ESP_LOGD(MULTIPART_STREAM_MIDDLEWARE, "Client %zu: End of stream", client_id);
    return res;
  }

} // namespace cjf
