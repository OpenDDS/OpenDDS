#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "tests/Utils/DDSApp.h"
#include "tests/Utils/Options.h"
#include "tests/Utils/ListenerRecorder.h"
#include "tests/Utils/StatusMatching.h"
#include "tests/Utils/DistributedConditionSet.h"
#include "ace/OS_NS_unistd.h"
#include <vector>

class SetDataReaderQosReliable {
  public:
    void operator()(DDS::DataReaderQos& qos)
    {
      qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    }
};

template<typename MessageType>
class SubDriver
{
public:

  typedef MessageType message_type;
  typedef OpenDDS::DCPS::DDSTraits<MessageType> TraitsType;
  typedef typename TraitsType::DataReaderType datareader_type;

    /* typedef std::vector < ::OpenDDS::DCPS::PublicationId > PublicationIds; */

    SubDriver()
    : num_writes_ (0),
    receive_delay_msec_ (0),
    verbose_ (false)
    {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) SubDriver::SubDriver\n")));
      }

    }

    virtual ~SubDriver()
    {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) SubDriver::~SubDriver\n")));
      }
    }

    void run(::TestUtils::DDSApp& ddsApp, ::TestUtils::Options& options)
    {
        parse_args(options);
        run(ddsApp);
    }

  private:

    void parse_args(::TestUtils::Options& options)
    {
      num_writes_         = options.get<long>("num_writes");
      receive_delay_msec_ = options.get<long>("receive_delay_msec");
      verbose_            = options.get<bool>("verbose");
    }

    void run(::TestUtils::DDSApp& ddsApp)
    {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) SubDriver::run\n")));
      }

      const std::string topic_name("topic_name");

      ::TestUtils::DDSTopicFacade<MessageType> topic_facade =
          ddsApp.topic_facade<MessageType> (topic_name);

      // Create Listener
      typedef typename ::TestUtils::ListenerRecorder< message_type,
        datareader_type> ListenerRecorder;

      ListenerRecorder* listener_impl = new ListenerRecorder;

      listener_impl->verbose(verbose_);
      DDS::DataReaderListener_var listener(listener_impl);

      // Create data reader for the topic
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Sub Creating Reader\n")));
      }

      SetDataReaderQosReliable data_reader_qos;

      DDS::DataReader_var reader = topic_facade.reader(listener,data_reader_qos);

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Sub waiting for writer to come and go\n")));
      }

      if (wait_match(reader, 1, Utils::GTE) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "ERROR: %N:%l: run: "
                   "wait for publisher failed\n"));
        return;
      }

      while (listener_impl->size() != num_writes_) {
        ACE_OS::sleep(1);
      }

      DistributedConditionSet_rch distributed_condition_set =
        OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

      distributed_condition_set->post("subscriber", "done");
    }

    size_t             num_writes_;
    long               receive_delay_msec_;
    bool               verbose_;
};

#endif
