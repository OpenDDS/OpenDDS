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

#include "ParticipantTask.h"
#include "ProgressIndicator.h"
#include "FooTypeTypeSupportImpl.h"

ParticipantTask::ParticipantTask(const std::size_t& samples_per_thread)
  : samples_per_thread_(samples_per_thread)
#ifdef OPENDDS_SAFETY_PROFILE
  , thread_index_(0)
#endif
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
    DDS::StatusCondition_var cond;
    DDS::WaitSet_var ws = new DDS::WaitSet;

    { // Scope for guard to serialize creating Entities.
      GuardType guard(lock_);

      // Create Participant
      participant =
        dpf->create_participant(42,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

#ifdef OPENDDS_SAFETY_PROFILE
      // RTPS cannot be shared
      char config_name[64], inst_name[64];
      ACE_OS::snprintf(config_name, 64, "cfg_%d", thread_index_);
      ACE_OS::snprintf(inst_name, 64, "rtps_%d", thread_index_);
      ++thread_index_;

      ACE_DEBUG((LM_INFO,
        "(%P|%t)    -> PARTICIPANT creating transport config %C\n",
        config_name));
      OpenDDS::DCPS::TransportConfig_rch config =
        TheTransportRegistry->create_config(config_name);
      OpenDDS::DCPS::TransportInst_rch inst =
        TheTransportRegistry->create_inst(inst_name, "rtps_udp");
      config->instances_.push_back(inst);
      TheTransportRegistry->bind_config(config_name, participant);
#endif

    } // End of lock scope.

    if (CORBA::is_nil(participant.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" create_participant failed!\n")), 1);

    {
      // Create Publisher
      publisher =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(publisher.in()))
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: svc()")
                          ACE_TEXT(" create_publisher failed!\n")), 1);


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

      if (CORBA::is_nil(topic.in()))
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: svc()")
                          ACE_TEXT(" create_topic failed!\n")), 1);

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

      if (CORBA::is_nil(writer.in()))
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: svc()")
                          ACE_TEXT(" create_datawriter failed!\n")), 1);

      writer_i = FooDataWriter::_narrow(writer);
      if (CORBA::is_nil(writer_i))
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: svc()")
                          ACE_TEXT(" _narrow failed!\n")), 1);

      // Block until Subscriber is available
      cond = writer->get_statuscondition();
      cond->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

      ws->attach_condition(cond);

      DDS::Duration_t timeout =
        { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

      DDS::ConditionSeq conditions;
      DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};
      do
      {
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l: svc()")
                            ACE_TEXT(" wait failed!\n")), 1);

        if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK)
        {
          ACE_ERROR ((LM_ERROR,
            "(%P|%t) ERROR: failed to get publication matched status\n"));
          ACE_OS::exit (1);
        }
      }
      while (matches.current_count < 1);

      ws->detach_condition(cond);

      // The following is intentionally inefficient to stress various
      // pathways related to publication; we should be especially dull
      // and write only one sample at a time per writer.

      ProgressIndicator progress("(%P|%t)       PARTICIPANT %d%% (%d samples sent)\n",
                                 samples_per_thread_);

      for (std::size_t i = 0; i < samples_per_thread_; ++i)
      {
        Foo foo;
        foo.key = 3;
        DDS::InstanceHandle_t handle = writer_i->register_instance(foo);

        if (writer_i->write(foo, handle) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l: svc()")
                            ACE_TEXT(" write failed!\n")), 1);
        }
        ++progress;
      }

      DDS::Duration_t interval = { 30, 0};
      if( DDS::RETCODE_OK != writer->wait_for_acknowledgments( interval)) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("(%P:%t) ERROR: svc() - ")
          ACE_TEXT("timed out waiting for acks!\n")
        ), 1);
      }
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
