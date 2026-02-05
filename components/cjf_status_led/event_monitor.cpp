#include "cjf/status_led/event_monitor.h"

namespace cjf
{
  bool event_key_t::operator<(const event_key_t &rhs) const
  {
    return event_base < rhs.event_base || (event_base == rhs.event_base && event_id < rhs.event_id);
  }

  bool event_key_t::operator==(const event_key_t &rhs) const
  {
    return event_base == rhs.event_base && event_id == rhs.event_id;
  }

} // namespace cjf
