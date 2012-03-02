#ifndef PUBDRIVER_H
#define PUBDRIVER_H

#include "Writer.h"
#include "dds/DdsDcpsC.h"
#include "dds/DCPS/Definitions.h"
#include "ace/INET_Addr.h"
#include "ace/Task.h"
#include "ace/String_Base.h"
#include <string>

class PubDriver
{
  public:

    friend class Writer;

    PubDriver();
    virtual ~PubDriver();

    void run(int& argc, ACE_TCHAR* argv[]);

  private:

    void parse_args(int& argc, ACE_TCHAR* argv[]);
    void init(int& argc, ACE_TCHAR* argv[]);
    void run();
    void end();

    ::DDS::DomainParticipant_var participant_;
    ::DDS::Topic_var             topic_;
    ::DDS::Publisher_var         publisher_;
    ::DDS::DataWriter_var *      datawriters_;
    Writer**                     writers_;

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
};

#endif
