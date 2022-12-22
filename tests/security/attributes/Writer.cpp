/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Writer.h"

#include "Args.h"
#include "SecurityAttributesMessageTypeSupportC.h"

#include <dds/DCPS/WaitSet.h>

#include <dds/DdsDcpsPublicationC.h>

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

const int num_instances_per_writer = 1;

Writer::Writer(DDS::DataWriter_ptr writer, const Args& args)
  : writer_(DDS::DataWriter::_duplicate(writer))
  , args_(args)
  , finished_instances_(0)
  , guard_condition_(new DDS::GuardCondition)
{
}

void
Writer::start()
{
  // Lanuch num_instances_per_writer threads. Each thread writes one
  // instance which uses the thread id as the key value.
  if (activate(THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%N:%l: start()")
               ACE_TEXT(" activate failed!\n")));
    ACE_OS::exit(1);
  }
}

void
Writer::end()
{
  guard_condition_->set_trigger_value(true);
  wait();
}

int
Writer::svc()
{
  DDS::InstanceHandleSeq handles;

  try {
    // Block until Subscriber is available
    DDS::StatusCondition_var condition = writer_->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);
    ws->attach_condition(guard_condition_);
    const DDS::Duration_t timeout = { 1 , 0 };

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};

    if (writer_->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: svc()")
                 ACE_TEXT(" ERROR: get_publication_matched_status failed!\n")));
      ACE_OS::exit(1);
    }

    while (matches.current_count < 1) {
      DDS::ReturnCode_t ret = ws->wait(conditions, timeout);

      if (ret != DDS::RETCODE_OK && ret != DDS::RETCODE_TIMEOUT) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: wait failed!\n")));
        ACE_OS::exit(1);
      }

      for (unsigned i = 0; i < conditions.length(); i++) {
        if (conditions[i] == guard_condition_) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("match timed out\n")));
          return 0;
        }
      }

      if (writer_->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: get_publication_matched_status failed!\n")));
        ACE_OS::exit(1);
      }
    }

    ws->detach_condition(condition);
    ws->detach_condition(guard_condition_);
    if (args_.secure_part_user_data_) {
      // Give secure participant writer time to send
      ACE_OS::sleep(3);
    }

    // Write samples
    SecurityAttributes::MessageDataWriter_var message_dw
      = SecurityAttributes::MessageDataWriter::_narrow(writer_.in());

    if (CORBA::is_nil(message_dw.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: _narrow failed!\n")));
        ACE_OS::exit(1);
    }

    SecurityAttributes::Message message;
    message.subject_id = 99;

    DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from         = "Comic Book Guy";
    message.subject      = "Review";
    message.text         = "Worst. Movie. Ever.";
    message.count        = 0;
    if (args_.extra_space_ > 0) {
      message.extra_space.length(args_.extra_space_);
      memset(&message.extra_space[0], 'a', args_.extra_space_);
    }

    for (int i = 0; i < args_.num_messages_; i++) {
      DDS::ReturnCode_t error;
      do {
        error = message_dw->write(message, handle);
      } while (error == DDS::RETCODE_TIMEOUT);

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write returned %d!\n"), error));
      }

      message.count++;
      message.subject_id--;
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
