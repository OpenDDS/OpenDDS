/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Args.h"

#include <tests/Utils/DistributedConditionSet.h>

#include <dds/DCPS/Replayer.h>
#include <dds/DCPS/Recorder.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/StaticIncludes.h>
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
# include <dds/DCPS/transport/udp/Udp.h>
# include <dds/DCPS/transport/multicast/Multicast.h>
# include <dds/DCPS/RTPS/RtpsDiscovery.h>
# include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
# include <dds/DCPS/transport/shmem/Shmem.h>
#endif

#include <ace/Condition_Thread_Mutex.h>

bool
make_dr_reliable()
{
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return gc->instances_[0]->name() == "the_rtps_transport";
}

class MessengerReplayerListener : public OpenDDS::DCPS::ReplayerListener {
public:
  virtual void on_replayer_matched(OpenDDS::DCPS::Replayer*,
                                   const ::DDS::PublicationMatchedStatus & status)
  {
    if (status.current_count > 0 ) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("MessengerReplayerListener -- a reader connect to replayer\n")));
      connected_readers_.insert(status.last_subscription_handle);
    } else if (status.current_count == 0 && status.total_count > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("MessengerRecorderListener -- reader disconnect with replayer\n")));
    }
  }

  typedef std::set<DDS::InstanceHandle_t> ReaderSet;

  const ReaderSet& connected_readers() const
  {
    return connected_readers_;
  }

private:
  ReaderSet connected_readers_;
};

class MessengerRecorderListener : public OpenDDS::DCPS::RecorderListener {
public:
  MessengerRecorderListener(const OpenDDS::DCPS::Replayer_var& replayer,
                            const DistributedConditionSet_rch& dcs)
    : replayer_(replayer)
    , sample_count_(0)
    , dcs_(dcs)
    , match_cond_(match_status_mutex_)
    , unmatch_cond_(match_status_mutex_)
    , match_status_(INVALID)
  {
  }

  virtual void on_sample_data_received(OpenDDS::DCPS::Recorder*,
                                       const OpenDDS::DCPS::RawDataSample& sample)
  {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("MessengerRecorderListener::on_sample_data_received\n")));

    if (++sample_count_ >= NUM_SAMPLES) {
      dcs_->post(ACTOR_RECORDER, EVENT_RECEIVED_ALL_SAMPLES);
    }

    MessengerReplayerListener* replayer_listener =
      static_cast<MessengerReplayerListener*>(replayer_->get_listener().in());

    if (replayer_listener->connected_readers().size()) {
      // get the instance handle of one of the connected reader
      const DDS::InstanceHandle_t reader_handle = *(replayer_listener->connected_readers().begin());

      // Send to only one connected reader. To send to all readers, use replayer_->write(sample)
      if (DDS::RETCODE_ERROR == replayer_->write_to_reader(reader_handle, sample)) {
        ACE_ERROR((LM_ERROR, "Write Sample Error\n"));
      } else {
        ACE_DEBUG((LM_DEBUG, "Relay write sample to reader\n"));
      }
    }
  }

  virtual void on_recorder_matched(OpenDDS::DCPS::Recorder*,
                                   const ::DDS::SubscriptionMatchedStatus& status)
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, match_status_mutex_);
    if (status.current_count == 1) {
      ACE_DEBUG((LM_DEBUG, "MessengerRecorderListener -- a writer connect to recorder\n"));
      match_status_ = MATCHED;
      match_cond_.signal();
    } else if (status.current_count == 0 && status.total_count > 0) {
      ACE_DEBUG((LM_DEBUG, "MessengerRecorderListener -- writer disconnect with recorder\n"));
      match_status_ = UNMATCHED;
      unmatch_cond_.signal();
    }
  }

  int wait_match()
  {
    // In the main thread, wait for the publisher to associate
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, match_status_mutex_, -1);
    while (match_status_ == UNMATCHED) {
      if (match_cond_.wait() != 0) {
        return -1;
      }
    }
    return 0;
  }

  int wait_unmatch()
  {
    // In the main thread, wait for the publisher to deassociate
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, match_status_mutex_, -1);
    while (match_status_ == MATCHED) {
      if (unmatch_cond_.wait() != 0) {
        return -1;
      }
    }
  }

private:
  OpenDDS::DCPS::Replayer_var replayer_;
  int sample_count_;
  DistributedConditionSet_rch dcs_;

  enum MatchStatus {
    MATCHED,
    UNMATCHED,
    INVALID
  };

  ACE_Condition_Thread_Mutex match_cond_;
  ACE_Condition_Thread_Mutex unmatch_cond_;
  ACE_Thread_Mutex match_status_mutex_;
  MatchStatus match_status_;
};

int run_test(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    int error;
    if ((error = parse_args(argc, argv)) != 0) {
      return error;
    }

    OpenDDS::DCPS::Service_Participant* service = TheServiceParticipant;

    // Create DomainParticipant
    DDS::DomainParticipant_var participant = dpf->create_participant(4,
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

    // Create Topic (Movie Discussion List)
    DDS::Topic_var topic = service->create_typeless_topic(participant,
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

    // Setup partition
    DDS::PublisherQos pub_qos;
    participant->get_default_publisher_qos(pub_qos);

    DDS::StringSeq my_partition2;
    my_partition2.length(1);
    my_partition2[0] = "Two";
    pub_qos.partition.name = my_partition2;

    RcHandle<MessengerReplayerListener> replayer_listener = make_rch<MessengerReplayerListener>();

    ACE_DEBUG((LM_DEBUG, "Creating replayer\n"));

    // Create Replayer
    OpenDDS::DCPS::Replayer_var replayer = service->create_replayer(participant,
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

    // Wait for the replayer to be matched with the subscriber
    DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();
    dcs->wait_for(ACTOR_REPLAYER, ACTOR_SUBSCRIBER, EVENT_SUBSCRIBER_JOINED);

    RcHandle<MessengerRecorderListener> recorder_listener = make_rch<MessengerRecorderListener>(replayer, dcs);

    // Setup partition
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
    OpenDDS::DCPS::Recorder_var recorder = service->create_recorder(participant,
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
    recorder->check_encap(false);

    // Wait for the recorder to be matched with the publisher.
    if (recorder_listener->wait_match() != 0) {
      return -1;
    }

    // Then post the condition so that the publisher can proceed
    dcs->post(ACTOR_RECORDER, EVENT_RECORDER_JOINED);

    // Now wait for the publisher to finish
    if (recorder_listener->wait_unmatch() != 0) {
      return -1;
    }

    service->delete_recorder(recorder);
    service->delete_replayer(replayer);

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
