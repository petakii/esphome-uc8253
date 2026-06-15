#include "uc8253.h"

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace uc8253 {

static const char *const TAG = "uc8253";

static const uint8_t UC8253_CMD_PANEL_SETTING = 0x00;
static const uint8_t UC8253_CMD_POWER_OFF = 0x02;
static const uint8_t UC8253_CMD_POWER_ON = 0x04;
static const uint8_t UC8253_CMD_DEEP_SLEEP = 0x07;
static const uint8_t UC8253_CMD_WRITE_RAM1 = 0x10;
static const uint8_t UC8253_CMD_DISPLAY_REFRESH = 0x12;
static const uint8_t UC8253_CMD_WRITE_RAM2 = 0x13;
static const uint8_t UC8253_CMD_LUT = 0x20;
static const uint8_t UC8253_CMD_RESOLUTION = 0x61;
static const uint8_t UC8253_CMD_GET_STATUS = 0x71;
static const uint8_t UC8253_CMD_VCOM_CDI = 0x50;
static const uint32_t UC8253_RESET_HIGH_MS = 20;
static const uint32_t UC8253_RESET_LOW_MS = 2;
static const uint32_t UC8253_BUSY_POLL_MS = 10;
static const uint32_t UC8253_SOFT_SPI_HALF_CLOCK_US = 1;

void UC8253::setup() {
  this->configure_model_();

  this->clk_pin_->setup();
  this->clk_pin_->digital_write(false);
  this->mosi_pin_->setup();
  this->mosi_pin_->digital_write(false);
  this->cs_pin_->setup();
  this->cs_pin_->digital_write(true);
  this->dc_pin_->setup();
  this->dc_pin_->digital_write(false);

  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
  }
  if (this->busy_pin_ != nullptr)
    this->busy_pin_->setup();

  const size_t len = this->get_buffer_length();
  this->black_plane_.assign(len, 0xFF);
  this->color_plane_.assign(len, 0xFF);

  this->init_controller_();
  this->display();
}

void UC8253::dump_config() {
  ESP_LOGCONFIG(TAG, "UC8253 E-Paper Display:");
  ESP_LOGCONFIG(TAG, "  Model: %s", this->model_ == UC8253_MODEL_3_7 ? "3.7\"" : "2.13\"");
  ESP_LOGCONFIG(TAG, "  Resolution: %ux%u", this->width_, this->height_);
  ESP_LOGCONFIG(TAG, "  Refresh mode: %s", this->refresh_mode_ == UC8253_REFRESH_PARTIAL ? "partial" : "full");
  ESP_LOGCONFIG(TAG, "  Full update every: %u", this->full_update_every_);
  LOG_PIN("  CLK Pin: ", this->clk_pin_);
  LOG_PIN("  MOSI Pin: ", this->mosi_pin_);
  LOG_PIN("  CS Pin: ", this->cs_pin_);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
}

void UC8253::update() {
  this->do_update_();
  this->display();
}

void UC8253::on_shutdown() { this->deep_sleep_(); }

int UC8253::get_width_internal() { return this->width_; }
int UC8253::get_height_internal() { return this->height_; }

size_t UC8253::get_buffer_length() {
  return (static_cast<size_t>(this->width_) * this->height_ + 7u) / 8u;
}

void UC8253::draw_absolute_pixel_internal(int x, int y, display::Color color) {
  if (x < 0 || y < 0 || x >= this->width_ || y >= this->height_)
    return;

  const uint32_t idx = static_cast<uint32_t>(x) + static_cast<uint32_t>(y) * this->width_;
  const bool is_white = !color.is_on();
  bool is_red = false;

  if (!is_white && this->has_color_) {
    is_red = color.r > color.g && color.r > color.b;
  }

  if (is_white) {
    this->set_plane_bit_(this->black_plane_, idx, true);
    this->set_plane_bit_(this->color_plane_, idx, true);
  } else if (is_red) {
    this->set_plane_bit_(this->black_plane_, idx, true);
    this->set_plane_bit_(this->color_plane_, idx, false);
  } else {
    this->set_plane_bit_(this->black_plane_, idx, false);
    this->set_plane_bit_(this->color_plane_, idx, true);
  }
}

