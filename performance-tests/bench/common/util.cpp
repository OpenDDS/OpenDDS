#include "util.h"

#include <dds/DCPS/DirentWrapper.h>

#include <ace/Lib_Find.h> // For ACE::get_temp_dir
#include <ace/OS_NS_string.h> // For ACE_OS::strcpy
#include <ace/OS_NS_sys_stat.h> // For ACE_OS::mkdir and ACE_OS::stat
#include <ace/OS_NS_stdlib.h> // For ACE_OS::mktemp

#include <iostream>
#include <exception>
#include <sstream>
#include <ctime>

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
    value = static_cast<int>(std::stoll(get_option_argument(i, argc, argv)));
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
    value = static_cast<unsigned>(std::stoull(get_option_argument(i, argc, argv)));
  } catch (const std::exception&) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i])
      << " requires an argument that's a valid positive number" << std::endl;
    throw 1;
  }
  return value;
}

std::string& string_replace(std::string& input, const std::string& oldstr, const std::string& newstr)
{
  size_t pos = input.find(oldstr);
  while (pos != std::string::npos) {
    input.replace(pos, oldstr.size(), newstr);
    pos = input.find(oldstr);
  }
  return input;
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

std::string iso8601(const std::chrono::system_clock::time_point& tp)
{
  using namespace std::chrono;
  std::stringstream ss;
  const std::time_t now = system_clock::to_time_t(tp);
  char buf[sizeof "2011-10-08T07:07:09Z"]; // longest possible for UTC times (other zones add offset suffix)
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
#ifdef ACE_HAS_TCHAR_DIRENT
      rv.push_back(ACE_TEXT_ALWAYS_CHAR(entry->d_name));
#else
      rv.push_back(entry->d_name);
#endif
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

uint32_t one_at_a_time_hash(const uint8_t* key, size_t length)
{
  size_t i = 0;
  uint32_t hash = 0;
  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

void update_stats_for_tags(std::unordered_map<std::string, uint64_t>& stats,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const Builder::ConstPropertyIndex& prop)
{
  for (CORBA::ULong i = 0; i < reported_tags.length(); ++i) {
    const std::string tag(reported_tags[i]);
    if (input_tags.find(tag) != input_tags.end()) {
      if (stats.find(tag) == stats.end()) {
        stats[tag] = prop->value.ull_prop();
      } else {
        stats[tag] += prop->value.ull_prop();
      }
    }
  }
}

void update_details_for_tags(std::unordered_map<std::string, std::string>& details,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const std::string& detail)
{
  for (CORBA::ULong i = 0; i < reported_tags.length(); ++i) {
    const std::string tag(reported_tags[i]);
    if (input_tags.find(tag) != input_tags.end()) {
      details[tag] += detail;
    }
  }
}

void consolidate_tagged_stats(std::unordered_map<std::string, Bench::SimpleStatBlock>& stats,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const Bench::ConstPropertyStatBlock& data)
{
  for (CORBA::ULong i = 0; i < reported_tags.length(); ++i) {
    const std::string tag(reported_tags[i]);
    if (input_tags.find(tag) != input_tags.end()) {
      stats[tag] = consolidate(stats[tag], data.to_simple_stat_block());
    }
  }
}
}
