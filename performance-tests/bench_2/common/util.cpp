#include "util.h"

#include <iostream>
#include <exception>
#include <sstream>
#include <chrono> // For std::chrono::system_clock
#include <ctime>

#include <ace/Lib_Find.h> // For ACE::get_temp_dir
#include <ace/OS_NS_string.h> // For ACE_OS::strcpy
#include <ace/OS_NS_sys_stat.h> // For ACE_OS::mkdir and ACE_OS::stat
#include <ace/OS_NS_stdlib.h> // For ACE_OS::mktemp
#include <ace/OS_NS_dirent.h> // For ACE_OS::opendir and friends

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

std::string join_path(const std::string& arg) {
  return arg;
}

std::string create_temp_dir(const std::string& prefix)
{
  // Create Template for mktemp
  ACE_TCHAR buffer[PATH_MAX];
  if (ACE::get_temp_dir(&buffer[0], PATH_MAX) == -1) {
    return "";
  }
  ACE_OS::strcpy(
    &buffer[0],
    ACE_TEXT_CHAR_TO_TCHAR(join_path(
      ACE_TEXT_ALWAYS_CHAR(&buffer[0]),
      (prefix + "_XXXXXX")).c_str()));

  // Fill the template and create the directory
  if (!ACE_OS::mktemp(buffer)) {
    return "";
  }
  if (ACE_OS::mkdir(buffer, S_IRWXU) == -1) {
    return "";
  }

  return ACE_TEXT_ALWAYS_CHAR(buffer);
}

std::string iso8601()
{
  using namespace std::chrono;
  std::stringstream ss;
  const std::time_t now = system_clock::to_time_t(system_clock::now());
  char buf[sizeof "2011-10-08T07:07:09Z"];
  std::strftime(buf, sizeof buf, "%FT%TZ", std::gmtime(&now));
  ss << buf;
  return ss.str();
}

std::vector<std::string> get_dir_contents(const std::string& path)
{
  std::vector<std::string> rv;
  ACE_DIR* dir = ACE_OS::opendir(ACE_TEXT_CHAR_TO_TCHAR(path.c_str()));
  if (dir) {
    ACE_DIRENT* entry;
    while ((entry = ACE_OS::readdir(dir))) {
      rv.push_back(entry->d_name);
    }
    ACE_OS::closedir(dir);
  } else {
    std::stringstream ss;
    ss << "getting contents of " << path << ": " << ACE_OS::strerror(errno);
    throw std::runtime_error(ss.str());
  }
  return rv;
}

bool file_exists(const std::string& path)
{
  ACE_stat stat_result;
  if (ACE_OS::stat(path.c_str(), &stat_result) == -1) {
    if (errno != ENOENT) {
      std::stringstream ss;
      ss << "checking if " << path << " exists: " << ACE_OS::strerror(errno);
      throw std::runtime_error(ss.str());
    }
    return false;
  }
  return true;
}

}
