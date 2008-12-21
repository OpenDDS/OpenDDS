#ifndef TESTDRIVER_H
#define TESTDRIVER_H

#include "TestData.h"
#include <ace/INET_Addr.h>
#include <ace/streams.h>


class TestDriver
{
  public:

    TestDriver();
    virtual ~TestDriver();

    void run(int& argc, char* argv[]);


  private:

    void parse_args(int& argc, char* argv[]);
    void init();
    void run_i();

    ACE_INET_Addr parse_address(const std::string& spec);


    char     publisher_id_;
    unsigned num_packets_;
    char     data_size_;

    TestData::PacketHolder* packet_;

    ACE_INET_Addr   publisher_listen_addr_;
    ACE_INET_Addr   subscriber_addr_;
};

#endif
