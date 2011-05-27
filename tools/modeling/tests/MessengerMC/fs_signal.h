#ifndef FS_SIGNAL_H
#define FS_SIGNAL_H

class FileSystemSignal {
  public:
    FileSystemSignal(int id);
    void signal();
    void wait_timeout(int dur_sec);
    void wait_forever();

  private:
    int _id;
    char _file_name[24];

    bool exists();
};

#endif
