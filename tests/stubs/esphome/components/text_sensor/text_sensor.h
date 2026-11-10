#pragma once
#include <string>
namespace esphome { namespace text_sensor {
class TextSensor {
 public:
  std::string state;
  bool has_state() const { return has_state_; }
  void publish_state(const std::string &value) { state = value; has_state_ = true; }
 protected:
  bool has_state_{false};
};
} }
