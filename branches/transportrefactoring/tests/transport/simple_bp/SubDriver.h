#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "SimpleSubscriber.h"
#include "dds/DCPS/Definitions.h"
#include "ace/INET_Addr.h"
#include <string>

class SubDriver
{
  public:

    SubDriver();
    virtual ~SubDriver();

    void run(int& argc, char* argv[]);


  private:

    enum TransportTypeId
    {
      SIMPLE_TCP
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


    SimpleSubscriber subscriber_;

    TAO::DCPS::RepoId pub_id_;
    ACE_INET_Addr     pub_addr_;

    TAO::DCPS::RepoId sub_id_;
    ACE_INET_Addr     sub_addr_;

    unsigned num_msgs_;
};

#endif
