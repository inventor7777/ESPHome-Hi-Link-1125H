#pragma once
#include <cstdint>
#include <cstddef>
#include <deque>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <optional>
namespace esphome {
using std::optional;
inline uint32_t test_now = 0;
inline uint32_t millis() { return test_now; }
namespace setup_priority { constexpr float DATA = 0; }
class Component { public:
 virtual void setup() {} virtual void loop() {} virtual void dump_config() {} virtual void on_shutdown() {}
 virtual float get_setup_priority() const { return 0; }
 template<class F> void set_timeout(const char *name, uint32_t, F callback) { test_timeouts[name]=callback; }
 bool cancel_timeout(const char *name) { return test_timeouts.erase(name) != 0; }
 std::map<std::string, std::function<void()>> test_timeouts;
 std::map<std::string, std::function<void()>> test_intervals;
 template<class F> void set_interval(const char *name, uint32_t, F callback) { test_intervals[name]=callback; }
};
inline std::string format_hex_pretty(const uint8_t *, size_t) { return {}; }
namespace uart { class UARTDevice { public:
 std::deque<uint8_t> rx;
 std::vector<std::vector<uint8_t>> tx;
 size_t available() const { return rx.size(); }
 bool read_byte(uint8_t *out) { if(rx.empty()) return false; *out=rx.front();rx.pop_front();++test_now;return true; }
 void write_array(const uint8_t *data,size_t size) { tx.emplace_back(data,data+size); }
 void flush() {}
}; }
namespace sensor { class Sensor { public: float state{}; std::vector<float> history; void publish_state(float s) { state=s; history.push_back(s); } }; }
namespace binary_sensor { class BinarySensor { public: bool state{}; std::vector<bool> history; void publish_state(bool s) { state=s; history.push_back(s); } }; }
namespace switch_ {
enum SwitchRestoreMode { SWITCH_RESTORE_DISABLED, SWITCH_RESTORE_DEFAULT_OFF, SWITCH_RESTORE_DEFAULT_ON, SWITCH_ALWAYS_OFF, SWITCH_ALWAYS_ON };
class Switch { public:
 bool state{};std::optional<bool> test_saved;
 SwitchRestoreMode restore_mode=SWITCH_RESTORE_DISABLED;
 void set_restore_mode(SwitchRestoreMode value) { restore_mode=value; }
 std::optional<bool> get_initial_state_with_restore_mode() {
  if(restore_mode==SWITCH_RESTORE_DISABLED)return {};
  if(restore_mode==SWITCH_ALWAYS_OFF)return false;
  if(restore_mode==SWITCH_ALWAYS_ON)return true;
  return test_saved.value_or(restore_mode==SWITCH_RESTORE_DEFAULT_ON);
 }
 void publish_state(bool s) { state=s;if(restore_mode==SWITCH_RESTORE_DEFAULT_OFF||restore_mode==SWITCH_RESTORE_DEFAULT_ON)test_saved=s; }
 protected: virtual void write_state(bool)=0;
}; }
namespace select { class Select { public: uint32_t get_preference_hash() { return 1; } std::string state; void publish_state(const std::string &s) { state=s; } void publish_state(size_t i) { state=std::to_string(i); } protected: virtual void control(const std::string &) {} virtual void control(size_t) {} }; }
namespace number { class Number { public: float state{}; void publish_state(float s) { state=s; } protected: virtual void control(float)=0; }; }
}
#define ESP_LOGCONFIG(tag, ...) ((void)(tag))
#define ESP_LOGW(tag, ...) ((void)(tag))
#define ESP_LOGV(tag, ...) ((void)(tag))
#define ESP_LOGD(tag, ...) ((void)(tag))
