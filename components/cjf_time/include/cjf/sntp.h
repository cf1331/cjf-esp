#ifndef AF6840AB_95FF_458E_89CD_F81A2A41FB9F
#define AF6840AB_95FF_458E_89CD_F81A2A41FB9F

/**
 * @file sntp.h
 * @brief SNTP (Simple Network Time Protocol) service for ESP-IDF
 *
 * This file provides a C++ wrapper around ESP-IDF SNTP functionality with RAII
 * lifecycle management and event notification when time synchronization occurs.
 */

#include <array>
#include <cjf/error_handling.h>
#include <cjf/scope_guard.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_netif_sntp.h>
#include <expected>
#include <freertos/FreeRTOS.h>

/**
 * @brief SNTP event types
 */
typedef enum
{
  CJF_SNTP_EVENT_SYNCED ///< Posted when time synchronization completes successfully
} cjf_sntp_event_t;

/**
 * @brief ESP event base for SNTP events
 */
ESP_EVENT_DECLARE_BASE(CJF_SNTP_EVENT);

namespace cjf
{
  /**
   * @brief SNTP time synchronization service
   *
   * Provides automatic time synchronization with NTP servers using ESP-IDF's SNTP client.
   * The service automatically initializes and starts the SNTP client, and posts events
   * when synchronization occurs.
   *
   * Features:
   * - Multiple NTP server support (compile-time configurable)
   * - Automatic or manual synchronization
   * - Event notification on sync completion
   * - Optional smooth time adjustment
   * - DHCP server discovery support
   * - RAII lifecycle management (automatically stops on destruction)
   *
   * Example usage:
   * @code
   * // Start with default settings (pool.ntp.org)
   * auto sntp = cjf::sntp_service::start();
   * RETURN_ON_UNEXPECTED(sntp, TAG);
   *
   * // Or with custom servers
   * std::array<const char*, 2> servers = {"time.google.com", "pool.ntp.org"};
   * auto sntp = cjf::sntp_service::start(servers);
   *
   * // Manually trigger synchronization
   * sntp->sync(pdMS_TO_TICKS(5000));
   * @endcode
   */
  class sntp_service
  {
  public:
    /// Default NTP server (pool.ntp.org)
    static constexpr const char *default_server = "pool.ntp.org";

    /**
     * @brief Configuration for SNTP service
     * @tparam ServersCount Number of NTP servers to use (default: 1)
     */
    template <size_t ServersCount = 1>
    struct config
    {
      bool smooth_sync = false;                              ///< Use smooth time adjustment instead of stepping
      bool server_from_dhcp = false;                         ///< Get NTP server from DHCP
      bool wait_for_sync = true;                             ///< Wait for first sync to complete
      bool start = true;                                     ///< Start synchronization immediately
      bool renew_servers_after_new_IP = false;               ///< Re-request servers from DHCP on new IP
      ip_event_t ip_event_to_renew = IP_EVENT_STA_GOT_IP;    ///< IP event that triggers server renewal
      const std::array<const char *, ServersCount> &servers; ///< NTP server hostnames or IPs
    };

    /**
     * @brief Start SNTP service with default configuration
     *
     * Initializes and starts the SNTP client using the default NTP server (pool.ntp.org).
     *
     * @return Expected containing sntp_service on success, or error code on failure
     */
    static std::expected<sntp_service, esp_err_t> start();

    /**
     * @brief Start SNTP service with custom NTP servers
     *
     * Initializes and starts the SNTP client with a custom list of NTP servers.
     * Uses default configuration with immediate synchronization.
     *
     * @tparam ServersCount Number of NTP servers (deduced from array size)
     * @param servers Array of NTP server hostnames or IP addresses
     * @return Expected containing sntp_service on success, or error code on failure
     */
    template <size_t ServersCount = 1>
    static std::expected<sntp_service, esp_err_t> start(const std::array<const char *, ServersCount> &servers);

    /**
     * @brief Start SNTP service with full configuration
     *
     * Initializes and starts the SNTP client with custom configuration.
     *
     * @tparam ServersCount Number of NTP servers (must match config)
     * @param config Full SNTP configuration
     * @return Expected containing sntp_service on success, or error code on failure
     */
    template <size_t ServersCount = 1>
    static std::expected<sntp_service, esp_err_t> start(const sntp_service::config<ServersCount> &config);

    // Move-only semantics: Each sntp_service manages the global SNTP client state.
    // Copying would create multiple managers of the same state, leading to
    // double-cleanup and undefined behavior when services are destroyed.
    sntp_service(sntp_service &&) = default;
    sntp_service &operator=(sntp_service &&) = default;
    sntp_service(const sntp_service &) = delete;
    sntp_service &operator=(const sntp_service &) = delete;

    /**
     * @brief Manually trigger time synchronization
     *
     * Starts a synchronization attempt without waiting for completion.
     *
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t sync();

    /**
     * @brief Manually trigger time synchronization and wait for completion
     *
     * Starts synchronization and blocks until it completes or times out.
     *
     * @param timeout Maximum time to wait for synchronization
     * @return ESP_OK on success, ESP_ERR_TIMEOUT if timed out, other error code on failure
     */
    esp_err_t sync(TickType_t timeout);

  private:
    /**
     * @brief Private constructor - use start() factory methods instead
     */
    sntp_service();

    struct deleter
    {
      void operator()() const noexcept;
    };

    cjf::scope_guard<deleter> scope_guard_;

    /**
     * @brief Callback invoked when time synchronization completes
     *
     * Posts CJF_SNTP_EVENT_SYNCED event with the synchronized time.
     *
     * @param tv Pointer to synchronized time value
     */
    static void sync_callback_(timeval *tv);
  };

  // Template method implementations

  template <size_t ServersCount>
  std::expected<sntp_service, esp_err_t> sntp_service::start(const sntp_service::config<ServersCount> &config)
  {
    static_assert(ServersCount > 0, "At least one SNTP server must be specified");
    static_assert(ServersCount <= CONFIG_LWIP_SNTP_MAX_SERVERS, "Number of SNTP servers exceeds CONFIG_LWIP_SNTP_MAX_SERVERS");

    esp_sntp_config_t sntp_config = {
        .smooth_sync = config.smooth_sync,
        .server_from_dhcp = config.server_from_dhcp,
        .wait_for_sync = config.wait_for_sync,
        .start = config.start,
        .sync_cb = sync_callback_,
        .renew_servers_after_new_IP = config.renew_servers_after_new_IP,
        .ip_event_to_renew = config.ip_event_to_renew,
        .index_of_first_server = 0,
        .num_of_servers = ServersCount,
        .servers = config.servers.front(),
    };

    RETURN_UNEXPECTED_ON_ERROR(esp_netif_sntp_init(&sntp_config), "cjf:sntp");
    return sntp_service();
  }

  template <size_t ServersCount>
  std::expected<sntp_service, esp_err_t> sntp_service::start(const std::array<const char *, ServersCount> &servers)
  {
    sntp_service::config<ServersCount> config = {
        .servers = const_cast<std::array<const char *, ServersCount> &>(servers)};
    return start(config);
  }

} // namespace cjf

#endif // AF6840AB_95FF_458E_89CD_F81A2A41FB9F
