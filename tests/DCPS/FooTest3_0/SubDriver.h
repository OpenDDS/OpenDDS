#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "dds/DdsDcpsC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportImpl.h"
#include <tests/Utils/DistributedConditionSet.h>
#include "dds/DCPS/Definitions.h"
#include "ace/INET_Addr.h"
#include "ace/String_Base.h"
#include <string>

#include <vector>

class DataReaderListenerImpl;

class SubDriver
{
  public:

    typedef std::vector < ::OpenDDS::DCPS::GUID_t > PublicationIds;

    SubDriver();
    virtual ~SubDriver();

    void run(int& argc, ACE_TCHAR* argv[]);


  private:

    void parse_args(int& argc, ACE_TCHAR* argv[]);
    void init(DistributedConditionSet_rch dcs, int& argc, ACE_TCHAR* argv[]);
    void run(DistributedConditionSet_rch dcs);

    int               num_writes_;
    int               num_disposed_;

    int               shutdown_pub_;
    int               add_new_subscription_;
    int               shutdown_delay_secs_;

    ::DDS::DomainParticipant_var participant_;
    ::DDS::Topic_var             topic_;
    ::DDS::Subscriber_var        subscriber_;
    ::DDS::DataReader_var        datareader_;
    ::Xyz::FooDataReader_var     foo_datareader_;

    DataReaderListenerImpl*      listener_;
    // Are we going to use the QueryCondition version of the listener
    bool                         qc_usage_;
};

#endif
