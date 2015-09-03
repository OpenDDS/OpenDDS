#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "dds/DdsDcpsC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportImpl.h"
#include "dds/DCPS/Definitions.h"
#include "ace/INET_Addr.h"
#include "ace/String_Base.h"
#include <string>

#include <vector>

class DataReaderListenerImpl;

class SubDriver
{
  public:

    typedef std::vector < ::OpenDDS::DCPS::PublicationId > PublicationIds;

    SubDriver();
    virtual ~SubDriver();

    void run(int& argc, ACE_TCHAR* argv[]);


  private:

    void parse_args(int& argc, ACE_TCHAR* argv[]);
    void init(int& argc, ACE_TCHAR* argv[]);
    void run();

    int               num_writes_;

    int               shutdown_pub_;
    int               add_new_subscription_;
    int               shutdown_delay_secs_;

    ACE_TString       sub_ready_filename_;

    ::DDS::DomainParticipant_var participant_;
    ::DDS::Topic_var             topic_;
    ::DDS::Subscriber_var        subscriber_;
    ::DDS::DataReader_var        datareader_;
    ::Xyz::FooDataReader_var     foo_datareader_;

    DataReaderListenerImpl*      listener_;

};

#endif
