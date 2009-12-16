/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#include <dds/DCPS/transport/simpleUnreliableDgram/SimpleUnreliableDgram.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#endif

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"

namespace {

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;

int
parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("t:"));

  int c;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 't':

      if (ACE_OS::strcmp(get_opts.opt_arg(), ACE_TEXT("udp")) == 0) {
        transport_impl_id = 2;

      } else if (ACE_OS::strcmp(get_opts.opt_arg(), ACE_TEXT("multicast")) == 0) {
        transport_impl_id = 3;

      } else if (ACE_OS::strcmp(get_opts.opt_arg(), ACE_TEXT("default_tcp")) == 0) {
        transport_impl_id = OpenDDS::DCPS::DEFAULT_SIMPLE_TCP_ID;

      } else if (ACE_OS::strcmp(get_opts.opt_arg(), ACE_TEXT("default_udp")) == 0) {
        transport_impl_id = OpenDDS::DCPS::DEFAULT_SIMPLE_UDP_ID;

      } else if (ACE_OS::strcmp(get_opts.opt_arg(), ACE_TEXT("default_multicast")) == 0) {
        transport_impl_id = OpenDDS::DCPS::DEFAULT_MULTICAST_ID;
      }

      break;
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("usage: %C -t config\n"), argv[0]),
                       -1);
    }
  }

  return 0;
}

} // namespace

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    int error;
    if ((error = parse_args(argc, argv)) != 0) {
      return error;
    }

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(411,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")),
                       -1);
    }

    // Register TypeSupport (Messenger::Message)
    Messenger::MessageTypeSupport_var mts =
      new Messenger::MessageTypeSupportImpl();

    if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: register_type failed!\n")),
                       -1);
    }

    // Create Topic
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                mts->get_type_name(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_topic failed!\n")),
                       -1);
    }

    // Create Publisher
    DDS::Publisher_var pub =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    DDS::PublisherListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(pub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_publisher failed!\n")),
                       -1);
    }

    // Initialize and attach Transport
    OpenDDS::DCPS::TransportImpl_rch transport_impl =
      TheTransportFactory->create_transport_impl(transport_impl_id,
                                                 OpenDDS::DCPS::AUTO_CONFIG);

    if (transport_impl->attach(pub.in()) != OpenDDS::DCPS::ATTACH_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: attach failed!\n")),
                       -1);
    }

    // Create DataWriter
    DDS::DataWriter_var dw =
      pub->create_datawriter(topic.in(),
                             DATAWRITER_QOS_DEFAULT,
                             DDS::DataWriterListener::_nil(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dw.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                       -1);
    }

    // Start writing threads
    Writer* writer = new Writer(dw.in());
    writer->start();

    while (!writer->is_finished()) {
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);
    }

    writer->end();
    delete writer;

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());

    TheTransportFactory->release();
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    ACE_OS::exit(-1);
  }

  return 0;
}
