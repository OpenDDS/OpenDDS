#ifndef PUBDRIVER_H
#define PUBDRIVER_H

#include "dds/DdsDcpsC.h"
#include "tests/DCPS/FooType3/FooTypeSupportC.h"
#include "tests/DCPS/FooType3/FooTypeSupportImpl.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "TestS.h"
#include "ace/INET_Addr.h"
#include "ace/Task.h"
#include "ace/String_Base.h"
#include <string>

class PubDriver
  : public ACE_Task_Base,
    public virtual POA_Test::TestPubDriver,
    public virtual PortableServer::RefCountServantBase
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

    void run(int& argc, char* argv[]);

    virtual void shutdown (
      )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));


    virtual void add_new_subscription (
      CORBA::Long       reader_id,
      const char *      sub_addr
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
    void parse_args(int& argc, char* argv[]);
    void initialize(int& argc, char* argv[]);
    void run();
    void end();
    void run_test (int test_to_run);

    int parse_pub_arg(const std::string& arg);
    int parse_sub_arg(const std::string& arg);

    void add_subscription (
    CORBA::Long       reader_id,
    const char *      sub_addr
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

    ACE_CString       pub_id_fname_;
    ACE_INET_Addr     pub_addr_;
    std::string       pub_addr_str_;

    OpenDDS::DCPS::RepoId sub_id_;
    ACE_CString       sub_addr_;
    int               history_depth_;
    int               test_to_run_;

    ACE_CString       pub_driver_ior_;
    int               add_new_subscription_;
    int               shutdown_;
};

#endif
