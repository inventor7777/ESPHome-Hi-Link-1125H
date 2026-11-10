#pragma once
#include <cstdint>
namespace esphome {
class ESPPreferenceObject {
 public:
  template<class T> bool load(T *out) { if (!saved) return false; *out=value; return true; }
  template<class T> bool save(const T *in) { value=*in;saved=true;return true; }
 private:
  uint8_t value{0};bool saved{false};
};
class TestPreferences { public: template<class T> ESPPreferenceObject make_preference(uint32_t) { return {}; } };
inline TestPreferences preferences;
inline TestPreferences *global_preferences=&preferences;
}
