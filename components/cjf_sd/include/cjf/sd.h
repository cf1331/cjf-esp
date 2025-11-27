#ifndef B60CC790_6593_413A_95F4_411B2AD4D184
#define B60CC790_6593_413A_95F4_411B2AD4D184

#include <cstdio>
#include <driver/gpio.h>
#include <driver/sdmmc_host.h>
#include <driver/sdmmc_types.h>
#include <esp_err.h>
#include <esp_vfs_fat.h>
#include <expected>
#include <memory>

namespace cjf
{
  class sd
  {
  public:
    static constexpr const char *DEFAULT_MOUNT_POINT = "/sd";

    struct config_type
    {
      const char *mount_point = DEFAULT_MOUNT_POINT;
      sdmmc_host_t host;
      sdmmc_slot_config_t slot;
      esp_vfs_fat_mount_config_t mount;
    };

    struct basic_config_type
    {
      const char *mount_point = DEFAULT_MOUNT_POINT;
      gpio_num_t clk;
      gpio_num_t cmd;
      gpio_num_t d0;
      gpio_num_t d1;
      gpio_num_t d2;
      gpio_num_t d3;
      gpio_num_t cd;
      gpio_num_t wp;
      bool format_if_mount_failed = true;
      size_t allocation_unit_size = 32768;
      int max_files = 5;
      int max_freq_khz = SDMMC_FREQ_DEFAULT;
    };

    static std::expected<sd, esp_err_t> mount(const basic_config_type &config);
    static std::expected<sd, esp_err_t> mount(const config_type &config);

    sdmmc_card_t *card() { return card_.get(); }
    const char *mount_point() const { return mount_point_; }
    void print_card_info(FILE* stream = stdout) const;

  private:
    struct deleter
    {
      const char* mount_point;
      deleter(const char* mp) : mount_point(mp) {}
      void operator()(sdmmc_card_t *card) const;
    };

    using card_ptr_type = std::unique_ptr<sdmmc_card_t, deleter>;

    card_ptr_type card_;
    const char *mount_point_;

    sd(card_ptr_type card, const char *mount_point);
  };

} // namespace cjf

#endif /* B60CC790_6593_413A_95F4_411B2AD4D184 */
