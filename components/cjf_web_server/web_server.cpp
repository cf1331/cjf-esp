#include "cjf/web_server.h"
#include <cjf/error_handling.h>
#include <cjf/memory.h>
#include <esp_netif.h>
#include <esp_log.h>

namespace cjf
{
  static const char *TAG = "cjf:web_server";
  static const char *ANONYMOUS_MIDDLEWARE_NAME = "anonymous";

  web_server::builder::builder(const httpd_config_t &config) : config_(config)
  {
    // Use a custom match function that matches all URIs so that all requests are
    // handled by the web servers internal request handler. The internal request
    // handler takes care of route matching and running the middleware chain.
    config_.uri_match_fn = uri_match_any;
  }

  web_server::builder &web_server::builder::use(middleware m)
  {
    use("/*", m);
    return *this;
  }

  web_server::builder &web_server::builder::use(handler_func handler)
  {
    use("/*", {nullptr, handler});
    return *this;
  }

  web_server::builder &web_server::builder::use(const char *path, middleware m)
  {
    const char *name = (m.name)
      ? m.name
      : ANONYMOUS_MIDDLEWARE_NAME;
    ESP_LOGI(TAG, "Using %s middleware for %s", name, path);
    routes_.push_back({path, m});
    return *this;
  }

  web_server::builder &web_server::builder::use(const char *path, handler_func handler)
  {
    use(path, {nullptr, handler});
    return *this;
  }

  web_server::builder &web_server::builder::set_async_worker_count(size_t count)
  {
    async_worker_count_ = count;
    return *this;
  }

  web_server::builder &web_server::builder::set_async_queue_depth(size_t depth)
  {
    async_queue_depth_ = depth;
    return *this;
  }

  std::expected<std::unique_ptr<web_server>, esp_err_t> web_server::builder::listen()
  {
    httpd_handle_t handle_raw = nullptr;
    esp_err_t err = httpd_start(&handle_raw, &config_);
    RETURN_UNEXPECTED_ON_ERROR(err, TAG);
    std::unique_ptr<void, deleter> handle(handle_raw);

    // Create task pool for async middleware handling
    auto pool = task_pool<web_server::async_req>::create({
        .worker_count = async_worker_count_,
        .queue_depth = async_queue_depth_,
        .stack_size = 4096,
        .priority = 5,
        .task_name = "web_async",
        .handler = [](web_server::async_req &req)
        {
          ESP_LOGD(TAG, "Processing async middleware");
          // Reconstruct the next function from the context
          std::function<esp_err_t()> next = [ctx = req.ctx]() -> esp_err_t {
            return ctx->next();
          };
          esp_err_t result = req.handler(req.req, next);
          httpd_req_async_handler_complete(req.req);
          if (result != ESP_OK)
          {
            ESP_LOGE(TAG, "Async middleware failed: %s", esp_err_to_name(result));
          }
        }});
    RETURN_ON_UNEXPECTED(pool, TAG);

    auto server = std::unique_ptr<web_server>(new web_server(std::move(handle), config_.server_port, routes_, std::move(*pool)));
    // Register handlers for common HTTP methods
    RETURN_UNEXPECTED_ON_ERROR(server->register_handler_for_method(HTTP_GET), TAG);
    RETURN_UNEXPECTED_ON_ERROR(server->register_handler_for_method(HTTP_POST), TAG);
    RETURN_UNEXPECTED_ON_ERROR(server->register_handler_for_method(HTTP_PUT), TAG);
    RETURN_UNEXPECTED_ON_ERROR(server->register_handler_for_method(HTTP_DELETE), TAG);
    RETURN_UNEXPECTED_ON_ERROR(server->register_handler_for_method(HTTP_PATCH), TAG);
    RETURN_UNEXPECTED_ON_ERROR(server->register_handler_for_method(HTTP_HEAD), TAG);
    RETURN_UNEXPECTED_ON_ERROR(server->register_handler_for_method(HTTP_OPTIONS), TAG);
    ESP_LOGI(TAG, "Listening on port %d", config_.server_port);
    return server;
  }

  web_server::builder web_server::init(const httpd_config_t &config)
  {
    return builder(config);
  }

  void web_server::deleter::operator()(void *handle) const
  {
    esp_err_t err = httpd_stop(static_cast<httpd_handle_t>(handle));
    if (err == ESP_OK)
    {
      ESP_LOGI(TAG, "Stopped");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to stop: %s", esp_err_to_name(err));
    }
  }

