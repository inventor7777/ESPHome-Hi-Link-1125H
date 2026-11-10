#pragma once
#include <string>
#include <vector>
namespace esphome { namespace event {
class Event {
 public:
  std::vector<std::string> events;
  void trigger(const char *type) { events.emplace_back(type); }
};
} }
