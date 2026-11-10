#pragma once
namespace esphome { namespace network {
inline bool test_connected = false, test_disabled = false;
inline bool is_connected() { return test_connected; }
inline bool is_disabled() { return test_disabled; }
} }
