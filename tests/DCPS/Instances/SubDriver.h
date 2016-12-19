#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "tests/Utils/DDSApp.h"
#include "tests/Utils/Options.h"
#include "tests/Utils/ListenerRecorder.h"
#include "model/Sync.h"
#include "ace/OS_NS_unistd.h"
#include <vector>

// Set data reader QOS to use topic QOS
class SetDataReaderQosUseTopicQos {
  public:
    void operator()(DDS::DataReaderQos& qos)
    {
      qos = DATAREADER_QOS_USE_TOPIC_QOS;
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
                   ACE_TEXT("(%P|%t) SubDriver::SubDriver \n")));
      }

    }

    virtual ~SubDriver()
    {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) SubDriver::~SubDriver \n")));
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
               ACE_TEXT("(%P|%t) SubDriver::run \n")));
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

      SetDataReaderQosUseTopicQos data_reader_qos = {};

      DDS::DataReader_var reader = topic_facade.reader(listener,data_reader_qos);

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Sub waiting for writer to come and go\n")));
      }
      {
        OpenDDS::Model::ReaderSync rs(reader);
      }

      const typename ListenerRecorder::Messages msgs = listener_impl->messages();
      if (msgs.empty()) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) ERROR: no messages received\n")));
      }

      while ( msgs.empty() || msgs.size() != num_writes_) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) STATUS: not enough messages seen\n")));
          ACE_OS::sleep(1);
      }

    }

    size_t             num_writes_;
    long               receive_delay_msec_;
    bool               verbose_;
};

#endif
