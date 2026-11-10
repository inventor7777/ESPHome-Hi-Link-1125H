#pragma once
namespace esphome { namespace api {
struct APIServer {
  bool connected{false}, state_subscription{false};
  bool is_connected_with_state_subscription() const { return connected && state_subscription; }
};
inline APIServer *global_api_server = nullptr;
} }