void UC8253::display() {
  if (this->refresh_mode_ == UC8253_REFRESH_PARTIAL && this->refresh_counter_ % this->full_update_every_ != 0) {
    this->send_command_(UC8253_CMD_VCOM_CDI);
    this->send_data_(0x17);
  } else {
    this->send_command_(UC8253_CMD_VCOM_CDI);
    this->send_data_(0x37);
  }

  this->write_buffers_();

  this->send_command_(UC8253_CMD_DISPLAY_REFRESH);
  this->wait_until_idle_();
  this->refresh_counter_++;
}

void UC8253::configure_model_() {
  switch (this->model_) {
    case UC8253_MODEL_2_13:
      this->width_ = 250;
      this->height_ = 122;
      break;
    case UC8253_MODEL_3_7:
    default:
      this->width_ = 416;
      this->height_ = 240;
      break;
  }
}

void UC8253::init_controller_() {
  this->hardware_reset_();

  this->power_on_();

  this->send_command_(UC8253_CMD_PANEL_SETTING);
  this->send_data_(this->has_color_ ? 0x0F : 0x1F);

  this->send_command_(UC8253_CMD_RESOLUTION);
  this->send_data_(this->width_ >> 8);
  this->send_data_(this->width_ & 0xFF);
  this->send_data_(this->height_ >> 8);
  this->send_data_(this->height_ & 0xFF);

  this->send_command_(UC8253_CMD_VCOM_CDI);
  this->send_data_(0x37);

  if (!this->lut_.empty()) {
    this->send_command_(UC8253_CMD_LUT);
    this->send_data_(this->lut_.data(), this->lut_.size());
  }
}

void UC8253::write_buffers_() {
  this->send_command_(UC8253_CMD_WRITE_RAM1);
  for (uint8_t value : this->black_plane_)
    this->send_data_(this->black_invert_ ? static_cast<uint8_t>(~value) : value);

  this->send_command_(UC8253_CMD_WRITE_RAM2);
  for (uint8_t value : this->color_plane_) {
    const uint8_t output = this->has_color_ ? value : 0xFF;
    this->send_data_(this->color_invert_ ? static_cast<uint8_t>(~output) : output);
  }
}

void UC8253::power_on_() {
  this->send_command_(UC8253_CMD_POWER_ON);
  this->wait_until_idle_();
}

void UC8253::power_off_() {
  this->send_command_(UC8253_CMD_POWER_OFF);
  this->wait_until_idle_();
}

void UC8253::deep_sleep_() {
  this->power_off_();
  this->send_command_(UC8253_CMD_DEEP_SLEEP);
  this->send_data_(0xA5);
}

void UC8253::hardware_reset_() {
  if (this->reset_pin_ == nullptr) {
    return;
  }
  this->reset_pin_->digital_write(true);
  delay(UC8253_RESET_HIGH_MS);
  this->reset_pin_->digital_write(false);
  delay(UC8253_RESET_LOW_MS);
  this->reset_pin_->digital_write(true);
  delay(UC8253_RESET_HIGH_MS);
}

void UC8253::wait_until_idle_(uint32_t timeout_ms) {
  if (this->busy_pin_ == nullptr) {
    return;
  }

  const uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    this->send_command_(UC8253_CMD_GET_STATUS);
    const bool busy_level = this->busy_pin_->digital_read();
    const bool busy = this->busy_active_high_ ? busy_level : !busy_level;
    if (!busy) {
      return;
    }
    delay(UC8253_BUSY_POLL_MS);
  }

  ESP_LOGW(TAG, "Timed out waiting for busy pin");
}

void UC8253::send_command_(uint8_t command) {
  this->dc_pin_->digital_write(false);
  this->spi_write_byte_(command);
}

void UC8253::send_data_(uint8_t data) {
  this->dc_pin_->digital_write(true);
  this->spi_write_byte_(data);
}

void UC8253::send_data_(const uint8_t *data, size_t len) {
  this->dc_pin_->digital_write(true);
  for (size_t i = 0; i < len; i++) {
    this->spi_write_byte_(data[i]);
  }
}

void UC8253::spi_write_byte_(uint8_t value) {
  this->cs_pin_->digital_write(false);
  for (uint8_t bit = 0; bit < 8; bit++) {
    this->clk_pin_->digital_write(false);
    this->mosi_pin_->digital_write((value & 0x80u) != 0);
    delayMicroseconds(UC8253_SOFT_SPI_HALF_CLOCK_US);
    this->clk_pin_->digital_write(true);
    delayMicroseconds(UC8253_SOFT_SPI_HALF_CLOCK_US);
    value <<= 1;
  }
  this->cs_pin_->digital_write(true);
}

}  // namespace uc8253
}  // namespace esphome
