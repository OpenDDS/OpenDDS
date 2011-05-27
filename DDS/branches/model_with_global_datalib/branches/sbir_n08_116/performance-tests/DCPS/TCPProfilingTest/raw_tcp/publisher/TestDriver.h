#ifndef TESTDRIVER_H
#define TESTDRIVER_H

#include "TestData.h"
#include <ace/INET_Addr.h>
#include <list>
#include <string>

class TcpPublisher;


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

    ACE_INET_Addr parse_subscriber_address(const std::string& spec);


    typedef std::list<ACE_INET_Addr> AddrList;
    typedef std::list<TcpPublisher*> PublisherList;

    char     publisher_id_;
    unsigned num_packets_;
    char     data_size_;

    TestData::PacketHolder* packet_;

    PublisherList publishers_;
    AddrList      subscriber_addrs_;
};

#endif
