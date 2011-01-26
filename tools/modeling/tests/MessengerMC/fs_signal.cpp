#include "fs_signal.h"
#include <fstream>
#include <ace/OS_NS_unistd.h>

FileSystemSignal::FileSystemSignal(int id) : _id(id) {
  sprintf(_file_name, "fs_signal_%d", _id);
}

void FileSystemSignal::signal() {
  std::ofstream ofs(_file_name);
  ofs << "hello" << std::endl;
}

void FileSystemSignal::wait_timeout(int dur_sec) {
  while (!exists() && dur_sec > 0) {
    ACE_OS::sleep(1);
    dur_sec -= 1;
  }
}

void FileSystemSignal::wait_forever() {
  while (!exists()) {
    ACE_OS::sleep(1);
  }
}

bool FileSystemSignal::exists() {
  std::ifstream ifs(_file_name);
  return !!ifs;
}

