#include "cjf/sd.h"
#include <cjf/error_handling.h>
#include <driver/sdmmc_host.h>
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

namespace cjf
{
  static const char *TAG = "cjf:sd";

  std::expected<sd, esp_err_t> sd::mount(const config_type &config)
  {
    sdmmc_card_t *card_raw = nullptr;
    esp_err_t err = esp_vfs_fat_sdmmc_mount(config.mount_point, &config.host, &config.slot, &config.mount, &card_raw);
    card_ptr_type card(card_raw, {config.mount_point});
    RETURN_UNEXPECTED_ON_ERROR(err, TAG);
    ESP_LOGI(TAG, "SD card mounted at %s", config.mount_point);
    return sd(std::move(card), config.mount_point);
  }

  std::expected<sd, esp_err_t> sd::mount(const basic_config_type &config)
  {
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = config.max_freq_khz;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = config.clk;
    slot_config.cmd = config.cmd;
    slot_config.d0 = config.d0;
    slot_config.d1 = config.d1;
    slot_config.d2 = config.d2;
    slot_config.d3 = config.d3;
    slot_config.cd = config.cd;
    slot_config.wp = config.wp;
    bool use_4bit = config.d0 != GPIO_NUM_NC && config.d1 != GPIO_NUM_NC && config.d2 != GPIO_NUM_NC && config.d3 != GPIO_NUM_NC;
    slot_config.width = (use_4bit) ? 4 : 1;
    // slot_config.width = 1;
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = config.format_if_mount_failed,
        .max_files = config.max_files,
        .allocation_unit_size = config.allocation_unit_size,
        .disk_status_check_enable = false,
        .use_one_fat = false};
    return mount({
      .mount_point = config.mount_point,
      .host = host,
      .slot = slot_config,
      .mount = mount_config
    });
  }

  void sd::print_card_info(FILE* stream) const
  {
    // clang-format off
    if (!card_) return;
    sdmmc_card_print_info(stream, card_.get());
    // clang-format on
  }

  void sd::deleter::operator()(sdmmc_card_t *card) const
  {
    if (!card)
      return;
    esp_vfs_fat_sdcard_unmount(mount_point, card);
  }

  sd::sd(card_ptr_type card, const char *mount_point)
      : card_(card.release(), deleter(mount_point)), mount_point_(mount_point)
  {
  }

} // namespace cjf
