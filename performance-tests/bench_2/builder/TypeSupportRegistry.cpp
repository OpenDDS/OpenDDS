#include "TypeSupportRegistry.h"

#include <iostream>
#include <sstream>

namespace Builder {

std::mutex TypeSupportRegistry::s_mutex;
TypeSupportRegistry::type_support_registry_map TypeSupportRegistry::s_registry_map;

bool TypeSupportRegistry::register_type_support(const std::string& name, DDS::TypeSupport* ts) {
  std::unique_lock<std::mutex> lock(s_mutex);
  bool result = false;

  auto it = s_registry_map.find(name);
  if (it == s_registry_map.end()) {
    s_registry_map[name] = ts;
    result = true;
  }
  return result;
}

DDS::TypeSupport_var TypeSupportRegistry::get_type_support(const std::string& name) {
  std::unique_lock<std::mutex> lock(s_mutex);
  DDS::TypeSupport_var result = DDS::TypeSupport::_nil();

  auto it = s_registry_map.find(name);
  if (it != s_registry_map.end()) {
    result = it->second;
  }
  return result;
}

std::vector<std::string> TypeSupportRegistry::get_type_names() {
  std::vector<std::string> result;
  std::unique_lock<std::mutex> lock(s_mutex);
  for (auto it = s_registry_map.begin(); it != s_registry_map.end(); ++it) {
    result.push_back(it->first);
  }
  return result;
}

TypeSupportRegistry::TypeSupportRegistration::TypeSupportRegistration(DDS::TypeSupport* ts) {
  if (ts) {
    CORBA::String_var type_name = ts->get_type_name();
    Log::log() << "TypeSupportRegistration created for name '" << type_name << "'" << std::endl;
    if (!register_type_support(type_name.in(), ts)) {
      std::stringstream ss;
      ss << "unable to register type support with name '" << type_name << "'" << std::flush;
      throw std::runtime_error(ss.str());
    }
  } else {
    throw std::runtime_error("null type support passed to registration");
  }
}

}

