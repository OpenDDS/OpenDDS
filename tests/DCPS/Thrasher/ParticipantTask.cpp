/*
 */

#include <ace/Atomic_Op_T.h>
#include <ace/Basic_Types.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_stdio.h>
#include <ace/Thread_Mutex.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <dds/DCPS/RTPS/RtpsDiscovery.h>

#include "ParticipantTask.h"
#include "ProgressIndicator.h"
#include "FooTypeTypeSupportImpl.h"

#include "tests/Utils/StatusMatching.h"

ParticipantTask::ParticipantTask(const std::size_t& samples_per_thread)
  : samples_per_thread_(samples_per_thread)
  , thread_index_(0)
{
}

ParticipantTask::~ParticipantTask()
{}

int
ParticipantTask::svc()
{
  try
  {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    -> PARTICIPANT STARTED\n")));

    DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    DDS::DomainParticipant_var participant;
    DDS::Publisher_var publisher;
    DDS::DataWriter_var writer;
    FooDataWriter_var writer_i;

    int this_thread_index = 0;
    { // Scope for guard to serialize creating Entities.
      GuardType guard(lock_);

      this_thread_index = thread_index_++;

      // Create Participant
      participant =
        dpf->create_participant(42,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      // RTPS cannot be shared
      OpenDDS::DCPS::Discovery_rch disc = TheServiceParticipant->get_discovery(42);
      OpenDDS::RTPS::RtpsDiscovery_rch rd = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(disc);
      if (!rd.is_nil()) {
        char config_name[64], inst_name[64];
        ACE_TCHAR nak_depth[8];
        ACE_OS::snprintf(config_name, 64, "cfg_%d", this_thread_index);
        ACE_OS::snprintf(inst_name, 64, "rtps_%d", this_thread_index);
        ACE_OS::snprintf(nak_depth, 8, ACE_TEXT("%lu"), samples_per_thread_);

        ACE_DEBUG((LM_INFO,
          "(%P|%t)    -> PARTICIPANT %d creating transport config %C\n",
          this_thread_index, config_name));
        OpenDDS::DCPS::TransportConfig_rch config =
          TheTransportRegistry->create_config(config_name);
        OpenDDS::DCPS::TransportInst_rch inst =
          TheTransportRegistry->create_inst(inst_name, "rtps_udp");
        ACE_Configuration_Heap ach;
        ACE_Configuration_Section_Key sect_key;
        ach.open();
        ach.open_section(ach.root_section(), ACE_TEXT("not_root"), 1, sect_key);
        ach.set_string_value(sect_key, ACE_TEXT("use_multicast"), ACE_TEXT("0"));
        ach.set_string_value(sect_key, ACE_TEXT("nak_depth"), nak_depth);
        ach.set_string_value(sect_key, ACE_TEXT("heartbeat_period"), ACE_TEXT("200"));
        ach.set_string_value(sect_key, ACE_TEXT("heartbeat_response_delay"), ACE_TEXT("100"));
        inst->load(ach, sect_key);
        config->instances_.push_back(inst);
        TheTransportRegistry->bind_config(config_name, participant);
      }
    } // End of lock scope.

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" create_participant failed!\n")), 1);
    }

    // Create Publisher
    publisher =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    DDS::PublisherListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(publisher.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" create_publisher failed!\n")), 1);
    }


    // Register Type (FooType)
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK)
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" register_type failed!\n")), 1);

    // Create Topic (FooTopic)
    DDS::Topic_var topic =
      participant->create_topic("FooTopic",
                                CORBA::String_var(ts->get_type_name()),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" create_topic failed!\n")), 1);
    }

    // Create DataWriter
    DDS::DataWriterQos writer_qos;
    publisher->get_default_datawriter_qos(writer_qos);
    writer_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    writer_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
    writer_qos.history.depth = static_cast<CORBA::Long>(samples_per_thread_);
#endif

    writer =
      publisher->create_datawriter(topic.in(),
                                   writer_qos,
                                   DDS::DataWriterListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(writer.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" create_datawriter failed!\n")), 1);
    }

    ACE_DEBUG((LM_INFO, "(%P|%t)    -> PARTICIPANT %d waiting for match\n", this_thread_index));

    Utils::wait_match(writer, 1);

    ACE_DEBUG((LM_INFO, "(%P|%t)    -> PARTICIPANT %d match found!\n", this_thread_index));

    writer_i = FooDataWriter::_narrow(writer);
    if (CORBA::is_nil(writer_i)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" _narrow failed!\n")), 1);
    }

    // The following is intentionally inefficient to stress various
    // pathways related to publication; we should be especially dull
    // and write only one sample at a time per writer.

    ProgressIndicator progress("(%P|%t)       PARTICIPANT %d%% (%d samples sent)\n",
                               samples_per_thread_);

    for (std::size_t i = 0; i < samples_per_thread_; ++i)
    {
      Foo foo;
      foo.key = 3;
      foo.x = (float) this_thread_index;
      foo.y = (float) i;
      DDS::InstanceHandle_t handle = writer_i->register_instance(foo);

      if (writer_i->write(foo, handle) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: svc()")
                          ACE_TEXT(" write failed!\n")), 1);
      }
      ++progress;
    }

    DDS::Duration_t interval = { 30, 0 };
    if (DDS::RETCODE_OK != writer->wait_for_acknowledgments(interval)) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P:%t) ERROR: svc() - ")
        ACE_TEXT("timed out waiting for acks!\n")
      ), 1);
    }

    // Clean-up!
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)       <- PUBLISHER PARTICIPANT DEL CONT ENTITIES\n")));
    participant->delete_contained_entities();
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)       <- PUBLISHER DELETE PARTICIPANT\n")));
    dpf->delete_participant(participant.in());
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)       <- PUBLISHER PARTICIPANT VARS GOING OUT OF SCOPE\n")));
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("caught in svc()");
    return 1;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- PARTICIPANT FINISHED\n")));

  return 0;
}
