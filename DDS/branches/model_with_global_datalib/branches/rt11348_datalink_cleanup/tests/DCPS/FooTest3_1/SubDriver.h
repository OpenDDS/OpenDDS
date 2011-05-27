#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "SimpleSubscriber.h"
#include "dds/DCPS/Definitions.h"
#include "TestC.h"
#include "ace/INET_Addr.h"
#include "ace/String_Base.h"

#include <vector>

class SubDriver
{
  public:
  
    typedef std::vector < ::OpenDDS::DCPS::PublicationId > PublicationIds;

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
    void init(int& argc, char* argv[]);
    void run();

    int parse_pub_arg(const std::string& arg);
    int parse_sub_arg(const std::string& arg);

    CORBA::ORB_var    orb_;

    SimpleSubscriber  subscriber_;

    ACE_CString       pub_id_fname_;
    ACE_INET_Addr     pub_addr_;
    std::string       pub_addr_str_; 

    OpenDDS::DCPS::RepoId sub_id_;
    ACE_INET_Addr     sub_addr_;
    std::string       sub_addr_str_;

    int               num_writes_;
    int               receive_delay_msec_;
    ::Test::TestPubDriver_var pub_driver_;
    ACE_CString       pub_driver_ior_;
};

#endif
