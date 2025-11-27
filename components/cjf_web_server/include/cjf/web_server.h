#ifndef A18CDF7C_AE1E_43CF_AD32_F2E605FCEED3
#define A18CDF7C_AE1E_43CF_AD32_F2E605FCEED3

#include "cjf/task_pool.h"
#include <cJSON.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <expected>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <functional>
#include <list>
#include <memory>
#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

namespace cjf
{

  class web_server
  {
  public:
    using next_func = std::function<esp_err_t()>;

    /// Function signature for request handlers.
    ///
    /// Handlers can be used in two ways:
    /// - Middleware: Calls next() and returns its result (e.g., logging, authentication)
    /// - Route handlers: Sends a response and returns without calling next()
    ///
    /// @param req HTTP request object
    /// @param next Function to invoke the next middleware in the chain
    /// @return ESP_OK on success, or an error code that will be converted to HTTP status
    using handler_func = std::function<esp_err_t(httpd_req_t *req, next_func next)>;

    /// Middleware registration containing a handler function and optional metadata.
    struct middleware
    {
      const char *name;     ///< Optional name for logging (must be a string literal or static)
      handler_func handler; ///< The handler function (capture state in the lambda instead of using external context)
      bool is_async = false; ///< Whether this handler should run asynchronously (allows other requests to be processed)
    };

    struct route
    {
      const std::string uri;
      const web_server::middleware middleware;

      bool can_handle(httpd_req_t *req)
      {
        return httpd_uri_match_wildcard(uri.c_str(), req->uri, strlen(req->uri));
      }
    };

    struct builder
    {
      builder(const httpd_config_t &config);
      builder &use(middleware m);
      builder &use(handler_func handler);
      builder &use(const char *path, middleware m);
      builder &use(const char *path, handler_func handler);
      builder &set_async_worker_count(size_t count);
      builder &set_async_queue_depth(size_t depth);
      std::expected<std::unique_ptr<web_server>, esp_err_t> listen();

    private:
      httpd_config_t config_;
      std::list<route> routes_;
      size_t async_worker_count_ = 3;  ///< Number of worker tasks for async handlers
      size_t async_queue_depth_ = 5;   ///< Depth of async request queue
    };

    static builder init(const httpd_config_t &config = HTTPD_DEFAULT_CONFIG());
    ~web_server();

    void log_endpoints();

  private:
    struct deleter
    {
      void operator()(void *handle) const;
    };

    struct chain_context;

    /// Structure for async request handling
    struct async_req
    {
      httpd_req_t *req;
      handler_func handler;
      std::shared_ptr<chain_context> ctx;
    };

    /// Shared state for middleware chain execution
    struct chain_context
    {
      std::list<web_server::route>::iterator route;
      std::list<web_server::route> *routes;
      cjf::task_pool<async_req> *task_pool;
      bool is_async = false;
      httpd_req_t *req = nullptr;  // Current request (switches to async_req when async handling starts)
      std::function<esp_err_t()> next;
    };

    // Members are destroyed in reverse order of declaration.
    // We want task_pool_ destroyed last (after handle_ stops accepting requests).
    task_pool<async_req> task_pool_;
    std::unique_ptr<void, deleter> handle_;
    const uint16_t port_;
    std::list<route> routes_;

    web_server(std::unique_ptr<void, deleter> handle, const uint16_t port, std::list<route> routes, task_pool<async_req> task_pool);

    esp_err_t register_handler_for_method(const httpd_method_t method);

    static esp_err_t req_handler(httpd_req_t *req);

    static bool uri_match_any(const char *uri_template, const char *uri_to_match, size_t match_upto);
  };

  esp_err_t send_json_response(httpd_req_t *req, cJSON *json);

} // namespace cjf

#endif /* A18CDF7C_AE1E_43CF_AD32_F2E605FCEED3 */
