#pragma once
#include "esphome_test.h"
#include <optional>
#include <algorithm>
namespace esphome {
namespace fan {
class FanTraits {
 public:
  int count; std::vector<const char *> presets;
  FanTraits(bool,bool,bool,int n):count(n){}
  const auto &supported_preset_modes() const {return presets;}
};
struct FanRestoreState {
 static constexpr uint8_t NO_PRESET=255;
 bool state{}; int speed{}; uint8_t preset_mode{NO_PRESET};
};
class Fan;
class FanCall {
 public:
  FanCall()=default;
  explicit FanCall(Fan &fan):parent_(&fan){}
  std::optional<bool> state;
  std::optional<int> speed;
  std::optional<std::string> preset;
  auto get_state() const {return state;}
  auto get_speed() const {return speed;}
  bool has_preset_mode() const {return preset.has_value();}
  const char *get_preset_mode() const {return preset->c_str();}
  FanCall &set_state(bool v){state=v;return *this;}
  FanCall &set_speed(int v){speed=v;return *this;}
  FanCall &set_preset_mode(const char *v){preset=v;return *this;}
  void perform();
 protected: Fan *parent_{nullptr};
};
class Fan {
 public:
  bool state{}; int speed{}; std::string preset;
  optional<FanRestoreState> test_restore;
  virtual FanTraits get_traits()=0;
  void publish_state(){}
  FanCall make_call(){return FanCall(*this);}
  void set_supported_preset_modes(std::initializer_list<const char *> modes){modes_=modes;}
 protected:
  friend FanCall;
  virtual void control(const FanCall&)=0;
  optional<FanRestoreState> restore_state_(){return test_restore;}
  void wire_preset_modes_(FanTraits &t){t.presets=modes_;}
  void clear_preset_mode_(){preset.clear();}
  void set_preset_mode_(const char *v){preset=v;}
  std::vector<const char *> modes_;
};
inline void FanCall::perform(){
 if(speed.has_value()){speed=std::clamp(*speed,1,parent_->get_traits().count);preset.reset();}
 parent_->control(*this);
}
} }
