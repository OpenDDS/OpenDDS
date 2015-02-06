#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "SimpleDataReader.h"
#include "dds/DCPS/Definitions.h"
#include "ace/INET_Addr.h"
#include <string>

class SubDriver
{
  public:

    SubDriver();
    virtual ~SubDriver();

    void run(int& argc, ACE_TCHAR* argv[]);


  private:

    void parse_args(int& argc, ACE_TCHAR* argv[]);
    void init();
    void run();

    int parse_pub_arg(const ACE_TString& arg);
    int parse_sub_arg(const ACE_TString& arg);

    OpenDDS::DCPS::RepoId pub_id_;
    ACE_INET_Addr     pub_addr_;
    ACE_TString       pub_addr_str_;

    OpenDDS::DCPS::RepoId sub_id_;
    ACE_INET_Addr     sub_addr_;
    ACE_TString       sub_addr_str_;

    SimpleDataReader reader_;

    int num_msgs_;
    bool shmem_;
};

#endif
