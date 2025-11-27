#ifndef DF8FF9A8_57E9_4DA8_AE65_9BA987CAF2E0
#define DF8FF9A8_57E9_4DA8_AE65_9BA987CAF2E0

/**
 * @file multipart_stream_middleware.h
 * @brief HTTP multipart streaming middleware for ESP-IDF web server
 *
 * This file provides a middleware for streaming multipart HTTP responses,
 * typically used for MJPEG video streams or other continuous data streams.
 * Uses FreeRTOS queues to decouple data production from HTTP transmission.
 */

#include "../web_server.h"
#include <expected>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <functional>
#include <memory>
#include <string>

namespace cjf
{

  class multipart_stream;

  /**
   * @brief Middleware for HTTP multipart streaming responses
   *
   * Implements a broadcast pattern for streaming multipart HTTP responses to multiple clients.
   * Data producers call `write()` to broadcast data parts to all connected clients,
   * which are transmitted as multipart/x-mixed-replace responses.
   *
   * Thread Safety:
   * - `write()` and `end()` are thread-safe and can be called from multiple threads
   * - Multiple HTTP clients can connect simultaneously and all receive the same frames
   * - Each client has its own queue to prevent blocking between clients
   *
   * Typical Usage:
   * 1. Create middleware with `multipart_stream(config)`
   * 2. Register middleware with web server using `.use()`
   * 3. Call `write()` from producer thread(s) to broadcast data parts to all clients
   * 4. Call `end()` to signal end of stream to all clients
   *
   * Example:
   * ```c++
   * auto stream = multipart_stream({
   *     .boundary = "frame",
   *     .part_content_type = "image/jpeg",
   *     .queue_depth = 3
   * });
   *
   * // Register with web server
   * server_builder.use("/stream", stream);
   *
   * // From producer thread - broadcasts to all connected clients
   * stream.write(image_data, image_size);
   * stream.end();
   * ```
   */
  class multipart_stream : public web_server::middleware
  {
  public:
    /// Middleware name for logging
    static constexpr const char *name = "multipart_stream";

    /// Callback function type invoked when streaming starts
    using stream_start_func = std::function<void()>;

    /// Callback function type invoked when streaming ends
    using stream_end_func = std::function<void()>;

    /**
     * @brief Configuration for multipart stream middleware
     */
    struct config_type
    {
      /// Multipart boundary string (e.g., "frame")
      const char *boundary;

      /// Content-Type for each part (e.g., "image/jpeg")
      const char *part_content_type;

      /// Optional callback invoked when HTTP stream starts
      stream_start_func on_stream_start = NULL;

      /// Optional callback invoked when HTTP stream ends
      stream_end_func on_stream_end = NULL;

      /// Depth of the FreeRTOS queue for buffering parts (default: 3)
      size_t queue_depth = 3;
    };

    /**
     * @brief Create a multipart stream middleware instance
     *
     * Allocates a FreeRTOS queue and initializes the middleware with the given configuration.
     *
     * @param config Configuration parameters for the stream
     * @return Expected containing the middleware instance if successful, or an error code
     * @retval ESP_ERR_NO_MEM Failed to allocate queue memory
     */
    // static std::expected<multipart_stream, esp_err_t> create(const config_type &config);

    /**
     *
     */
    multipart_stream(const config_type &config);

    /**
     * @brief Signal end of stream
     *
     * Enqueues a sentinel value to signal the HTTP handler to close the stream.
     * This should only be called once per stream.
     *
     * Thread Safety: Safe to call from any thread, but should only be called once.
     *
     * @param ticks_to_wait Maximum time to wait if queue is full (default: wait forever)
     * @return ESP_OK on success
     * @retval ESP_ERR_TIMEOUT Queue full and timeout expired
     */
    esp_err_t end(TickType_t ticks_to_wait = portMAX_DELAY);

    /**
     * @brief Write a data part to the stream
     *
     * Broadcasts a data part to all connected clients.
     * The data is wrapped in a shared_ptr to manage lifetime across threads.
     *
     * Thread Safety: Safe to call from multiple threads. If multiple producers call
     * write() concurrently, part ordering in the stream is non-deterministic.
     *
     * @param data Shared pointer to the data buffer (must not be null)
     * @param size Size of the data in bytes
     * @param ticks_to_wait Maximum time to wait if any client queue is full (default: no wait)
     * @return ESP_OK on success
     * @retval ESP_ERR_INVALID_ARG Data pointer is null
     * @retval ESP_ERR_TIMEOUT One or more client queues are full
     */
    esp_err_t write(std::shared_ptr<const void> data, const size_t size, TickType_t ticks_to_wait = 0);

  private:
    /**
     * @brief Internal context shared between producer and consumers
     */
    struct context
    {
      /// Mutex protecting the subscribers list
      SemaphoreHandle_t mutex;

      /// List of subscriber queues (one per connected client)
      std::vector<QueueHandle_t> subscribers;

      /// Queue depth for each subscriber
      const size_t queue_depth;

      /// Formatted multipart boundary string with CRLF
      const std::string part_boundary;

      /// Formatted Content-Type header for each part
      const std::string part_content_type;

      /// Formatted Content-Type header for the stream response
      const std::string stream_content_type;

      /// Optional callback invoked when streaming starts
      const stream_start_func on_stream_start;

      /// Optional callback invoked when streaming ends
      const stream_end_func on_stream_end;

      /**
       * @brief Custom deleter for context cleanup
       * @param ctx Context to delete
       */
      static void deleter(context *ctx);
    };

    /**
     * @brief Data structure for a single part in the stream
     */
    struct part_data
    {
      /// Shared pointer to the data buffer (nullptr signals end-of-stream)
      std::shared_ptr<const void> buffer;

      /// Size of the data in bytes (0 signals end-of-stream)
      size_t size;
    };

    /// Shared context containing queue and configuration
    std::shared_ptr<context> _ctx;

    /**
     * @brief Private constructor
     * @param ctx Shared context
     */
    multipart_stream(std::shared_ptr<context> ctx);

    /**
     * @brief HTTP handler that sends parts from the queue
     * @param req HTTP request handle
     * @param context Shared context containing the queue
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t send_part(httpd_req_t *req, std::shared_ptr<context> context);
  };

} // namespace cjf

#endif /* DF8FF9A8_57E9_4DA8_AE65_9BA987CAF2E0 */
