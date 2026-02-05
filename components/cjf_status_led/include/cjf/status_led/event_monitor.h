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
    esp_event_base_t event_base; ///< Event base (e.g., WIFI_EVENT, IP_EVENT)
    int32_t event_id;            ///< Event ID within the base

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
   * @brief Type alias for event-to-mode mapping
   *
   * Maps event keys to LED animation modes. The StatusLedMode template parameter
   * allows using custom mode variants that include user-defined modes.
   *
   * @tparam StatusLedMode The mode type stored in the map (default: status_led_mode variant)
   */
  template <typename StatusLedMode = status_led_mode>
  using status_led_event_map = std::map<event_key_t, StatusLedMode>;

  /**
   * @brief Automatic status LED controller based on system events
   *
   * Monitors ESP-IDF events and automatically changes the status LED mode
   * when matching events occur. Useful for providing visual feedback for
   * system state changes (WiFi connecting, IP acquired, errors, etc.).
   *
   * Template parameter LedStrip can be cjf::led_strip or any RAII wrapper type
   * that fulfils the LedStripLike concept.
   *
   * Template parameter EventMap is the map type for storing event->mode mappings.
   * Should be std::map<event_key_t, status_led_mode> or compatible.
   *
   * Example:
   * @code
   * using event_map_t = std::map<cjf::event_key_t, cjf::status_led_mode>;
   * static event_map_t events;
   * events[{WIFI_EVENT, WIFI_EVENT_STA_CONNECTED}] =
   *     cjf::status_led_blink::create({.led = 0, .color = cjf::colors::blue, ...});
   *
   * std::expected<cjf::status_led_event_monitor<cjf::led_strip, event_map_t>, esp_err_t> monitor;
   * cjf::status_led_event_monitor<cjf::led_strip, event_map_t>::init(monitor, std::move(led_strip), events);
   * @endcode
   */
  template <LedStripLike LedStrip = cjf::led_strip,
            typename EventMap = std::map<event_key_t, status_led_mode>>
  class status_led_event_monitor
  {
  public:
    /**
     * @brief Initialize an event monitor in-place within a std::expected
     *
     * @param[out] monitor Reference to std::expected to construct into
     * @param leds LED strip or wrapper to manage
     * @param event_modes Map of events to LED modes (must outlive this object)
     * @return Event monitor on success, or error code on failure
     *
     * Creates an internal status_led controller and registers event handlers
     * for all events in the map. When a matching event occurs, the corresponding
     * mode is applied to the LED strip.
     *
     * Sets monitor to error if event handler registration fails for any event.
     */
    static std::expected<status_led_event_monitor, esp_err_t> &init(
        std::expected<status_led_event_monitor, esp_err_t> &monitor,
        LedStrip leds,
        const EventMap &event_modes);

    void clear_override() noexcept;
    void clear_override_on_next_event() noexcept;
    void disable() noexcept;
    void enable() noexcept;
    void override(const status_led_mode_base *mode) noexcept;
    void reapply_last_mode() noexcept;

    // Non-copyable, non-movable (contains non-movable status_led)
    status_led_event_monitor(const status_led_event_monitor &) = delete;
    status_led_event_monitor &operator=(const status_led_event_monitor &) = delete;
    status_led_event_monitor(status_led_event_monitor &&) = delete;
    status_led_event_monitor &operator=(status_led_event_monitor &&) = delete;

    // Public constructor for std::expected::emplace() - use init() instead
    status_led_event_monitor(LedStrip leds, const EventMap &event_modes) noexcept;

  private:
    cjf::status_led<LedStrip> status_led_;
    const EventMap &event_modes_;
    std::vector<cjf::event_handler> event_handlers_;
    event_key_t last_event_{};
    const status_led_mode_base *override_mode_ = nullptr;
    bool enabled_ = true;

    static void event_handler_(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  };

  // ============================================================================
  // Template implementation
  // ============================================================================

  template <LedStripLike LedStrip, typename EventMap>
  status_led_event_monitor<LedStrip, EventMap>::status_led_event_monitor(LedStrip leds, const EventMap &event_modes) noexcept
      : status_led_(std::move(leds)), event_modes_(event_modes) {}

  template <LedStripLike LedStrip, typename EventMap>
  std::expected<status_led_event_monitor<LedStrip, EventMap>, esp_err_t> &
  status_led_event_monitor<LedStrip, EventMap>::init(
      std::expected<status_led_event_monitor, esp_err_t> &monitor,
      LedStrip leds,
      const EventMap &event_modes)
  {
    // Construct in-place
    monitor.emplace(std::move(leds), event_modes);

    // Register event handlers
    monitor->event_handlers_.reserve(event_modes.size());
    for (const auto &[key, mode] : event_modes)
    {
      auto handler = cjf::event_handler::create(
          key.event_base,
          key.event_id,
          event_handler_,
          &(*monitor));
      if (!handler)
      {
        monitor = std::unexpected(handler.error());
        return monitor;
      }
      monitor->event_handlers_.emplace_back(std::move(*handler));
    }
    return monitor;
  }

  template <LedStripLike LedStrip, typename EventMap>
  void status_led_event_monitor<LedStrip, EventMap>::clear_override() noexcept
  {
    override_mode_ = nullptr;
    reapply_last_mode();
  }

  template <LedStripLike LedStrip, typename EventMap>
  void status_led_event_monitor<LedStrip, EventMap>::clear_override_on_next_event() noexcept
  {
    override_mode_ = nullptr;
  }

  template <LedStripLike LedStrip, typename EventMap>
  void status_led_event_monitor<LedStrip, EventMap>::disable() noexcept
  {
    enabled_ = false;
    status_led_.set_mode(nullptr);
  }

  template <LedStripLike LedStrip, typename EventMap>
  void status_led_event_monitor<LedStrip, EventMap>::enable() noexcept
  {
    enabled_ = true;
    if (override_mode_)
    {
      status_led_.set_mode(override_mode_);
    }
    else
    {
      reapply_last_mode();
    }
  }

  template <LedStripLike LedStrip, typename EventMap>
  void status_led_event_monitor<LedStrip, EventMap>::override(const status_led_mode_base *mode) noexcept
  {
    override_mode_ = mode;
    status_led_.set_mode(mode);
  }

  template <LedStripLike LedStrip, typename EventMap>
  void status_led_event_monitor<LedStrip, EventMap>::reapply_last_mode() noexcept
  {
    if (last_event_.event_base == nullptr)
      return;
    auto it = event_modes_.find(last_event_);
    if (it == event_modes_.end())
      return;
    const auto &mode = it->second;
    const status_led_mode_base *base_ptr = std::visit(
        [](const auto &m) -> const status_led_mode_base * { return &m; },
        mode);
    status_led_.set_mode(base_ptr);
  }

  template <LedStripLike LedStrip, typename EventMap>
  void status_led_event_monitor<LedStrip, EventMap>::event_handler_(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
  {
    auto self = static_cast<status_led_event_monitor *>(arg);
    auto it = self->event_modes_.find({event_base, event_id});
    if (it == self->event_modes_.end())
      return;
    self->last_event_ = {event_base, event_id};
    if (!self->enabled_ || self->override_mode_)
      return;
    const auto &mode = it->second;
    const status_led_mode_base *base_ptr = std::visit(
        [](const auto &m) -> const status_led_mode_base * { return &m; },
        mode);
    self->status_led_.set_mode(base_ptr);
  }

} // namespace cjf

#endif // DAC8546A_6CC2_4C4C_B854_4862708BF9DC
