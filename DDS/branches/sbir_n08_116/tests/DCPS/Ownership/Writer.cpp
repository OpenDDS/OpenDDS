/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
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
const int num_messages = 10;
extern int reset_ownership_strength;
extern int delay;
extern int reset_delay;

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
    message.subject_id = 99;

    DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from       = ownership_dw_id_.c_str();
    message.subject    = CORBA::string_dup("Review");
    message.text       = CORBA::string_dup("Worst. Movie. Ever.");
    message.count      = 0;

    for (int i = 0; i < num_messages; i++) {
      DDS::ReturnCode_t error = message_dw->write(message, handle);

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write returned %d!\n"), error));

        if (error == DDS::RETCODE_TIMEOUT) {
          timeout_writes_++;
        }
      }
      
      message.count++;
      
      if (i == num_messages/2) {
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
          std::cout << ownership_dw_id_ << ": reset ownership strength from " 
                    << old << " to " << reset_ownership_strength << std::endl;
          error = this->writer_->set_qos (qos);
          if (error != ::DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: set_qos returned %d!\n"), error));
          }  
        }
        
      }

      if (i == num_messages/2 && reset_delay != 0) {
        //std::cout << ownership_dw_id << ": reset delay from " << delay << " to " 
        //          << reset_delay << std::endl;
        ACE_OS::sleep (reset_delay);
      }
      else if (delay > 0) {
        ACE_OS::sleep (delay);
      }
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
