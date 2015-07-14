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
#include "model/Sync.h"

const int num_instances_per_writer = 1;
const int num_messages = 20;
extern int reset_ownership_strength;
extern ACE_Time_Value dds_delay;
extern ACE_Time_Value reset_delay;
extern int ownership_strength;

Writer::Writer(DDS::DataWriter_ptr writer, const char* ownership_dw_id)
  : writer_(DDS::DataWriter::_duplicate(writer)),
    ownership_dw_id_(ownership_dw_id),
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

    Messenger::Message message;

    message.from       = CORBA::string_dup(ownership_dw_id_.c_str());
    message.subject    = CORBA::string_dup("Review");
    message.text       = CORBA::string_dup("Worst. Movie. Ever.");
    message.count      = 1;
    message.strength   = ownership_strength;

    for (int i = 0; i < num_messages; i++) {
      message.subject_id = message.count % 2;  // 0 or 1
      ACE_DEBUG ((LM_DEBUG, "(%P|%t) %s writes instance %d count %d str %d\n",
      ownership_dw_id_.c_str(), message.subject_id, message.count, message.strength));
      DDS::ReturnCode_t error = message_dw->write(message, ::DDS::HANDLE_NIL);

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write returned %d!\n"), error));

        if (error == DDS::RETCODE_TIMEOUT) {
          timeout_writes_++;
        }
      }

      if (message.count == 5) {
        ::DDS::DataWriterQos qos;
        error = this->writer_->get_qos (qos);
        if (error != ::DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: get_qos returned %d!\n"), error));
        }
        CORBA::Long old = qos.ownership_strength.value;
        if (reset_ownership_strength != -1 && old != reset_ownership_strength) {
          qos.ownership_strength.value = reset_ownership_strength;
          // Add a delay so the builtin topic data update arrives after samples
          // with previous strength is received by datareader. This helps simplify
          // result verification on subscriber side.
          ACE_OS::sleep (1);
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) %s : reset ownership strength from %d to %d\n"),
            ownership_dw_id_.c_str(), old, reset_ownership_strength));
          error = this->writer_->set_qos (qos);
          if (error != ::DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: set_qos returned %d!\n"), error));
          }
          else {
            message.strength   =  reset_ownership_strength;
            ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) ownership strength in message is now %d\n"),
              message.strength));
            error = this->writer_->get_qos(qos);
            if (error != ::DDS::RETCODE_OK) {
              ACE_ERROR((LM_ERROR,
                       ACE_TEXT("%N:%l: svc()")
                       ACE_TEXT(" ERROR: get_qos returned %d!\n"), error));
            } else {
              ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) ownership strength in qos is now %d\n"),
                qos.ownership_strength.value));
            }
          }
        }

      }

      if ((message.count == 5)
           && reset_delay > ACE_Time_Value::zero) {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) %s : reset delay from %d to %d at sample %d\n"),
          ownership_dw_id_.c_str(), dds_delay.msec(), reset_delay.msec(), message.count));
        ACE_OS::sleep (reset_delay);
      }
      else if (dds_delay > ACE_Time_Value::zero) {
        ACE_OS::sleep (dds_delay);
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

void
Writer::wait_for_acks()
{
  OpenDDS::Model::WriterSync::wait_ack(writer_);
}

int
Writer::get_timeout_writes() const
{
  return timeout_writes_.value();
}
