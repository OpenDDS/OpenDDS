#include "util.h"

#include <iostream>
#include <exception>

#include <ace/Lib_Find.h> // For ACE::get_temp_dir
#include <ace/OS_NS_string.h> // For ACE_OS::strcpy
#include <ace/OS_NS_sys_stat.h> // For ACE_OS::mkdir
#include <ace/OS_NS_stdlib.h> // For ACE_OS::mktemp

namespace Bench {

std::string get_option_argument(int& i, int argc, ACE_TCHAR* argv[])
{
  if (i == argc - 1) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i])<< " requires an argument" << std::endl;
    throw int{1};
  }
  return ACE_TEXT_ALWAYS_CHAR(argv[++i]);
}

int get_option_argument_int(int& i, int argc, ACE_TCHAR* argv[])
{
  int value;
  try {
    value = std::stoll(get_option_argument(i, argc, argv));
  } catch (const std::exception&) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i]) << " requires an argument that's a valid number" << std::endl;
    throw 1;
  }
  return value;
}

unsigned get_option_argument_uint(int& i, int argc, ACE_TCHAR* argv[])
{
  unsigned value;
  try {
    value = std::stoull(get_option_argument(i, argc, argv));
  } catch (const std::exception&) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i])
      << " requires an argument that's a valid positive number" << std::endl;
    throw 1;
  }
  return value;
}

std::string create_temp_dir(const std::string& prefix)
{
  // Create Template for mktemp
  char buffer[PATH_MAX];
  if (ACE::get_temp_dir(&buffer[0], PATH_MAX) == -1) {
    return "";
  }
  ACE_OS::strcpy(
    &buffer[0],
    join_path(
      ACE_TEXT_ALWAYS_CHAR(&buffer[0]),
      (prefix + "_XXXXXX")).c_str());

  // Fill the template and create the directory
  if (!ACE_OS::mktemp(buffer)) {
    return "";
  }
  if (ACE_OS::mkdir(buffer, S_IRWXU) == -1) {
    return "";
  }

  return buffer;
}

}
