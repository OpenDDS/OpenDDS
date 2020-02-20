#pragma once

#include "Common.h"
#include "dds/DdsDcpsTopicC.h"

#include <map>
#include <mutex>
#include <vector>

namespace Builder {

class Bench_Builder_Export TypeSupportRegistry {
public:

  using type_support_registry_map = std::map<std::string, DDS::TypeSupport_var>;

  static bool register_type_support(const std::string& name, DDS::TypeSupport* ts);
  static DDS::TypeSupport_var get_type_support(const std::string& name);
  static std::vector<std::string> get_type_names();

  class Bench_Builder_Export TypeSupportRegistration {
  public:
    explicit TypeSupportRegistration(DDS::TypeSupport* ts);
  };

protected:
  static std::mutex s_mutex;
  static type_support_registry_map s_registry_map;
};

}

