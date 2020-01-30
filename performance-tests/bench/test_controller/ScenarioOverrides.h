#ifndef SCENARIO_OVERRIDES_HEADER
#define SCENARIO_OVERRIDES_HEADER

#include <string>

/**
 * ScenarioOverrides contains information that will be applied to (override) the parsed worker configs
 * which can be helpful for quickly coordinating system-wide timing changes and unique partition names
 */
struct ScenarioOverrides {
  std::string bench_partition_suffix;
  unsigned create_time_delta{0};
  unsigned enable_time_delta{0};
  unsigned start_time_delta{0};
  unsigned stop_time_delta{0};
  unsigned destruction_time_delta{0};
};

#endif
