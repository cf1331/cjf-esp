#ifndef CJF_WIFI_EVENTS_H
#define CJF_WIFI_EVENTS_H

#include <esp_event.h>

typedef enum
{
  CJF_WIFI_EVENT_CONNECTING,
  CJF_WIFI_EVENT_PROVISIONING,
  CJF_WIFI_EVENT_AUTH_FAILED,
} cjf_wifi_event_t;

ESP_EVENT_DEFINE_BASE(CJF_WIFI_EVENT);

namespace cjf
{

} // namespace cjf

#endif // CJF_WIFI_EVENTS_H
