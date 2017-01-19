/*
 */

#include <ace/Atomic_Op_T.h>
#include <ace/Basic_Types.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/Thread_Mutex.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/WaitSet.h>

#include "ParticipantTask.h"
#include "ProgressIndicator.h"
#include "FooTypeTypeSupportImpl.h"

ParticipantTask::ParticipantTask(const std::size_t& samples_per_thread,
                                 int delay_between_pubs_msec,
                                 const DDS::Duration_t & deadline)
  : samples_per_thread_(samples_per_thread)
  , delay_between_pubs_msec_(delay_between_pubs_msec)
  , deadline_(deadline)
{}

ParticipantTask::~ParticipantTask()
{}

int
ParticipantTask::svc()
{
  try
  {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    -> PARTICIPANT STARTED\n")));

    ACE_Time_Value delay_between_pubs(0, this->delay_between_pubs_msec_ * 1000);

    DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    // Create Participant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(42,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" create_participant failed!\n")), 1);

    // Create Publisher
    DDS::Publisher_var publisher =
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

    writer_qos.history.depth = static_cast<CORBA::Long>(samples_per_thread_);

    if (deadline_.sec > 0 || deadline_.nanosec > 0)
      {
        writer_qos.deadline.period.sec = deadline_.sec;
        writer_qos.deadline.period.nanosec = deadline_.nanosec;
      }

    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic.in(),
                                   writer_qos,
                                   DDS::DataWriterListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(writer.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" create_datawriter failed!\n")), 1);

    FooDataWriter_var writer_i = FooDataWriter::_narrow(writer);
    if (CORBA::is_nil(writer_i))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: svc()")
                        ACE_TEXT(" _narrow failed!\n")), 1);

    // Block until Subscriber is available
    DDS::StatusCondition_var cond = writer->get_statuscondition();
    cond->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
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

      if (writer_i->write(foo, handle) != DDS::RETCODE_OK)
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: svc()")
                          ACE_TEXT(" write failed!\n")), 1);
           ++progress;
      ACE_OS::sleep(delay_between_pubs);
    }

    DDS::Duration_t interval = { 30, 0};
    if( DDS::RETCODE_OK != writer->wait_for_acknowledgments( interval)) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P:%t) ERROR: svc() - ")
        ACE_TEXT("timed out waiting for acks!\n")
      ), 1);
    }
    publisher->delete_datawriter(writer);

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("caught in svc()");
    return 1;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- PARTICIPANT FINISHED\n")));

  return 0;
}
