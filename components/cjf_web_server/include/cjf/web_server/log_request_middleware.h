#ifndef AEB5C052_BBCC_4AFE_ADC9_78EFB093487C
#define AEB5C052_BBCC_4AFE_ADC9_78EFB093487C

#include "../web_server.h"

namespace cjf
{

  class log_request_middleware : public web_server::middleware
  {
  public:
    log_request_middleware();
  };

}

#endif /* AEB5C052_BBCC_4AFE_ADC9_78EFB093487C */
