#include "hi_link_ld1125h.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

namespace esphome::hi_link_ld1125h {
static const char *const TAG = "hi_link_ld1125h";
static bool parse_float(const std::string &text, float &value) {
  if (text.empty()) return false;
  char *end;
  errno = 0;
  value = strtof(text.c_str(), &end);
  return errno != ERANGE && end != text.c_str() && *end == '\0' && std::isfinite(value);
}
static std::string trim(const std::string &text) {
  const auto first = text.find_first_not_of(" \t\r");
  if (first == std::string::npos) return {};
  return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}
void RadarNumber::control(float value) {
  if (!std::isfinite(value)) return;
  char text[32];
  snprintf(text, sizeof(text), "%.*f", decimals_, value);
  parent_->send_command(command_ + "=" + text, true);
}
void TestMode::write_state(bool state) { parent_->send_command(state ? "test_mode=1" : "test_mode=0", true); }
void CommandButton::press_action() { parent_->send_command(command_, command_ == "initial"); }
void LD1125H::setup() {
  for (auto *entity : {target_, movement_, stationary_})
    if (entity != nullptr) entity->publish_state(false);
  const auto initial_test_mode = test_mode_ != nullptr ? test_mode_->get_initial_state_with_restore_mode() : optional<bool>{};
  // Read existing settings; never overwrite calibration or persist flash on boot.
  const auto query = [this]() {
    this->send_command("get_all");
    if (firmware_version_ != nullptr) this->send_command("VER");
  };
  this->set_timeout("read_settings", 1000, [this, initial_test_mode, query]() {
    if (initial_test_mode.has_value()) this->send_command(*initial_test_mode ? "test_mode=1" : "test_mode=0");
    query();
  });
  // The radar can still be starting when the Wi-Fi MCU makes its first query.
  this->set_timeout("retry_read_settings", 10000, query);
}
void LD1125H::dump_config() { ESP_LOGCONFIG(TAG, "Hi-Link LD1125H, 115200 8N1 ASCII UART"); }
bool LD1125H::send_command(const std::string &command, bool save) {
  if (command.empty() || command.size() > 96 || command.find_first_of("\r\n") != std::string::npos ||
      queue_.size() + (save ? 2 : 1) > 16) {
    ESP_LOGW(TAG, "Command rejected (invalid command or full queue)");
    return false;
  }
  queue_.push_back(command + "\r\n");
  if (save) queue_.push_back("save\r\n");
  return true;
}
void LD1125H::loop() {
  const uint32_t now = millis();
  if ((!line_.empty() || discard_line_) && uint32_t(now - last_rx_) > 30000) {
    line_.clear();
    discard_line_ = false;
  }
  // Bound work per loop so continuous diagnostic output cannot starve ESPHome.
  for (size_t count = 0; count < 512 && this->available(); count++) {
    uint8_t byte;
    if (!this->read_byte(&byte)) break;
    last_rx_ = millis();
    if (byte == '\n') {
      if (!discard_line_ && !line_.empty()) this->parse_line_(trim(line_));
      line_.clear();
      discard_line_ = false;
    } else if (!discard_line_) {
      if (byte == '\r') continue;
      if (byte < 32 || byte > 126 || line_.size() >= 255) {
        line_.clear();
        discard_line_ = true;
      } else line_.push_back(char(byte));
    }
  }
  if (!queue_.empty() && uint32_t(now - last_tx_) >= 100) {
    const auto command = queue_.front();
    queue_.pop_front();
    this->write_array(reinterpret_cast<const uint8_t *>(command.data()), command.size());
    if (command == "save\r\n") {
      this->set_timeout("save_ack", 5000, []() {
        ESP_LOGW(TAG, "Save acknowledgement not received; persistence unconfirmed");
      });
    }
    last_tx_ = now;
    ESP_LOGD(TAG, "TX %s", command.c_str());
  }
}
void LD1125H::pulse_(binary_sensor::BinarySensor *entity) {
  if (entity == nullptr) return;
  entity->publish_state(true);
  entity->publish_state(false);
}
void LD1125H::parse_line_(const std::string &line) {
  ESP_LOGV(TAG, "RX %s", line.c_str());
  if (line == "all vars have been saved") {
    this->cancel_timeout("save_ack");
    ESP_LOGD(TAG, "Radar settings saved");
    return;
  }
  const bool moving = line.compare(0, 4, "mov,") == 0;
  const bool stationary = line.compare(0, 4, "occ,") == 0;
  if (moving || stationary) {
    const auto fields = trim(line.substr(4));
    if (fields.compare(0, 4, "dis=") != 0) return;
    const auto comma = fields.find(',');
    float distance, strength;
    if (!parse_float(trim(fields.substr(4, comma == std::string::npos ? comma : comma - 4)), distance) || distance < 0) return;
    bool has_strength = comma != std::string::npos;
    if (has_strength) {
      const auto field = trim(fields.substr(comma + 1));
      if (field.compare(0, 4, "str=") != 0 || !parse_float(trim(field.substr(4)), strength) || strength < 0) return;
    }
    if (distance_ != nullptr) distance_->publish_state(distance);
    // Missing strength is normal outside test mode; invalidate only on transition.
    if (signal_strength_ != nullptr && (has_strength || strength_available_))
      signal_strength_->publish_state(has_strength ? strength : NAN);
    strength_available_ = has_strength;
    pulse_(target_);
    pulse_(moving ? movement_ : stationary_);
    return;
  }
  const auto separator = line.find(" is ");
  if (separator != std::string::npos) {
    const auto key = line.substr(0, separator);
    float value;
    if (!parse_float(trim(line.substr(separator + 4)), value) || value < 0) return;
    for (auto *entity : numbers_)
      if (entity->command() == key) entity->publish_state(value);
    if (key == "test_mode" && test_mode_ != nullptr && (value == 0 || value == 1)) test_mode_->publish_state(value == 1);
  } else if (firmware_version_ != nullptr &&
             (line.compare(0, 6, "BW200M") == 0 || line.compare(0, 7, "RKB1125") == 0 || line.compare(0, 7, "LD1125H") == 0)) {
    firmware_version_->publish_state(line);
  }
}
}  // namespace esphome::hi_link_ld1125h
