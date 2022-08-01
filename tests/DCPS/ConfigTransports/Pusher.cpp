// -*- C++ -*-
//

#include "Pusher.h"
#include "Factory.h"
#include "../common/TestSupport.h"
#include "../FooType4/FooDefTypeSupportImpl.h"
#include "dds/DCPS/WaitSet.h"
#include "ace/OS_NS_unistd.h"

Pusher::Pusher(const Factory& factory,
               const DDS::DomainParticipantFactory_var& dpf,
               const DDS::DomainParticipant_var& participant,
               const DDS::DataWriterListener_var& listener)
  : dpf_(dpf)
  , dp_(participant)
  , pub_(factory.publisher(dp_))
  , topic_(factory.topic(dp_))
  , writer_(factory.writer(pub_, topic_, listener))
{
}

Pusher::Pusher(const Factory& factory,
               const DDS::DomainParticipantFactory_var& dpf,
               const DDS::DomainParticipant_var& participant,
               const DDS::Publisher_var& publisher,
               const DDS::DataWriterListener_var& listener)
  : dpf_(dpf)
  , dp_(participant)
  , pub_(publisher)
  , topic_(factory.topic(dp_))
  , writer_(factory.writer(pub_, topic_, listener))
{
}

Pusher::~Pusher()
{
  // Clean up and shut down DDS objects
  pub_->delete_contained_entities();
  dp_->delete_contained_entities();
}


const int default_key = 101010;

int
Pusher::push(const ACE_Time_Value& duration)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Pusher::push: begins.\n")));

  const ACE_Time_Value started = ACE_OS::gettimeofday();
  unsigned int pass = 0;

  while(ACE_OS::gettimeofday() < started + duration) {
    ++pass;
    try {
      ::Xyz::Foo foo;
      //foo.key set below.
      foo.x = -1;
      foo.y = -1;
      foo.key = default_key;

      ::Xyz::FooDataWriter_var foo_dw = ::Xyz::FooDataWriter::_narrow(writer_.in());
      TEST_ASSERT(!CORBA::is_nil(foo_dw.in()));

      // Block until Subscriber is available
      DDS::StatusCondition_var condition = foo_dw->get_statuscondition();
      condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(condition);

      while (true) {
        DDS::PublicationMatchedStatus matches;
        if (foo_dw->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: Pusher::push: ")
                            ACE_TEXT(" get_publication_matched_status failed (%T)!\n")),
                           -1);
        }

        if (matches.current_count) {
          break;
        }

        DDS::ConditionSeq conditions;
        DDS::Duration_t timeout = { 60, 0 };
        // TODO(sonndinh): The while (true) seems to tell that it wants to wait infinitely
        // until a subscriber matches. But the call to wait below means it only waits 60s.
        // May need to change if the intention was to wait forever.
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: Pusher::push:")
                            ACE_TEXT(" wait failet (%T)!\n")),
                           -1);
        }
      }

      ws->detach_condition(condition);

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Pusher::push: starting to write pass %d (%T)\n"),
                 pass));

      ::DDS::InstanceHandle_t handle = foo_dw->register_instance(foo);

      foo.x = 5.0;
      foo.y = (float)(pass);

      TEST_CHECK(DDS::RETCODE_OK == foo_dw->write(foo, handle));

      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Pusher::push: done writing (%T).\n")));
      ACE_OS::sleep(1);
    } catch (const CORBA::Exception& ex) {
      ex._tao_print_exception("Exception caught in run_test:");
    }
  }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Pusher::push: finished.\n")));
  return 0;
}
