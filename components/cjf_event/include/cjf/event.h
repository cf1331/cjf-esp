#ifndef A8667D7E_A830_4F3D_83FB_4C800317C15C
#define A8667D7E_A830_4F3D_83FB_4C800317C15C

/**
 * @file event.h
 * @brief ESP-IDF event handler wrapper for C++
 *
 * This file provides a C++ wrapper class for ESP-IDF event handling functionality,
 * offering RAII-based automatic registration and deregistration of event handlers.
 */

#include <esp_event.h>
#include <expected>
#include <functional>
#include <memory>
#include <utility>

namespace cjf
{

  /**
   * @brief RAII wrapper for ESP-IDF event handlers
   *
   * Provides automatic registration and deregistration of event handlers with the
   * ESP-IDF event loop system. The handler is automatically unregistered when the
   * object is destroyed, ensuring proper cleanup and preventing dangling handlers.
   *
   * This class uses move-only semantics to ensure exclusive ownership of the
   * event handler registration.
   */
  class event_handler
  {
  public:
    /**
     * @brief Create an event handler with user-provided argument
     *
     * Registers an event handler for the specified event base and ID.
     *
     * @warning The event handler argument is NOT managed by this class and must
     * remain valid for the ENTIRE LIFETIME of the event_handler object. If you need
     * automatic memory management, use `create_with_managed_arg()` instead.
    *
     * @param event_base The event base to register for (e.g., WIFI_EVENT, IP_EVENT)
     * @param event_id The specific event ID to handle
     * @param handler The callback function to be called when the event occurs
     * @param event_handler_arg Optional user data passed to the handler function
     *                          (MUST remain valid for the lifetime of this object)
     * @return Expected containing event_handler on success, or esp_err_t on failure
     */
    static std::expected<event_handler, esp_err_t> create(
        esp_event_base_t event_base,
        int32_t event_id,
        esp_event_handler_t handler,
        void *event_handler_arg = nullptr) noexcept;

    /**
     * @brief Create an event handler with managed argument
     *
     * Registers an event handler for the specified event base and ID with automatic
     * memory management of the handler argument. The provided deleter function will
     * be called to clean up the argument when the event_handler is destroyed.
     *
     * @param event_base The event base to register for (e.g., WIFI_EVENT, IP_EVENT)
     * @param event_id The specific event ID to handle
     * @param handler The callback function to be called when the event occurs
     * @param event_handler_arg User data passed to the handler function (will be managed)
     * @param deleter Function to call to clean up the handler argument
     * @return Expected containing event_handler on success, or esp_err_t on failure
     */
    static std::expected<event_handler, esp_err_t> create_with_managed_arg(
        esp_event_base_t event_base,
        int32_t event_id,
        esp_event_handler_t handler,
        void *event_handler_arg,
        void (*deleter)(void *) = nullptr) noexcept;

    // Move-only semantics: Each `event_handler` represents exclusive ownership of an
    // event handler registration. Copying would create multiple registrations for the
    // same handler, leading to undefined behavior during deregistration.

    /**
     * @brief Move constructor
     * @param other The event_handler to move from
     */
    event_handler(event_handler &&other) noexcept;

    /**
     * @brief Move assignment operator
     * @param other The event_handler to move from
     * @return Reference to this object
     */
    event_handler &operator=(event_handler &&other) noexcept;

    event_handler(const event_handler &) = delete;
    event_handler &operator=(const event_handler &) = delete;

    /**
     * @brief Destructor - automatically unregisters the event handler
     *
     * Ensures proper cleanup by unregistering the event handler from the event loop.
     * If a managed argument was provided, its deleter function will also be called.
     */
    ~event_handler();

  private:
    /// The event base this handler is registered for
    esp_event_base_t event_base_;

    /// The specific event ID this handler is registered for
    int32_t event_id_;

    /// The callback function registered with the event loop
    esp_event_handler_t handler_;

    /// Managed argument with custom deleter (nullptr if not using managed argument)
    std::unique_ptr<void, void (*)(void *)> managed_arg_;

    /**
     * @brief Private constructor for event handlers
     * @param event_base The event base to register for
     * @param event_id The specific event ID to handle
     * @param handler The callback function
     * @param managed_arg Unique pointer with custom deleter for the handler argument
     *                    (defaults to nullptr for unmanaged arguments)
     */
    event_handler(
        esp_event_base_t event_base,
        int32_t event_id,
        esp_event_handler_t handler,
        std::unique_ptr<void, void (*)(void *)> managed_arg = {nullptr, nullptr}) noexcept;
  };

  class event_handler_2
  {
  public:
    using handler_func = std::function<void(esp_event_base_t, int32_t, void *)>;

    static std::expected<event_handler_2, esp_err_t> &
    emplace_and_register_with_default_loop(
        std::expected<event_handler_2, esp_err_t> &event_handler,
        const esp_event_base_t event_base, const int32_t event_id,
        handler_func handler) noexcept;

    /**
     * @brief Constructor with optional handler function
     * @param handler The handler function to use (default: empty function)
     */
    explicit event_handler_2(handler_func handler = {}) noexcept;

    /**
     * @brief Move constructor
     * @param other The event_handler_2 to move from
     *
     * @note Moving and already registered handler is not recommended and will
     * trigger a warning as the event loop registration is tied to the address
     * of this object. Moving an already registered handler will attempt to
     * unregister the existing handler and re-register with the new address,
     * but if any step fails, the system will `abort()`.
     */
    event_handler_2(event_handler_2 &&other) noexcept;

    /**
     * @brief Move assignment operator
     * @param other The event_handler_2 to move from
     * @return Reference to this object
     *
     * @note Moving and already registered handler is not recommended and will
     * trigger a warning as the event loop registration is tied to the address
     * of this object. Moving an already registered handler will attempt to
     * unregister the existing handler and re-register with the new address,
     * but if any step fails, the system will `abort()`.
     */
    event_handler_2 &operator=(event_handler_2 &&other) noexcept;

    event_handler_2(const event_handler_2 &) = delete;
    event_handler_2 &operator=(const event_handler_2 &) = delete;

    /**
     * @brief Destructor - automatically unregisters the event handler
     */
    ~event_handler_2();

    /**
     * @brief Register this handler for the specified event
     * @param event_base The event base to register for
     * @param event_id The specific event ID to handle
     * @return ESP_OK on success, or ESP_ERR_INVALID_STATE if handler is already registered
     */
    esp_err_t register_with_default_loop(esp_event_base_t event_base, int32_t event_id) noexcept;

  private:
    handler_func handler_;
    esp_event_base_t event_base_ = nullptr;
    int32_t event_id_ = 0;

    static void event_dispatcher_(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) noexcept;

    void move_registration_(event_handler_2 &&other) noexcept;
  };

} // namespace cjf

#endif /* A8667D7E_A830_4F3D_83FB_4C800317C15C */
