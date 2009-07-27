#ifndef PUBDRIVER_H
#define PUBDRIVER_H

#include "Writer.h"
#include "dds/DdsDcpsC.h"
#include "dds/DCPS/Definitions.h"
#include "TestS.h"
#include "ace/INET_Addr.h"
#include "ace/Task.h"
#include "ace/String_Base.h"

#include <string>

class PubDriver
  : public virtual POA_Test::TestPubDriver,
    public virtual PortableServer::RefCountServantBase
{
  public:

    friend class Writer;

    PubDriver();
    virtual ~PubDriver();

    void run(int& argc, ACE_TCHAR* argv[]);

    virtual void shutdown (
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

    void parse_args(int& argc, ACE_TCHAR* argv[]);
    void init(int& argc, ACE_TCHAR* argv[]);
    void run();
    void end();
    void attach_to_transport ();

    int parse_pub_arg(const ACE_TString& arg);
    int parse_sub_arg(const ACE_TString& arg);

    ::DDS::DomainParticipant_var participant_;
    ::DDS::Topic_var             topic_;
    ::DDS::Publisher_var         publisher_;
    ::DDS::DataWriter_var *      datawriters_;
    Writer**                     writers_;

    ACE_TString       pub_id_fname_;
    ACE_INET_Addr     pub_addr_;
    ACE_TString       pub_addr_str_;

    OpenDDS::DCPS::RepoId sub_id_;
    ACE_INET_Addr     sub_addr_;
    ACE_TString       sub_addr_str_;

    int               block_on_write_;
    int               num_threads_to_write_;
    int               multiple_instances_;
    int               num_writes_per_thread_;
    int               num_datawriters_;
    int               max_samples_per_instance_;
    int               history_depth_;
    int               has_key_;
    int               write_delay_msec_;
    int               check_data_dropped_;
    ACE_CString       pub_driver_ior_;
    bool              shutdown_;

    ACE_TString       sub_ready_filename_;
};

#endif