  web_server::web_server(std::unique_ptr<void, deleter> handle, const uint16_t port, std::list<route> routes, task_pool<async_req> task_pool)
      : task_pool_(std::move(task_pool)), handle_(std::move(handle)), port_(port), routes_(std::move(routes)) {}

  web_server::~web_server() = default;

  esp_err_t web_server::register_handler_for_method(const httpd_method_t method)
  {
    httpd_uri_t uri = {
        .uri = "/*",
        .method = method,
        .handler = req_handler,
        .user_ctx = this,
#if CONFIG_HTTPD_WS_SUPPORT
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
#endif
    };
    return httpd_register_uri_handler(handle_.get(), &uri);
  }

  void web_server::log_endpoints()
  {
    esp_netif_t *netif = esp_netif_get_default_netif();
    if (netif == nullptr)
    {
      ESP_LOGW(TAG, "No network interface available");
      return;
    }
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK)
    {
      ESP_LOGW(TAG, "No endpoints available (no IP address)");
      return;
    }
    const std::string protocol = "http://";
    char host[16];
    esp_ip4addr_ntoa(&ip_info.ip, host, sizeof(host));
    std::string origin = protocol + host;
    if (port_ != 80)
    {
      origin += ":" + std::to_string(port_);
    }
    ESP_LOGI(TAG, "Registered endpoints:");
    for (const auto &route : routes_)
    {
      ESP_LOGI(TAG, "%s%s", origin.c_str(), route.uri.c_str());
    }
  }

  esp_err_t web_server::req_handler(httpd_req_t *req)
  {
    auto server = reinterpret_cast<web_server *>(req->user_ctx);
    auto &routes = server->routes_;
    auto &task_pool = server->task_pool_;

    ESP_LOGI(TAG, "Received %s request for %s", http_method_str(static_cast<httpd_method_t>(req->method)), req->uri);

    if (routes.empty())
    {
      return httpd_resp_send_404(req);
    }

    // Shared state for the middleware chain - owned by the next function closures
    auto ctx = std::make_shared<chain_context>();
    ctx->route = routes.begin();
    ctx->routes = &routes;
    ctx->task_pool = &task_pool;
    ctx->req = req;

    ctx->next = [ctx]() -> esp_err_t
    {
      while (ctx->route != ctx->routes->end() && !ctx->route->can_handle(ctx->req))
      {
        ctx->route++;
      }
      if (ctx->route == ctx->routes->end())
      {
        return ESP_ERR_NOT_FOUND;
      }

      const char* name = ctx->route->middleware.name
        ? ctx->route->middleware.name
        : ANONYMOUS_MIDDLEWARE_NAME;
      ESP_LOGI(TAG, "Running %s middleware for %s", name, ctx->req->uri);

      // If this is the first async middleware, submit to task pool
      if (ctx->route->middleware.is_async && !ctx->is_async)
      {
        httpd_req_t *async_req = nullptr;
        esp_err_t res = httpd_req_async_handler_begin(ctx->req, &async_req);
        if (res != ESP_OK)
        {
          ESP_LOGE(TAG, "Failed to begin async handler: %s", esp_err_to_name(res));
          return res;
        }
        ESP_LOGD(TAG, "Started async handling for %s middleware", name);

        ctx->is_async = true;
        ctx->req = async_req;  // Switch to async request

        auto current_route = ctx->route;
        ctx->route++;

        web_server::async_req work_item = {
            .req = async_req,
            .handler = current_route->middleware.handler,
            .ctx = ctx};

        if (!ctx->task_pool->submit(work_item, 0))
        {
          ESP_LOGE(TAG, "Failed to submit async work to task pool (queue full)");
          httpd_req_async_handler_complete(async_req);
          return ESP_ERR_NO_MEM;
        }

        return ESP_OK;
      }

      // Execute middleware directly
      return (ctx->route++)->middleware.handler(ctx->req, ctx->next);
    };

    esp_err_t result = ctx->next();

    switch (result)
    {
      case ESP_OK: return ESP_OK;
      case ESP_ERR_NOT_FOUND: return httpd_resp_send_404(req);
      default: return httpd_resp_send_500(req);
    }
  }

  bool web_server::uri_match_any(const char *uri_template, const char *uri_to_match, size_t match_upto)
  {
    return true;
  }

  esp_err_t send_json_response(httpd_req_t *req, cJSON *json)
  {
    unique_c_ptr<char> json_str(cJSON_PrintUnformatted(json));
    RETURN_ON_ERROR(httpd_resp_set_type(req, "application/json"), TAG);
    RETURN_ON_ERROR(httpd_resp_sendstr(req, json_str.get()), TAG);
    return ESP_OK;
  }

} // namespace cjf
