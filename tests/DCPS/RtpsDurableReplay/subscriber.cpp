// -*- C++ -*-
#include "Common.h"
#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/DistributedConditionSet.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace Messenger;
using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

const TimeDuration LEASE_DURATION(300, 0); // 300 seconds
const TimeDuration RESEND_PERIOD(30, 0); // 30 seconds

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  DataReaderListenerImpl(DistributedConditionSet_rch dcs,
                         RcHandle<TransportInst> discovery_inst,
                         RcHandle<TransportInst> transport_inst)
    : dcs_(dcs)
    , discovery_inst_(discovery_inst)
    , transport_inst_(transport_inst)
    , all_instances_received_before_(false)
    , unmatched_(false)
    , all_instances_received_after_(false)
  {}

  virtual ~DataReaderListenerImpl(void) {}

  virtual void on_requested_deadline_missed(DDS::DataReader_ptr,
                                            const DDS::RequestedDeadlineMissedStatus&) {}

  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr,
                                             const DDS::RequestedIncompatibleQosStatus&) {}

  virtual void on_liveliness_changed(DDS::DataReader_ptr,
                                     const DDS::LivelinessChangedStatus&) {}

  virtual void on_subscription_matched(DDS::DataReader_ptr,
                                       const DDS::SubscriptionMatchedStatus& status)
  {
    if (!unmatched_ &&
        status.total_count == 1 &&
        status.total_count_change == 0 &&
        status.current_count == 0 &&
        status.current_count_change == -1) {
      unmatched_ = true;
      dcs_->post(SUBSCRIBER_ACTOR, UNMATCHED);
    }
  }

  virtual void on_sample_rejected(DDS::DataReader_ptr,
                                  const DDS::SampleRejectedStatus&) {}

  virtual void on_data_available(DDS::DataReader_ptr reader)
  {
    ::Messenger::MessageDataReader_var message_dr = ::Messenger::MessageDataReader::_narrow(reader);

    Messenger::Message message;
    DDS::SampleInfo si;
    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si);

    if (status == DDS::RETCODE_OK) {
      if (si.valid_data) {
        instances_.insert(message.subject_id);
        if (!all_instances_received_before_ && instances_.size() == INSTANCE_COUNT) {
          all_instances_received_before_ = true;
          instances_.clear();
          // Kill communication.
          discovery_inst_->drop_messages_b(1.0);
          discovery_inst_->drop_messages(true);
          transport_inst_->drop_messages_b(1.0);
          transport_inst_->drop_messages(true);
          dcs_->post(SUBSCRIBER_ACTOR, ALL_INSTANCES_RECEIVED_BEFORE);
        }
        if (!all_instances_received_after_ && instances_.size() == INSTANCE_COUNT) {
          all_instances_received_after_ = true;
          dcs_->post(SUBSCRIBER_ACTOR, ALL_INSTANCES_RECEIVED_AFTER);
        }
      }
    }
  }

  virtual void on_sample_lost(DDS::DataReader_ptr,
                              const DDS::SampleLostStatus&) {}

private:
  DistributedConditionSet_rch dcs_;
  RcHandle<TransportInst> discovery_inst_;
  RcHandle<TransportInst> transport_inst_;
  OPENDDS_SET(int) instances_;
  bool all_instances_received_before_;
  bool unmatched_;
  bool all_instances_received_after_;
};

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  // Create discovery.
  RcHandle<RtpsDiscovery> discovery = make_rch<RtpsDiscovery>("RtpsDiscovery");
  TheServiceParticipant->add_discovery(discovery);
  discovery->config()->lease_duration(LEASE_DURATION);
  discovery->config()->resend_period(RESEND_PERIOD);
  TheServiceParticipant->set_repo_domain(TEST_DOMAIN, discovery->key());

  // Create transport.
  RcHandle<TransportConfig> config = TheTransportRegistry->create_config("RtpsTransport");
  RcHandle<TransportInst> transport_inst = TheTransportRegistry->create_inst("RtpsTransport", "rtps_udp");
  config->instances_.push_back(transport_inst);

  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  DDS::DomainParticipant_var participant =
    dpf->create_participant(TEST_DOMAIN,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  OpenDDS::DCPS::TransportRegistry::instance()->bind_config("RtpsTransport", participant);

  OpenDDS::DCPS::DomainParticipantImpl* impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in());
  RcHandle<TransportInst> discovery_inst = discovery->sedp_transport_inst(TEST_DOMAIN, impl->get_id());

  MessageTypeSupport_var type_support = new MessageTypeSupportImpl();

  type_support->register_type(participant, "");

  CORBA::String_var type_name = type_support->get_type_name();

  DDS::Topic_var topic =
    participant->create_topic("Movie Discussion List",
                              type_name,
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::Subscriber_var subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   DDS::SubscriberListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DataReaderListenerImpl* reader_listener = new DataReaderListenerImpl(dcs, discovery_inst, transport_inst);
  ::DDS::DataReaderListener_var listener_var(reader_listener);

  ::DDS::DataReaderQos reader_qos;
  subscriber->get_default_datareader_qos(reader_qos);

  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

  DDS::DataReader_var reader_var =
    subscriber->create_datareader(topic,
                                  reader_qos,
                                  listener_var,
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  dcs->wait_for(SUBSCRIBER_ACTOR, SUBSCRIBER_ACTOR, UNMATCHED);

  // Sleep for 15 second for fallback to engage.
  ACE_OS::sleep(15);

  dcs->post(SUBSCRIBER_ACTOR, RESUME);
  transport_inst->drop_messages(false);
  discovery_inst->drop_messages(false);

  const MonotonicTimePoint before = MonotonicTimePoint::now();
  dcs->wait_for(SUBSCRIBER_ACTOR, SUBSCRIBER_ACTOR, ALL_INSTANCES_RECEIVED_AFTER);
  const MonotonicTimePoint after = MonotonicTimePoint::now();
  const TimeDuration elapsed = after - before;

  participant->delete_contained_entities();
  dpf->delete_participant(participant);

  TheServiceParticipant->shutdown();

  const TimeDuration limit(6);

  ACE_DEBUG((LM_DEBUG, "elapsed: %C limit: %C\n", elapsed.sec_str().c_str(), limit.sec_str().c_str()));

  return elapsed < limit ? 0 : 1;
}
