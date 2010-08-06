#ifndef PUBDRIVER_H
#define PUBDRIVER_H

#include "dds/DdsDcpsC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportImpl.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "TestS.h"
#include "ace/INET_Addr.h"
#include "ace/Task.h"
#include "ace/String_Base.h"
#include <string>

class PubDriver
  : public ACE_Task_Base,
    public virtual POA_Test::TestPubDriver
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

    virtual void shutdown (
      )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));


    virtual void add_new_subscription (
      const OpenDDS::DCPS::RepoId& reader_id,
      const char *                 sub_addr
      )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  private:

    enum TransportTypeId
    {
      SIMPLE_TCP
    };

    enum TransportInstanceId
    {
      ALL_TRAFFIC
    };

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

    int parse_pub_arg(const ACE_TString& arg);
    int parse_sub_arg(const ACE_TString& arg);

    void add_subscription (
      const OpenDDS::DCPS::RepoId& reader_id,
      const ACE_TCHAR* sub_addr
    );

    void attach_to_transport ();

    ::DDS::DomainParticipant_var participant_;
    ::DDS::Topic_var             topic_;
    ::DDS::Publisher_var         publisher_;
    ::OpenDDS::DCPS::PublisherImpl*  publisher_servant_;
    ::DDS::DataWriter_var        datawriter_;
    ::OpenDDS::DCPS::DataWriterImpl* datawriter_servant_;
    ::Xyz::FooDataWriter_var    foo_datawriter_;
    ::Xyz::FooDataWriterImpl*   foo_datawriter_servant_;

    ACE_TString       pub_id_fname_;
    ACE_INET_Addr     pub_addr_;
    ACE_TString       pub_addr_str_;

    OpenDDS::DCPS::RepoId sub_id_;
    ACE_TString       sub_addr_;
    int               history_depth_;
    int               test_to_run_;

    ACE_CString       pub_driver_ior_;
    int               add_new_subscription_;
    int               shutdown_;

    ACE_TString       sub_ready_filename_;
};

#endif
