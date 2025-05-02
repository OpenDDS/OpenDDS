// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 */
 // ============================================================================

#include "MessengerTypeSupportImpl.h"
#include "DataReaderListener.h"

#include <tests/Utils/DistributedConditionSet.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include "dds/DCPS/StaticIncludes.h"

#include "ace/Get_Opt.h"

using namespace Messenger;
using namespace std;

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  bool ok = true;

  try {
    DistributedConditionSet_rch dcs =
      OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    const OpenDDS::DCPS::String actor = TheServiceParticipant->config_store()->get("APP_NAME", "");
    ACE_DEBUG((LM_DEBUG, "(%P|%t) actor = %C\n", actor.c_str()));

    DDS::DomainParticipant_var participant =
      dpf->create_participant(4,
        PARTICIPANT_QOS_DEFAULT,
        DDS::DomainParticipantListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_participant failed.\n")));
      return 1;
    }

    DDS::TypeSupport_var ts = new MessageTypeSupportImpl;

    if (DDS::RETCODE_OK != ts->register_type(participant, "")) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) register_type failed.\n")));
      exit(1);
    }

    CORBA::String_var type_name = ts->get_type_name();

    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List TLT",
        type_name,
        TOPIC_QOS_DEFAULT,
        DDS::TopicListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(topic)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_topic failed.\n")));
      exit(1);
    }

    //Create a subscriber before the publisher
    // Create the subscriber
    DDS::Subscriber_var sub1 =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
        DDS::SubscriberListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(sub1)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Failed to create_subscriber.\n")));
      exit(1);
    }

    // activate the listener
    DDS::DataReaderListener_var listener1(new DataReaderListenerImpl(dcs, "pub sub 1", 4));
    DataReaderListenerImpl* listener_servant1 =
      dynamic_cast<DataReaderListenerImpl*>(listener1.in());

    if (CORBA::is_nil(listener1)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) listener is nil.\n")));
      exit(1);
    }
    if (!listener_servant1) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: listener_servant1 is nil (dynamic_cast failed)!\n")), -1);
    }

    // Create the Datareaders
    DDS::DataReaderQos dr_qos;
    sub1->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    DDS::DataReader_var dr1 = sub1->create_datareader(topic,
      dr_qos,
      listener1,
      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dr1)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datareader dr1 durable failed.\n")));
      exit(1);
    }
    else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) create_datareader dr1 durable success.\n")));
    }

    //Now create the publisher
    DDS::Publisher_var pub =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(pub)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_publisher failed.\n")));
      exit(1);
    }

    // Create the datawriter
    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);
    dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    dw_qos.resource_limits.max_samples_per_instance = 1000;
    dw_qos.history.depth = 1;
    dw_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;

    DDS::DataWriter_var dw1 =
      pub->create_datawriter(topic,
        dw_qos,
        0,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dw1)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datawriter dw1 failed.\n")));
      exit(1);
    }

    MessageDataWriter_var message_dw = MessageDataWriter::_narrow(dw1);

    Message message_instance1;
    message_instance1.subject_id = 1;
    DDS::InstanceHandle_t handle_instance1 = message_dw->register_instance(message_instance1);

    message_instance1.from = "Comic Book Guy";
    message_instance1.subject = "Review";
    message_instance1.text = "Worst. Movie. Ever.";
    message_instance1.count = 1;

    Message message_instance2;
    message_instance2.subject_id = 2;
    DDS::InstanceHandle_t handle_instance2 = message_dw->register_instance(message_instance2);

    message_instance2.from = "Comic Book Guy - Instance 2";
    message_instance2.subject = "Review - Instance 2";
    message_instance2.text = "Worst. Movie. Ever. - Instance 2";
    message_instance2.count = 2;

    DDS::ReturnCode_t ret = message_dw->write(message_instance1, handle_instance1);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: instance: %d message: %d - write() returned %d.\n"),
        message_instance1.subject_id, message_instance1.count, ret));
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) wrote instance: %d message : %d\n"),
        message_instance1.subject_id, message_instance1.count));
    }

    ret = message_dw->write(message_instance2, handle_instance2);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: instance: %d message: %d - write() returned %d.\n"),
        message_instance2.count, ret));
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) wrote instance: %d message : %d\n"),
        message_instance2.subject_id, message_instance2.count));
    }

    DDS::DataWriter_var dw2 =
      pub->create_datawriter(topic,
        dw_qos,
        0,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dw2)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datawriter dw2 failed.\n")));
      exit(1);
    }

    MessageDataWriter_var message_dw2 = MessageDataWriter::_narrow(dw2);

    Message message_instance3;
    message_instance3.subject_id = 3;
    DDS::InstanceHandle_t handle_instance3 = message_dw2->register_instance(message_instance3);

    message_instance3.from = "Comic Book Guy - Instance 3";
    message_instance3.subject = "Review - Instance 3";
    message_instance3.text = "Worst. Movie. Ever. - Instance 3";
    message_instance3.count = 3;

    Message message_instance4;
    message_instance4.subject_id = 4;
    DDS::InstanceHandle_t handle_instance4 = message_dw2->register_instance(message_instance4);

    message_instance4.from = "Comic Book Guy - Instance 4";
    message_instance4.subject = "Review - Instance 4";
    message_instance4.text = "Worst. Movie. Ever. - Instance 4";
    message_instance4.count = 4;

    ret = message_dw2->write(message_instance3, handle_instance3);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: instance: %d message: %d - write() returned %d.\n"),
        message_instance3.subject_id, message_instance3.count, ret));
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) wrote instance: %d message : %d\n"),
        message_instance3.subject_id, message_instance3.count));
    }

    ret = message_dw2->write(message_instance4, handle_instance4);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: instance: %d message: %d - write() returned %d.\n"),
        message_instance4.subject_id, message_instance4.count, ret));
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) wrote instance: %d message : %d\n"),
        message_instance4.subject_id, message_instance4.count));
    }

    dcs->post(actor, "write done");
    dcs->wait_for(actor, "pub sub 1", "read done");

    ok = listener_servant1->ok();
    if (ok) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Reader 1 in pub process received all samples\n")));
    } else {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Reader 1 in pub process failed to receive expected number of samples\n")));
    }

    // activate the second listener
    DDS::DataReaderListener_var listener2(new DataReaderListenerImpl(dcs, "pub sub 2", 4));
    DataReaderListenerImpl* listener_servant2 =
      dynamic_cast<DataReaderListenerImpl*>(listener2.in());

    if (CORBA::is_nil(listener2)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) listener is nil.\n")));
      exit(1);
    }
    if (!listener_servant2) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: listener_servant2 is nil (dynamic_cast failed)!\n")), -1);
    }

    // Create the Datareaders
    DDS::DataReader_var dr2 = sub1->create_datareader(topic,
      dr_qos,
      listener2,
      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dr2)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datareader dr2 durable failed.\n")));
      exit(1);
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) create_datareader dr2 durable success.\n")));
    }

    dcs->wait_for(actor, "pub sub 2", "read done");

    if (ok) {
      ok = listener_servant2->ok();
      if (ok) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Reader 2 in pub process received all samples\n")));
      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Reader 2 in pub process failed to receive expected number of samples\n")));
      }
    }

    dcs->wait_for(actor, "sub1", "read done");
    dcs->wait_for(actor, "sub2", "read done");

    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
  }
  catch (CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PUB: Exception caught in main.cpp: %C\n"), e._info().c_str()));
    exit(1);
  }

  return ok ? 0 : 1;
}
