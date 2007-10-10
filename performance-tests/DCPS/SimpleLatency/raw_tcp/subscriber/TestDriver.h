#ifndef TESTDRIVER_H
#define TESTDRIVER_H

#include <ace/INET_Addr.h>
#include <ace/streams.h>


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

    ACE_INET_Addr parse_address(const std::string& spec);

    unsigned num_packets_;
    char     data_size_;
    unsigned num_bytes_per_packet_;

    ACE_INET_Addr sub_addr_;

};

#endif
