#ifndef D83773A7_1BEF_46FB_B198_D222E587FA4B
#define D83773A7_1BEF_46FB_B198_D222E587FA4B

#include <cjf/error_handling.h>
#include <driver/ledc.h>
#include <expected>
#include <memory>
#include <optional>
#include <experimental/scope>

namespace cjf
{
  class ledc_timer
  {
  public:
    using config_type = ledc_timer_config_t;

    std::expected<ledc_timer, esp_err_t> init(const config_type &config);

  private:
    struct deleter
    {
      void operator()(ledc_timer_t timer);
    };

    std::experimental::unique_resource<ledc_timer_t, deleter> timer_;
    ledc_timer(ledc_timer_t timer);
  };

  class ledc_channel
  {
  public:
    using config_type = ledc_channel_config_t;

    std::expected<ledc_channel, esp_err_t> init(const ledc_timer& timer, const config_type &config);

  private:
    struct deleter
    {
      void operator()(ledc_channel_t *channel);
    };

    std::unique_ptr<ledc_channel_t, deleter> channel_;

    ledc_channel();
  };

}

#endif /* D83773A7_1BEF_46FB_B198_D222E587FA4B */
