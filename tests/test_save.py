"""Run with Python 3 and clang++: python tests/test_save.py."""

from pathlib import Path
import subprocess
import tempfile
import unittest


SOURCE = Path(__file__).resolve().parents[1] / "components/hi_link_ld1125h"
STUBS = r"""
#pragma once
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace esphome {
template<class T> using optional = std::optional<T>;
inline uint32_t clock_ms = 100;
inline uint32_t millis() { return clock_ms; }
inline std::vector<std::string> logs;
class Component {
 public:
  std::map<std::string, std::function<void()>> timers;
  virtual void setup() {} virtual void loop() {} virtual void dump_config() {}
  void set_timeout(const char *name, uint32_t, std::function<void()> f) { timers[name] = f; }
  bool cancel_timeout(const char *name) { return timers.erase(name) != 0; }
};
namespace uart { class UARTDevice {
 public:
  std::deque<uint8_t> rx;
  std::string tx;
  bool available() { return !rx.empty(); }
  bool read_byte(uint8_t *b) { *b = rx.front(); rx.pop_front(); return true; }
  void write_array(const uint8_t *b, size_t n) { tx.append((const char *) b, n); }
}; }
namespace sensor { class Sensor { public: void publish_state(float) {} }; }
namespace binary_sensor { class BinarySensor { public: void publish_state(bool) {} }; }
namespace number { class Number { public: void publish_state(float) {} virtual void control(float) = 0; }; }
namespace switch_ { class Switch { public:
  optional<bool> initial{false};
  optional<bool> get_initial_state_with_restore_mode() { return initial; }
  void control(bool x) { write_state(x); }
  void publish_state(bool) {} virtual void write_state(bool) = 0;
}; }
namespace button { class Button { public: virtual void press_action() = 0; }; }
namespace text_sensor { class TextSensor { public: void publish_state(const std::string &) {} }; }
}
#define ESP_LOGW(tag, format, ...) esphome::logs.emplace_back(format)
#define ESP_LOGD(tag, format, ...) esphome::logs.emplace_back(format)
#define ESP_LOGV(...) ((void)0)
#define ESP_LOGCONFIG(...) ((void)0)
"""
TEST = r"""
#include "hi_link_ld1125h.h"
#include <cassert>
using namespace esphome;
using namespace esphome::hi_link_ld1125h;
class TestRadar : public LD1125H { public: using LD1125H::queue_; };
class TestNumber : public RadarNumber { public: using RadarNumber::RadarNumber; using RadarNumber::control; };
class TestButton : public CommandButton { public: using CommandButton::CommandButton; using CommandButton::press_action; };
int main() {
  LD1125H radar;
  auto receive = [&](const std::string &s) {
    for (char c : s) radar.rx.push_back(c);
    radar.loop();
  };
  assert(radar.send_command("get_all"));
  assert(radar.send_command("save"));
  assert(radar.timers.count("save_ack") == 0);
  radar.loop();
  assert(radar.tx == "get_all\r\n");
  assert(radar.timers.count("save_ack") == 0);
  clock_ms += 100;
  radar.loop();
  assert(radar.tx == "get_all\r\nsave\r\n");
  assert(radar.timers.count("save_ack") == 1);
  receive("received message: save\r\n");
  assert(radar.timers.count("save_ack") == 1);
  receive("all vars have been ");
  assert(radar.timers.count("save_ack") == 1);
  receive("saved\r\n");
  assert(radar.timers.count("save_ack") == 0);
  assert(logs.back() == "Radar settings saved");
  assert(radar.send_command("save"));
  clock_ms += 100;
  radar.loop();
  auto timeout = radar.timers.at("save_ack");
  radar.timers.erase("save_ack");
  timeout();
  assert(logs.back() == "Save acknowledgement not received; persistence unconfirmed");
  auto sent = radar.tx;
  clock_ms += 100;
  receive("all vars have been saved\r\n");
  assert(logs.back() == "Radar settings saved");
  assert(radar.tx == sent);  // Missing replies must never trigger another flash write.

  TestRadar automatic;
  TestNumber distance(&automatic, "rmax", 1);
  TestNumber threshold(&automatic, "mth1", 0);
  distance.control(3.5f);
  threshold.control(80);
  assert((automatic.queue_ == std::deque<std::string>{"rmax=3.5\r\n", "save\r\n", "mth1=80\r\n", "save\r\n"}));
  clock_ms += 100;
  automatic.loop();
  assert(automatic.tx == "rmax=3.5\r\n");
  assert(automatic.timers.count("save_ack") == 0);
  clock_ms += 100;
  automatic.loop();
  assert(automatic.tx == "rmax=3.5\r\nsave\r\n");
  assert(automatic.timers.count("save_ack") == 1);

  TestRadar settings;
  TestMode mode(&settings);
  mode.control(true);
  mode.control(false);
  TestButton reset(&settings, "initial");
  reset.press_action();
  assert((settings.queue_ == std::deque<std::string>{"test_mode=1\r\n", "save\r\n", "test_mode=0\r\n", "save\r\n", "initial\r\n", "save\r\n"}));

  TestRadar full;
  for (int i = 0; i < 15; i++) assert(full.send_command("get_all"));
  assert(!full.send_command("rmax=4", true));
  assert(full.queue_.size() == 15);
  assert(full.send_command("get_all"));
  assert(full.queue_.size() == 16);

  for (optional<bool> initial : {optional<bool>{}, optional<bool>{false}, optional<bool>{true}}) {
    TestRadar startup;
    TestMode boot_mode(&startup);
    boot_mode.initial = initial;
    startup.set_test_mode(&boot_mode);
    startup.setup();
    startup.timers.at("read_settings")();
    std::deque<std::string> expected;
    if (initial.has_value()) expected.push_back(*initial ? "test_mode=1\r\n" : "test_mode=0\r\n");
    expected.push_back("get_all\r\n");
    assert(startup.queue_ == expected);
    startup.timers.at("retry_read_settings")();
    expected.push_back("get_all\r\n");
    assert(startup.queue_ == expected);
  }
}
"""


class SaveTests(unittest.TestCase):
    def test_save_acknowledgement(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "stubs.h").write_text(STUBS)
            headers = ["core/component.h", "core/hal.h", "core/log.h"]
            headers += [f"components/{name}/{name}.h" for name in (
                "uart", "sensor", "binary_sensor", "number", "switch", "button", "text_sensor"
            )]
            for header in headers:
                path = root / "esphome" / header
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('#include "stubs.h"\n')
            (root / "test.cpp").write_text(TEST)
            subprocess.run([
                "clang++", "-std=c++17", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                f"-I{root}", f"-I{SOURCE}", str(root / "test.cpp"),
                str(SOURCE / "hi_link_ld1125h.cpp"), "-o", str(root / "test"),
            ], check=True)
            subprocess.run([str(root / "test")], check=True)


if __name__ == "__main__":
    unittest.main()
