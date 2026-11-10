#include "hi_link_ld1125h/hi_link_ld1125h.h"
#include "esphome_test.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace esphome;
using namespace esphome::hi_link_ld1125h;
class Radar : public LD1125H {
 public:
  using LD1125H::parse_line_;
  void feed(const std::string &s) { rx.insert(rx.end(), s.begin(), s.end()); loop(); }
};
class Setting : public RadarNumber { public: using RadarNumber::RadarNumber; using RadarNumber::control; };
class TestSwitch : public TestMode { public: using TestMode::TestMode; using TestMode::write_state; };
int main() {
  Radar radar;
  sensor::Sensor distance, strength;
  text_sensor::TextSensor version;
  binary_sensor::BinarySensor target, movement, stationary;
  radar.set_target(&target); radar.set_movement(&movement); radar.set_stationary(&stationary);
  Setting threshold(&radar, "mth1_mov", 0), legacy(&radar, "mth1", 0), range(&radar, "rmax", 1);
  TestSwitch test(&radar);
  radar.set_distance(&distance); radar.set_signal_strength(&strength);
  radar.set_firmware_version(&version); radar.set_test_mode(&test);
  radar.add_number(&threshold); radar.add_number(&legacy); radar.add_number(&range);
  radar.setup();
  radar.test_timeouts["read_settings"]();
  test_now = 100; radar.loop();
  assert(std::string(radar.tx.back().begin(), radar.tx.back().end()) == "get_all\r\n");
  test_now += 100; radar.loop();
  assert(std::string(radar.tx.back().begin(), radar.tx.back().end()) == "VER\r\n");
  radar.feed("mov, di");
  assert(distance.state == 0);
  radar.feed("s=2.34, str=80.5\r\n");
  assert(std::abs(distance.state - 2.34f) < 0.001 && strength.state == 80.5f);
  assert(movement.history == std::vector<bool>({false, true, false}));
  radar.feed("occ,dis=1.5\n");
  assert(stationary.history == std::vector<bool>({false, true, false}));
  assert(target.history == std::vector<bool>({false, true, false, true, false}));
  assert(distance.state == 1.5f && std::isnan(strength.state));
  const auto invalidations = strength.history.size();
  for (int i = 0; i < 100; ++i) radar.parse_line_("occ, dis=1.5");
  assert(strength.history.size() == invalidations);
  radar.parse_line_("occ, dis=1.5, str=42");
  assert(strength.state == 42);
  radar.parse_line_("occ, dis=1.5");
  assert(std::isnan(strength.state) && strength.history.size() == invalidations + 2);
  for (const auto &line : {"mov,", "occ, dis=", "mov, dis=nan", "occ, dis=inf", "mov, dis=-1",
       "mov, dis=3junk", "mov, dis=3,str=bad", "mov, dis=3,extra=3", "m", "received message: get_all"})
    radar.parse_line_(line);
  assert(distance.state == 1.5f);
  threshold.control(123);
  assert(threshold.state == 0); // No optimistic success.
  test_now += 200; radar.loop();
  assert(std::string(radar.tx.back().begin(), radar.tx.back().end()) == "mth1_mov=123\r\n");
  radar.feed("mth1_mov is 123\r\nmth1 is 60\r\nrmax is 8.00\r\n");
  assert(threshold.state == 123 && legacy.state == 60 && range.state == 8);
  radar.feed("mth1_mov is NaN\r\nmth1_mov is 42junk\r\n");
  assert(threshold.state == 123);
  test.write_state(true);
  assert(!test.state);
  radar.feed("test_mode is 1\r\n"); assert(test.state);
  radar.feed("test_mode is 3\r\n"); assert(test.state);
  radar.feed("BW200M V1.0\r\n"); assert(version.state == "BW200M V1.0");
  radar.feed("RKB1125H BW1800M 20230412 V4.1\r\n");
  assert(version.state == "RKB1125H BW1800M 20230412 V4.1");
  radar.feed("RKB1125FS2 BW800M V1.0.0 20250421\r\n");
  assert(version.state == "RKB1125FS2 BW800M V1.0.0 20250421");
  radar.feed(std::string(300, 'x') + "\nmov, dis=4\r\n"); assert(distance.state == 4);
  radar.feed(std::string("\0", 1) + "mov, dis=5\n"); assert(distance.state == 4);
  radar.feed("mov, dis="); test_now += 15000; radar.loop();
  radar.feed("6\r\n"); assert(distance.state == 6); // Permissive split receive workaround.
  radar.feed("mov, dis="); test_now += 30001; radar.loop();
  radar.feed("7\r\n"); assert(distance.state == 6);
  radar.feed("mov, dis=8\n"); assert(distance.state == 8);
  assert(!radar.send_command("save\ninitial"));
  assert(!radar.send_command("")); assert(!radar.send_command(std::string(97, 'a')));
  for (int i = 0; i < 16; i++) radar.send_command("get_all");
  assert(!radar.send_command("get_all"));
  {
    Radar boot;
    text_sensor::TextSensor boot_version;
    boot.set_firmware_version(&boot_version);
    boot.setup();
    boot.test_timeouts["retry_read_settings"]();
    test_now += 100; boot.loop();
    assert(std::string(boot.tx.back().begin(), boot.tx.back().end()) == "get_all\r\n");
    test_now += 100; boot.loop();
    assert(std::string(boot.tx.back().begin(), boot.tx.back().end()) == "VER\r\n");
  }
  {
    Radar fresh;
    sensor::Sensor unknown_strength;
    fresh.set_signal_strength(&unknown_strength);
    for (int i = 0; i < 100; ++i) fresh.parse_line_("occ, dis=1.00");
    assert(unknown_strength.history.empty());
  }
  // No periodic receive traffic is required: an empty room legitimately goes quiet.
  std::cout << "LD1125H framing, strict parsing, settings readback, queue and timeout checks passed\n";
}
