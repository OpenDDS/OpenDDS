#ifndef BENCH_UTIL_HEADER
#define BENCH_UTIL_HEADER

#include <string>
#include <vector>

#include <ace/ace_wchar.h> // For ACE_TCHAR
#include <ace/Default_Constants.h> // For ACE_DIRECTORY_SEPARATOR_*
#include <ace/SString.h>

#include "Bench_Common_Export.h"

namespace Bench {

// Argument Parsing
std::string Bench_Common_Export get_option_argument(int& i, int argc, ACE_TCHAR* argv[]);
int Bench_Common_Export get_option_argument_int(int& i, int argc, ACE_TCHAR* argv[]);
unsigned Bench_Common_Export get_option_argument_uint(int& i, int argc, ACE_TCHAR* argv[]);

// Filesytem
std::string Bench_Common_Export join_path(const std::string& arg);
template <typename... Args>
std::string join_path(const std::string& arg, Args... args) {
  return arg + (arg.back() == ACE_DIRECTORY_SEPARATOR_CHAR_A ? "" : ACE_DIRECTORY_SEPARATOR_STR_A) + join_path(args...);
}

std::string Bench_Common_Export create_temp_dir(const std::string& prefix);
std::vector<std::string> Bench_Common_Export get_dir_contents(const std::string& path);
bool Bench_Common_Export file_exists(const std::string& path);

/**
 * Get Current UTC Time in ISO8601
 */
std::string Bench_Common_Export iso8601();

}

#endif
