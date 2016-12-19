#ifndef PUBDRIVER_H
#define PUBDRIVER_H

#include "dds/DdsDcpsC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportImpl.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataWriterImpl_T.h"
#include "ace/INET_Addr.h"
#include "ace/Task.h"
#include "ace/String_Base.h"
#include <string>

namespace Xyz {
  typedef OpenDDS::DCPS::DataWriterImpl_T<Xyz::Foo> FooDataWriterImpl;
}

class PubDriver : public ACE_Task_Base
{
  public:

    enum Test_Kind {
      REGISTER_TEST   ,
      UNREGISTER_TEST ,
      DISPOSE_TEST    ,
      RESUME_TEST     ,
      LISTENER_TEST   ,
      ALLOCATOR_TEST  ,
      LIVELINESS_TEST
    };

    PubDriver();
    virtual ~PubDriver();

    void run(int& argc, ACE_TCHAR* argv[]);

  private:

    void register_test ();
    void dispose_test ();
    void unregister_test ();
    void resume_test ();
    void listener_test ();
    void allocator_test ();
    void liveliness_test ();
    void parse_args(int& argc, ACE_TCHAR* argv[]);
    void initialize(int& argc, ACE_TCHAR* argv[]);
    void run();
    void end();
    void run_test (int test_to_run);

    void shutdown ();

    ::DDS::DomainParticipant_var participant_;
    ::DDS::Topic_var             topic_;
    ::DDS::Publisher_var         publisher_;
    ::DDS::DataWriter_var        datawriter_;
    ::OpenDDS::DCPS::DataWriterImpl* datawriter_servant_;
    ::Xyz::FooDataWriter_var    foo_datawriter_;
    ::Xyz::FooDataWriterImpl*   foo_datawriter_servant_;

    DDS::InstanceHandle_t sub_handle_;
    int               history_depth_;
    int               test_to_run_;

    int               shutdown_;

    ACE_TString       sub_ready_filename_;
};

#endif
