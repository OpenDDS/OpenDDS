/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#endif

#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include "Args.h"

#include "dds/DCPS/Replayer.h"
#include "dds/DCPS/Recorder.h"

#include "ace/Semaphore.h"
#include "ace/Thread_Semaphore.h"

bool
make_dr_reliable()
{
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return gc->instances_[0]->name() == "the_rtps_transport";
}


class MessengerReplayerListener : public OpenDDS::DCPS::ReplayerListener
{
public:
  MessengerReplayerListener()
    : sem_(0)
  {
  }

  virtual void on_replayer_matched(OpenDDS::DCPS::                         Replayer*,
                                   const ::DDS::PublicationMatchedStatus & status)
  {
    if (status.current_count > 0 ) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("MessengerReplayerListener -- a reader connect to replayer\n" )));
      connected_readers_.insert(status.last_subscription_handle);
      sem_.release();
    }
    else if (status.current_count == 0 && status.total_count > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("MessengerRecorderListener -- reader disconnect with replayer\n" )));
    }
  }

  int wait(const ACE_Time_Value & tv) {
    ACE_Time_Value timeout = ACE_OS::gettimeofday() + tv;
    return sem_.acquire(timeout);
  }
  std::set<DDS::InstanceHandle_t> connected_readers_;
private:
  ACE_Thread_Semaphore sem_;
};


class MessengerRecorderListener : public OpenDDS::DCPS::RecorderListener
{
public:
  explicit MessengerRecorderListener(const OpenDDS::DCPS::Replayer_var& replayer)
    : sem_(0)
    , replayer_(replayer)
  {
  }

  virtual void on_sample_data_received(OpenDDS::DCPS::                     Recorder*,
                                       const OpenDDS::DCPS::RawDataSample& sample)
  {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("MessengerRecorderListener::on_sample_data_received\n")));

    MessengerReplayerListener* replayer_listener =
      static_cast<MessengerReplayerListener*>(replayer_->get_listener().in());

    if (replayer_listener->connected_readers_.size()) {
      // get the instance handle of one of the connected reader
      DDS::InstanceHandle_t reader_handle = *(replayer_listener->connected_readers_.begin());

      // Send to only one connected reader. To send to all readers, use
      // replayer_->write(sample)
      if (DDS::RETCODE_ERROR == replayer_->write_to_reader(reader_handle, sample)) {
        ACE_DEBUG((LM_DEBUG, "Write Sample Error\n"));
      }
    }
  }

  virtual void on_recorder_matched(OpenDDS::DCPS::                         Recorder*,
                                   const ::DDS::SubscriptionMatchedStatus& status )
  {
    if (status.current_count == 1) {
        ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("MessengerRecorderListener -- a writer connect to recorder\n")));
    }
    else if (status.current_count == 0 && status.total_count > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("MessengerRecorderListener -- writer disconnect with recorder\n")));
      sem_.release();
    }
  }

  int wait(const ACE_Time_Value & tv) {
    ACE_Time_Value timeout = ACE_OS::gettimeofday() + tv;
    return sem_.acquire(timeout);
  }
private:
  ACE_Thread_Semaphore sem_;
  OpenDDS::DCPS::Replayer_var replayer_;
};


int run_test(int argc, ACE_TCHAR *argv[]){
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    int error;
    if ((error = parse_args(argc, argv)) != 0) {
      return error;
    }

    OpenDDS::DCPS::Service_Participant* service = TheServiceParticipant;

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(4,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_participant failed!\n")),
                       -1);
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) Start relay\n"));

    using namespace OpenDDS::DCPS;

    {
      // Create Topic (Movie Discussion List)
      DDS::Topic_var topic =
        service->create_typeless_topic(participant,
                                       "Movie Discussion List",
                                       "Messenger",
                                       true,
                                       TOPIC_QOS_DEFAULT,
                                       0,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!topic) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_topic failed!\n")),
                         -1);
      }

      // setup partition
      DDS::PublisherQos pub_qos;
      participant->get_default_publisher_qos(pub_qos);

      DDS::StringSeq my_partition2;
      my_partition2.length(1);
      my_partition2[0] = "Two";
      pub_qos.partition.name = my_partition2;


      RcHandle<MessengerReplayerListener> replayer_listener = make_rch<MessengerReplayerListener>();


      ACE_DEBUG((LM_DEBUG, "Creating replayer\n"));

      // Create Replayer
      OpenDDS::DCPS::Replayer_var replayer =
        service->create_replayer(participant,
                                 topic.in(),
                                 pub_qos,
                                 DATAWRITER_QOS_DEFAULT,
                                 replayer_listener);

      if (!replayer.in()) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_replayer failed!\n")),
                         -1);
      }

      ACE_Time_Value wait_time(60, 0);

      // wait until there exist a reader connect to the replayer
      if (replayer_listener->wait(wait_time) == -1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" replayer timeout!\n")),
                         -1);
      }
      ACE_DEBUG((LM_DEBUG, "replayer listener wait done\n"));

      RcHandle<MessengerRecorderListener> recorder_listener = make_rch<MessengerRecorderListener> (replayer);

      // setup partition
      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos(sub_qos);
      DDS::StringSeq my_partition1;
      my_partition1.length(1);
      my_partition1[0] = "One";
      sub_qos.partition.name = my_partition1;

      DDS::DataReaderQos dr_qos = service->initial_DataReaderQos();

      if (make_dr_reliable()) {
        dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      // Create Recorder
      OpenDDS::DCPS::Recorder_var recorder =
        service->create_recorder(participant,
                                 topic.in(),
                                 sub_qos,
                                 dr_qos,
                                 recorder_listener);

      if (!recorder.in()) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_recorder failed!\n")),
                         -1);
      }


      // wait until the writer disconnnects
      if (recorder_listener->wait(wait_time) == -1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" recorder timeout!\n")),
                         -1);
      }

      service->delete_recorder(recorder);
      service->delete_replayer(replayer);
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) Stop relay\n"));

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    TheServiceParticipant->shutdown();


  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  ACE_DEBUG((LM_DEBUG, "(%P|%t) Relay exiting\n"));

  return 0;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = run_test(argc, argv);
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
