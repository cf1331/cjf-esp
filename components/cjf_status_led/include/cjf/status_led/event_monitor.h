#ifndef DAC8546A_6CC2_4C4C_B854_4862708BF9DC
#define DAC8546A_6CC2_4C4C_B854_4862708BF9DC

/**
 * @file event_monitor.h
 * @brief Event-driven status LED mode switching
 *
 * This file provides automatic status LED mode changes in response to ESP-IDF
 * events. Maps specific events to LED animation modes for visual feedback.
 */

#include "cjf/status_led.h"
#include "cjf/status_led/modes.h"
#include <cjf/event.h>
#include <esp_err.h>
#include <expected>
#include <map>
#include <vector>

namespace cjf
{

  /**
   * @brief Key type for identifying ESP-IDF events
   *
   * Combines event base and event ID to uniquely identify events
   * for use as map keys.
   */
  struct event_key_t
  {
    esp_event_base_t event_base;  ///< Event base (e.g., WIFI_EVENT, IP_EVENT)
    int32_t event_id;              ///< Event ID within the base

    /**
     * @brief Compare for ordering in std::map
     * @param rhs Right-hand side of comparison
     * @return true if this key is less than rhs
     */
    bool operator<(const event_key_t &rhs) const;

    /**
     * @brief Compare for equality
     * @param rhs Right-hand side of comparison
     * @return true if events are identical
     */
    bool operator==(const event_key_t &rhs) const;
  };

  /**
   * @brief Automatic status LED controller based on system events
   *
   * Monitors ESP-IDF events and automatically changes the status LED mode
   * when matching events occur. Useful for providing visual feedback for
   * system state changes (WiFi connecting, IP acquired, errors, etc.).
   *
   * Example:
   * @code
   * cjf::status_led_event_monitor::event_map events;
   * events[{WIFI_EVENT, WIFI_EVENT_STA_CONNECTED}] =
   *     cjf::status_led_blink::create({.led = 0, .color = cjf::colors::blue, ...});
   * events[{IP_EVENT, IP_EVENT_STA_GOT_IP}] =
   *     cjf::status_led_pulse::create({.led = 0, .color = cjf::colors::green, ...});
   *
   * auto monitor = cjf::status_led_event_monitor::create(status_led, events);
   * @endcode
   */
  class status_led_event_monitor
  {
  public:
    /**
     * @brief Map of events to LED animation modes
     *
     * Associates event keys with the mode to activate when that event occurs.
     * The modes are owned by the map and must remain valid for the monitor's lifetime.
     */
    using event_map = std::map<event_key_t, std::unique_ptr<status_led_mode>>;

    /**
     * @brief Create an event monitor
     * @param status_led The status LED controller to manage (must outlive this object)
     * @param event_modes Map of events to LED modes (must outlive this object)
     * @return Event monitor on success, or error code on failure
     *
     * Registers event handlers for all events in the map. When a matching event
     * occurs, the corresponding mode is applied to the status LED.
     *
     * Returns error if event handler registration fails for any event.
     */
    static std::expected<status_led_event_monitor, esp_err_t> create(
        cjf::status_led &status_led,
        const event_map &event_modes);

    void reapply_last_mode() noexcept;

  private:
    struct context
    {
      const event_map &event_modes;
      cjf::status_led &status_led;
      std::vector<cjf::event_handler> event_handlers;
      event_key_t last_event;
    };
    std::unique_ptr<context> ctx_;

    explicit status_led_event_monitor(std::unique_ptr<context> ctx) noexcept;

    static void event_handler_(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  };

} // namespace cjf

#endif // DAC8546A_6CC2_4C4C_B854_4862708BF9DC
