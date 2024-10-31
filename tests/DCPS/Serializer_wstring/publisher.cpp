// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <ace/OS_main.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "tests/Utils/DistributedConditionSet.h"
#include "tests/Utils/StatusMatching.h"

#include "dds/DCPS/StaticIncludes.h"

using namespace Messenger;
using namespace std;

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    DistributedConditionSet_rch dcs =
      OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    DDS::DomainParticipant_var participant =
      dpf->create_participant(111,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (participant.in ())) {
      cerr << "create_participant failed." << endl;
      return 1;
    }

    MessageTypeSupportImpl::_var_type servant = new MessageTypeSupportImpl;

    if (DDS::RETCODE_OK != servant->register_type(participant.in (), "")) {
      cerr << "register_type failed." << endl;
      exit(1);
    }

    CORBA::String_var type_name = servant->get_type_name ();

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);
    DDS::Topic_var topic =
      participant->create_topic ("Movie Discussion List",
                                 type_name.in (),
                                 topic_qos,
                                 DDS::TopicListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (topic.in ())) {
      cerr << "create_topic failed." << endl;
      exit(1);
    }

    DDS::Publisher_var pub =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    DDS::PublisherListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (pub.in ())) {
      cerr << "create_publisher failed." << endl;
      exit(1);
    }

    // Create the datawriter
    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos (dw_qos);
    DDS::DataWriter_var dw =
      pub->create_datawriter(topic.in (),
                             dw_qos,
                             DDS::DataWriterListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (dw.in ())) {
      cerr << "create_datawriter failed." << endl;
      exit(1);
    }

    Utils::wait_match(dw, 1, Utils::EQ);
    dcs->wait_for("publisher", "subscriber", "ready");

    Writer* writer = new Writer(dw.in());

    writer->start ();
    writer->end ();
    dcs->wait_for("publisher", "subscriber", "done");

    delete writer;
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in ());
    TheServiceParticipant->shutdown ();
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in main.cpp:" << endl
         << e << endl;
    exit(1);
  }

  return 0;
}
