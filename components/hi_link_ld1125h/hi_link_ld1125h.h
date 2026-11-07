#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/button/button.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <deque>
#include <string>
#include <vector>

namespace esphome::hi_link_ld1125h {
class LD1125H;
class RadarNumber : public number::Number {
 public:
  RadarNumber(LD1125H *parent, const char *command, uint8_t decimals)
      : parent_(parent), command_(command), decimals_(decimals) {}
  const std::string &command() const { return command_; }
 protected:
  void control(float value) override;
  LD1125H *parent_;
  std::string command_;
  uint8_t decimals_;
};
class TestMode : public switch_::Switch {
 public:
  explicit TestMode(LD1125H *parent) : parent_(parent) {}
 protected:
  void write_state(bool state) override;
  LD1125H *parent_;
};
class CommandButton : public button::Button {
 public:
  CommandButton(LD1125H *parent, const char *command) : parent_(parent), command_(command) {}
 protected:
  void press_action() override;
  LD1125H *parent_;
  std::string command_;
};
class LD1125H : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  bool send_command(const std::string &command, bool save = false);
  void add_number(RadarNumber *entity) { numbers_.push_back(entity); }
  void set_distance(sensor::Sensor *entity) { distance_ = entity; }
  void set_signal_strength(sensor::Sensor *entity) { signal_strength_ = entity; }
  void set_target(binary_sensor::BinarySensor *entity) { target_ = entity; }
  void set_movement(binary_sensor::BinarySensor *entity) { movement_ = entity; }
  void set_stationary(binary_sensor::BinarySensor *entity) { stationary_ = entity; }
  void set_firmware_version(text_sensor::TextSensor *entity) { firmware_version_ = entity; }
  void set_test_mode(TestMode *entity) { test_mode_ = entity; }
 protected:
  void parse_line_(const std::string &line);
  static void pulse_(binary_sensor::BinarySensor *entity);
  std::string line_;
  bool discard_line_{false};
  bool strength_available_{false};
  uint32_t last_rx_{0}, last_tx_{0};
  std::deque<std::string> queue_;
  std::vector<RadarNumber *> numbers_;
  sensor::Sensor *distance_{nullptr}, *signal_strength_{nullptr};
  binary_sensor::BinarySensor *target_{nullptr}, *movement_{nullptr}, *stationary_{nullptr};
  text_sensor::TextSensor *firmware_version_{nullptr};
  TestMode *test_mode_{nullptr};
};
}  // namespace esphome::hi_link_ld1125h
