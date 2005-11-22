#ifndef PUBDRIVER_H
#define PUBDRIVER_H

#include "SimplePublisher.h"
#include "dds/DCPS/Definitions.h"
#include "ace/INET_Addr.h"
#include <string>


class PubDriver
{
  public:

    PubDriver();
    virtual ~PubDriver();

    void run(int& argc, char* argv[]);


  private:

    enum TransportTypeId
    {
      SIMPLE_UDP
    };

    enum TransportInstanceId
    {
      ALL_TRAFFIC
    };

    void parse_args(int& argc, char* argv[]);
    void init();
    void run();

    int parse_pub_arg(const std::string& arg);
    int parse_sub_arg(const std::string& arg);


    SimplePublisher publisher_;

    TAO::DCPS::RepoId pub_id_;
    ACE_INET_Addr     pub_addr_;

    TAO::DCPS::RepoId sub_id_;
    ACE_INET_Addr     sub_addr_;
};

#endif
