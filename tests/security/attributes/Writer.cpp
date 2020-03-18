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

#include "Args.h"
#include "SecurityAttributesMessageTypeSupportC.h"
#include "Writer.h"

const int num_instances_per_writer = 1;

Writer::Writer(DDS::DataWriter_ptr writer, const SecurityAttributes::Args& args)
  : writer_(DDS::DataWriter::_duplicate(writer))
  , args_(args)
  , finished_instances_(0)
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

    } while (matches.current_count < 1);

    ws->detach_condition(condition);

    // Write samples
    SecurityAttributes::MessageDataWriter_var message_dw
      = SecurityAttributes::MessageDataWriter::_narrow(writer_.in());

    if (CORBA::is_nil(message_dw.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: _narrow failed!\n")));
        ACE_OS::exit(-1);
    }

    SecurityAttributes::Message message;
    message.subject_id = 99;

    DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from         = "Comic Book Guy";
    message.subject      = "Review";
    message.text         = "Worst. Movie. Ever.";
    message.count        = 0;
    if (args_.extra_space > 0) {
      message.extra_space.length(args_.extra_space);
      memset(&message.extra_space[0], 'a', args_.extra_space);
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
