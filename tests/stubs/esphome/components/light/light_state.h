#pragma once
namespace esphome { namespace light {
class LightState {
 public:
  bool state{false};
  unsigned calls{0};
  class Call {
   public:
    explicit Call(LightState *light) : light_(light) {}
    Call &set_state(bool state) { state_ = state; return *this; }
    void perform() { light_->state = state_; ++light_->calls; }
   private:
    LightState *light_;
    bool state_{!light_->state};
  };
  Call toggle() { return Call(this); }
  Call make_call() { return Call(this); }
};
} }
