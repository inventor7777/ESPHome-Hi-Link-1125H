#pragma once
#include "esphome_test.h"
#include <optional>
#include <functional>
#include <cmath>
namespace esphome { using std::optional; namespace climate {
enum ClimateMode { CLIMATE_MODE_OFF, CLIMATE_MODE_HEAT, CLIMATE_MODE_FAN_ONLY };
enum ClimateAction { CLIMATE_ACTION_OFF, CLIMATE_ACTION_IDLE, CLIMATE_ACTION_HEATING, CLIMATE_ACTION_FAN };
enum ClimateSwingMode { CLIMATE_SWING_OFF, CLIMATE_SWING_HORIZONTAL };
enum ClimateFanMode { CLIMATE_FAN_AUTO, CLIMATE_FAN_LOW, CLIMATE_FAN_MEDIUM, CLIMATE_FAN_HIGH };
constexpr unsigned CLIMATE_SUPPORTS_CURRENT_TEMPERATURE=1,CLIMATE_SUPPORTS_ACTION=2;
class ClimateTraits { public:
 std::vector<ClimateFanMode> fan_modes;
 void set_feature_flags(unsigned) {}
 void set_supported_modes(std::initializer_list<ClimateMode>) {}
 void set_supported_fan_modes(std::initializer_list<ClimateFanMode> modes) { fan_modes.assign(modes); }
 void set_supported_swing_modes(std::initializer_list<ClimateSwingMode>) {}
 void set_visual_min_temperature(float) {} void set_visual_max_temperature(float) {} void set_visual_temperature_step(float) {}
};
class Climate;
class ClimateCall;
struct ClimateDeviceRestoreState {
 ClimateMode mode=CLIMATE_MODE_OFF;
 ClimateFanMode fan_mode=CLIMATE_FAN_LOW;
 ClimateSwingMode swing_mode=CLIMATE_SWING_OFF;
 float target_temperature=20;
 std::string preset="Auto";
 ClimateCall to_call(Climate *climate);
 void apply(Climate *climate);
};
class ClimateCall { public:
 explicit ClimateCall(Climate *p):p(p) {}
 ClimateCall &set_mode(ClimateMode v) { m=v;return *this; }
 ClimateCall &set_fan_mode(ClimateFanMode v) { f=v;return *this; }
 ClimateCall &set_swing_mode(ClimateSwingMode v) { swing=v;return *this; }
 const std::optional<ClimateSwingMode> &get_swing_mode() const { return swing; }
 ClimateCall &set_target_temperature(float v) { t=v;return *this; }
 ClimateCall &set_custom_preset(const std::string &v) { preset=v;return *this; }
 const std::optional<ClimateMode> &get_mode() const { return m; }
 const std::optional<ClimateFanMode> &get_fan_mode() const { return f; }
 const std::optional<float> &get_target_temperature() const { return t; }
 bool has_custom_preset() const { return !preset.empty(); }
 const std::string &get_custom_preset() const { return preset; }
 void perform();
 Climate *p; std::optional<ClimateMode> m;std::optional<ClimateFanMode> f;std::optional<float> t;std::optional<ClimateSwingMode> swing;std::string preset;
};
class Climate { public:
 virtual ~Climate()=default;
 ClimateMode mode=CLIMATE_MODE_OFF;ClimateAction action=CLIMATE_ACTION_OFF;
 std::optional<ClimateFanMode> fan_mode=CLIMATE_FAN_LOW;
 ClimateSwingMode swing_mode=CLIMATE_SWING_OFF;
 float current_temperature=NAN,target_temperature=20;
 ClimateCall make_call() { return ClimateCall(this); }
 void add_on_state_callback(std::function<void(Climate &)> cb) { callbacks.push_back(cb); }
 void publish_state() {
  for(auto &cb:callbacks) cb(*this);
  if (test_preferences_initialized) test_restore=ClimateDeviceRestoreState{mode,fan_mode.value_or(CLIMATE_FAN_LOW),swing_mode,target_temperature,preset};
 }
 std::optional<ClimateDeviceRestoreState> test_restore;
 void set_supported_custom_presets(std::initializer_list<const char *>) {}
 std::string get_custom_preset() const { return preset; }
 protected:
 friend class ClimateCall;
 friend struct ClimateDeviceRestoreState;
 std::optional<ClimateDeviceRestoreState> restore_state_() { test_preferences_initialized=true;return test_restore; }
 bool test_preferences_initialized=false;
 virtual ClimateTraits traits() { return {}; }
 virtual void control(const ClimateCall &call)=0;
 void set_custom_preset_(const char *v) { preset=v; }
 std::vector<std::function<void(Climate &)>> callbacks;std::string preset;
};
inline ClimateCall ClimateDeviceRestoreState::to_call(Climate *c) {
 return c->make_call().set_mode(mode).set_fan_mode(fan_mode).set_swing_mode(swing_mode).set_target_temperature(target_temperature).set_custom_preset(preset);
}
inline void ClimateDeviceRestoreState::apply(Climate *c) {
 c->mode=mode;c->fan_mode=fan_mode;c->swing_mode=swing_mode;c->target_temperature=target_temperature;c->preset=preset;
}
inline void ClimateCall::perform() { p->control(*this); }
} }
