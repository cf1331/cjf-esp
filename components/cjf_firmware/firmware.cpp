#include "cjf/firmware.h"
#include <esp_cpu.h>
#include <esp_log.h>
#include <hal/usb_wrap_hal.h>
#include <hal/usb_wrap_ll.h>
#include <hal/clk_gate_ll.h>
#include <rom/usb/usb_dc.h>
#include <rom/usb/usb_persist.h>
#include <rom/usb/chip_usb_dw_wrapper.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>
#include <soc/efuse_reg.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/usb_struct.h>
#include <soc/usb_reg.h>
#include <soc/usb_wrap_reg.h>
#include <soc/usb_wrap_struct.h>
#include <soc/usb_periph.h>
#include <soc/periph_defs.h>
#include <soc/timer_group_struct.h>
#include <soc/system_reg.h>
#include <esp_system.h>

namespace cjf
{

static void IRAM_ATTR usb_persist_shutdown_handler()
{
  // Based on https://github.com/espressif/arduino-esp32/blob/b20655a009008e374fd37f5a6c45f3ba53b9ba86/cores/esp32/esp32-hal-tinyusb.c#L568
  periph_ll_reset(PERIPH_USB_MODULE);
  periph_ll_enable_clk_clear_rst(PERIPH_USB_MODULE);
  REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
}

void reset_to_bootloader_usb_firmware_download()
{
  esp_register_shutdown_handler(usb_persist_shutdown_handler);
  esp_restart();
}

} // namespace cjf
