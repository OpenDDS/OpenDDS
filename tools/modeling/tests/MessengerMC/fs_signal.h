#ifndef FS_SIGNAL_H
#define FS_SIGNAL_H

#include <dds/DCPS/PoolAllocator.h>

class FileSystemSignal {
  public:
    FileSystemSignal(int id);
    void signal();
    void wait_timeout(int dur_sec);
    void wait_forever();

  private:
    int _id;
    OpenDDS::DCPS::String _file_name;

    bool exists();
};

#endif
