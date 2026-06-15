#pragma once

#include <vector>
#include "esphome/components/display/display_buffer.h"
#include "esphome/core/color.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace uc8253 {

enum UC8253Model {
  UC8253_MODEL_3_7,
  UC8253_MODEL_2_13,
};

enum UC8253RefreshMode {
  UC8253_REFRESH_FULL,
  UC8253_REFRESH_PARTIAL,
};

class UC8253 : public esphome::display::DisplayBuffer {
 public:
  void setup() override;
  void dump_config() override;
  void on_shutdown() override;
  void update() override;

  esphome::display::DisplayType get_display_type() override { 
    return esphome::display::DisplayType::DISPLAY_TYPE_COLOR; 
  }

  void set_model(UC8253Model model) { this->model_ = model; }
  void set_refresh_mode(UC8253RefreshMode refresh_mode) { this->refresh_mode_ = refresh_mode; }
  void set_full_update_every(uint32_t value) { this->full_update_every_ = value; }
  void set_black_invert(bool value) { this->black_invert_ = value; }
  void set_color_invert(bool value) { this->color_invert_ = value; }
  void set_busy_active_high(bool value) { this->busy_active_high_ = value; }
  void set_has_color(bool value) { this->has_color_ = value; }

  void set_clk_pin(esphome::GPIOPin *pin) { this->clk_pin_ = pin; }
  void set_mosi_pin(esphome::GPIOPin *pin) { this->mosi_pin_ = pin; }
  void set_cs_pin(esphome::GPIOPin *pin) { this->cs_pin_ = pin; }
  void set_dc_pin(esphome::GPIOPin *pin) { this->dc_pin_ = pin; }
  void set_reset_pin(esphome::GPIOPin *pin) { this->reset_pin_ = pin; }
  void set_busy_pin(esphome::GPIOPin *pin) { this->busy_pin_ = pin; }
  
  void set_lut(const ::std::vector<uint8_t> &lut) { this->lut_ = lut; }

 protected:
  // JAVÍTÁS: display::Color helyett esphome::Color van, így az override érvényes!
  void draw_absolute_pixel_internal(int x, int y, esphome::Color color) override;
  int get_width_internal() override;
  int get_height_internal() override;
  
  // JAVÍTÁS: Az aláhúzás jelzi a belső függvényt, nincs rajta override!
  size_t get_buffer_length_();
  void write_display_data_();

  void configure_model_();
  void init_controller_();
  void write_buffers_();
  void power_on_();
  void power_off_();
  void deep_sleep_();
  void hardware_reset_();
  void wait_until_idle_(uint32_t timeout_ms = 1000);
  void send_command_(uint8_t command);
  void send_data_(uint8_t data);
  void send_data_(const uint8_t *data, size_t len);
  void spi_write_byte_(uint8_t value);
  void set_plane_bit_(::std::vector<uint8_t> &plane, uint32_t idx, bool value);

  UC8253Model model_;
  UC8253RefreshMode refresh_mode_{UC8253_REFRESH_FULL};
  uint32_t full_update_every_{1};
  uint32_t refresh_counter_{0};
  bool black_invert_{false};
  bool color_invert_{false};
  bool busy_active_high_{false};
  bool has_color_{false};
  int width_{0};
  int height_{0};

  esphome::GPIOPin *clk_pin_{nullptr};
  esphome::GPIOPin *mosi_pin_{nullptr};
  esphome::GPIOPin *cs_pin_{nullptr};
  esphome::GPIOPin *dc_pin_{nullptr};
  esphome::GPIOPin *reset_pin_{nullptr};
  esphome::GPIOPin *busy_pin_{nullptr};

  ::std::vector<uint8_t> black_plane_;
  ::std::vector<uint8_t> color_plane_;

 private:
  ::std::vector<uint8_t> lut_;
};

} // namespace uc8253
} // namespace esphome
