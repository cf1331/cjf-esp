#ifndef B5FB95F3_1AA6_4C78_B1EE_155FB888679B
#define B5FB95F3_1AA6_4C78_B1EE_155FB888679B

#include "cjf/sd.h"
#include <cstdint>
#include <esp_timer.h>
#include <memory>
#include <sdmmc_cmd.h>

namespace cjf
{

  struct sd_write_benchmark_result
  {
    int64_t duration_us = 0;
    size_t write_bytes = 0;
    float write_speed = 0;
    size_t successful_iterations = 0;
  };

  sd_write_benchmark_result sd_benchmark_sequential_write(
      const size_t sector_size,
      const size_t sector_count,
      const size_t iterations = 1,
      const char *benchmark_file = "/sd/benchmark.bin",
      sdmmc_card_t *card = nullptr)
  {
    sd_write_benchmark_result result;
    const size_t total_bytes = sector_size * sector_count;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[total_bytes]);
    std::fill(buffer.get(), buffer.get() + total_bytes, 0x55);
    for (size_t i = 0; i < iterations; i++)
    {
      int64_t start_time, end_time, duration_us;
      size_t written_bytes, written_sectors;
      if (benchmark_file)
      {
        FILE *f = fopen(benchmark_file, "w");
        if (!f)
        {
          ESP_LOGE("cjf::sd_benchmark", "[%u] Failed to open file \"%s\" for writing", i, benchmark_file);
          continue;
        }
        start_time = esp_timer_get_time();
        written_sectors = fwrite(buffer.get(), sector_size, sector_count, f);
        end_time = esp_timer_get_time();
        written_bytes = written_sectors * sector_size;
        duration_us = end_time - start_time;
        fclose(f);
      }
      else
      {
        start_time = esp_timer_get_time();
        esp_err_t res = sdmmc_write_sectors(card, buffer.get(), 11, sector_count);
        end_time = esp_timer_get_time();
        written_bytes = (res == ESP_OK) ? total_bytes : 0;
        duration_us = end_time - start_time;
      }
      if (written_bytes == total_bytes)
      {
        float write_speed = (duration_us > 0) ? written_bytes / (duration_us / 1e6f) : -1;
        result.duration_us += duration_us;
        result.write_bytes += written_bytes;
        result.successful_iterations++;
        ESP_LOGI("cjf::sd_benchmark", "[%u] Wrote %u bytes in %lld µs (%.2f KB/s)", i, written_bytes, duration_us, write_speed / 1024);
      }
      else
      {
        ESP_LOGE("cjf::sd_benchmark", "[%u] Failed to write all data to file \"%s\" (wrote %u of %u bytes)", i, benchmark_file, written_bytes, total_bytes);
      }
    }
    if (result.duration_us > 0)
    {
      result.write_speed = result.write_bytes / (result.duration_us / 1e6f);
    }
    ESP_LOGI("cjf::sd_benchmark", "Result: Wrote %u bytes in %lld µs (%.2f KB/s) in %u successful iterations", result.write_bytes, result.duration_us, result.write_speed / 1024, result.successful_iterations);
    return result;
  }

} // namespace cjf

#endif /* B5FB95F3_1AA6_4C78_B1EE_155FB888679B */
