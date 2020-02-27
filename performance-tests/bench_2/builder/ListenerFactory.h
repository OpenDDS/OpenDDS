#pragma once

#include "Common.h"

#include <map>
#include <functional>
#include <mutex>
#include <iostream>
#include <sstream>

namespace Builder {

template <typename ListenerType>
class ListenerFactory {
public:
  using Listener = ListenerType;
  using Listener_var = typename ListenerType::_var_type;

  using listener_factory = std::function<Listener_var(const Builder::PropertySeq&)>;
  using listener_factory_map = std::map<std::string, listener_factory>;

  static bool register_listener_factory(const std::string& name, const listener_factory& factory) {
    std::unique_lock<std::mutex> lock(s_mutex);
    bool result = false;

    auto it = s_factory_map.find(name);
    if (it == s_factory_map.end()) {
      s_factory_map[name] = factory;
      result = true;
    }
    return result;
  }

  static Listener_var create_listener(const std::string& name) {
    std::unique_lock<std::mutex> lock(s_mutex);
    Listener_var result = Listener::_nil();

    auto it = s_factory_map.find(name);
    if (it != s_factory_map.end()) {
      result = (it->second)(Builder::PropertySeq());
    }
    return result;
  }

  static Listener_var create_listener(const std::string& name, const Builder::PropertySeq& properties) {
    std::unique_lock<std::mutex> lock(s_mutex);
    Listener_var result = Listener::_nil();

    auto it = s_factory_map.find(name);
    if (it != s_factory_map.end()) {
      result = (it->second)(properties);
    }
    return result;
  }

  class Registration {
  public:
    Registration(const std::string& name, const listener_factory& factory) {
      Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
      if (!register_listener_factory(name, factory)) {
        std::stringstream ss;
        ss << "unable to register listener factory with name '" << name << "'" << std::flush;
        throw std::runtime_error(ss.str());
      }
    }
  };

protected:
  static std::mutex s_mutex;
  static listener_factory_map s_factory_map;
};

template <typename ListenerType>
std::mutex ListenerFactory<ListenerType>::s_mutex;

template <typename ListenerType>
typename ListenerFactory<ListenerType>::listener_factory_map ListenerFactory<ListenerType>::s_factory_map;

}
