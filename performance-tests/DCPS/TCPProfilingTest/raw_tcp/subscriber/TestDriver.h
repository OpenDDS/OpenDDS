#ifndef TESTDRIVER_H
#define TESTDRIVER_H

#include "TestStats.h"
#include <ace/INET_Addr.h>


class TestDriver
{
  public:

    TestDriver();
    virtual ~TestDriver();

    void run(int& argc, ACE_TCHAR* argv[]);


  private:

    void parse_args(int& argc, ACE_TCHAR* argv[]);
    void init();
    void run_i();

    unsigned num_publishers_;
    unsigned num_packets_;
    char     data_size_;

    ACE_INET_Addr addr_;

    TestStats stats_;
};

#endif
