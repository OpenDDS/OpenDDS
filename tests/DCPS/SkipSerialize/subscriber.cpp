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
#ifdef ACE_AS_STATIC_LIBS
# ifndef OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
# endif
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <cstdlib>

#include "DataReaderListener.h"
#include "SkipSerializeTypeSupportImpl.h"
#include "Args.h"

#include <dds/DCPS/DataReaderImpl_T.h>

bool reliable = false;
bool wait_for_acks = false;

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = EXIT_SUCCESS;

  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    if ((status = parse_args(argc, argv)) != EXIT_SUCCESS) {
      return status;
    }

    DDS::DomainParticipantQos part_qos;
    dpf->get_default_participant_qos(part_qos);

    DDS::PropertySeq& props = part_qos.property.value;
    append(props, "OpenDDS.RtpsRelay.Groups", "SkipSerialize", true);

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(4,
                              part_qos,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_participant() failed!\n")),
                       EXIT_FAILURE);
    }

    // Register Type (SkipSerialize::Message)
    SkipSerialize::MessageTypeSupport_var ts =
      new SkipSerialize::MessageTypeSupportImpl();

    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")),
                       EXIT_FAILURE);
    }

    // Create Topic (Movie Discussion List)
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_topic() failed!\n")),
                       EXIT_FAILURE);
    }

    // Create Subscriber
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(sub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_subscriber() failed!\n")),
                       EXIT_FAILURE);
    }

    // Create DataReader
    DataReaderListenerImpl* const listener_servant = new DataReaderListenerImpl;
    DDS::DataReaderListener_var listener(listener_servant);

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    if (DataReaderListenerImpl::is_reliable()) {
      std::cout << "Reliable DataReader" << std::endl;
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    }

    DDS::DataReader_var reader =
      sub->create_datareader(topic.in(),
                             dr_qos,
                             listener.in(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")),
                       EXIT_FAILURE);
    }

    OpenDDS::DCPS::DataReaderImpl_T<SkipSerialize::Message> * dri_mt = dynamic_cast<OpenDDS::DCPS::DataReaderImpl_T<SkipSerialize::Message>*>(reader.in());
    dri_mt->set_marshal_skip_serialize(true);

    // Block until Publisher completes
    DDS::StatusCondition_var condition = reader->get_statuscondition();
    condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::Duration_t timeout =
      { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0 };

    while (true) {
      if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: get_subscription_matched_status() failed!\n")),
                         EXIT_FAILURE);
      }
      if (matches.current_count == 0 && matches.total_count > 0) {
        break;
      }
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: wait() failed!\n")),
                         EXIT_FAILURE);
      }
    }

    if (!listener_servant->is_valid()) {
      status = EXIT_FAILURE;
    }

    ws->detach_condition(condition);

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  }

  return status;
}
