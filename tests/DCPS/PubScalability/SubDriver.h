#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "SampleTypeTypeSupportC.h"
#include "SampleTypeTypeSupportImpl.h"
#include "SampleTypeC.h"
#include "dds/DCPS/Definitions.h"
#include "ace/String_Base.h"

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
    int               receive_delay_msec_;

    ::DDS::DomainParticipant_var participant_;
    ::DDS::Topic_var             topic_;
    ::DDS::Subscriber_var        subscriber_;
    ::DDS::DataReader_var        datareader_;
    ::Xyz::SampleTypeDataReader_var     foo_datareader_;

    DataReaderListenerImpl*      listener_;
};

#endif
