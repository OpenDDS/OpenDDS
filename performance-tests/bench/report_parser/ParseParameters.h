#ifndef PARSE_PARAMETERS_HEADER
#define PARSE_PARAMETERS_HEADER

#include <string>
#include <unordered_set>

namespace Bench {

struct ParseParameters
{
  std::unordered_set<std::string> tags;
  std::unordered_set<std::string> stats;
  std::unordered_set<std::string> values;
};

} // namespace Bench

#endif
