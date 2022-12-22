/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/WaitSet.h>

#include "MessengerTypeSupportC.h"
#include "Writer.h"

const int num_instances_per_writer = 1;
extern int num_messages;
static ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> count;


Writer::Writer(DDS::Publisher_ptr publisher, DDS::DataWriter_ptr writer)
  : publisher_(DDS::Publisher::_duplicate(publisher)),
    writer_(DDS::DataWriter::_duplicate(writer)),
    finished_instances_(0),
    timeout_writes_(0)
{
}

void
Writer::start()
{
  if (activate(THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%N:%l: start()")
               ACE_TEXT(" activate failed!\n")));
    ACE_OS::exit(-1);
  }
}

void
Writer::end()
{
  wait();
}

int
Writer::svc()
{
  ACE_thread_t thr_id = ACE_OS::thr_self();

  DDS::InstanceHandleSeq handles;

  try {
    // Block until Subscriber is available
    DDS::StatusCondition_var condition = writer_->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::Duration_t timeout =
      { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};

    do {
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: wait failed!\n")));
        ACE_OS::exit(-1);
      }

      if (writer_->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: get_publication_matched_status failed!\n")));
        ACE_OS::exit(-1);
      }

    } while (matches.current_count < 2);

    ws->detach_condition(condition);

    // Write samples
    Messenger::MessageDataWriter_var message_dw
      = Messenger::MessageDataWriter::_narrow(writer_.in());

    if (CORBA::is_nil(message_dw.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: _narrow failed!\n")));
        ACE_OS::exit(-1);
    }

    ::DDS::ReturnCode_t ret = publisher_->begin_coherent_changes();
    if (ret != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: begin_coherent_changes failed!\n")));
      ACE_OS::exit(-1);
    }

    Messenger::Message message;
    message.subject_id = CORBA::ULongLong(thr_id);

    DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from       = CORBA::string_dup("Comic Book Guy");
    message.subject    = CORBA::string_dup("Review");
    message.text       = CORBA::string_dup("Worst. Movie. Ever.");

    for (int i = 0; i < num_messages; i++) {
      message.count      = next_count();
      DDS::ReturnCode_t error = message_dw->write(message, handle);

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write returned %d!\n"), error));

        if (error == DDS::RETCODE_TIMEOUT) {
          timeout_writes_++;
        }
      }

      ACE_OS::sleep(1);
    }

    ret = publisher_->end_coherent_changes();
    if (ret != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: end_coherent_changes failed!\n")));
      ACE_OS::exit(-1);
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in svc():");
  }

  finished_instances_ ++;
  return 0;
}

bool
Writer::is_finished() const
{
  return finished_instances_ == num_instances_per_writer;
}

int
Writer::get_timeout_writes() const
{
  return timeout_writes_.value();
}


int
Writer::next_count()
{
  ++ count;
  return count.value();
}
