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
#include "MessengerTypeSupportC.h"
#include "Writer.h"

const int num_instances_per_writer = 1;
bool reliable = false;
bool wait_for_acks = false;
bool use_data = false;
Writer::Writer(DDS::DataWriter_ptr writer, const Writer::message_t messageType)
  : writer_(DDS::DataWriter::_duplicate(writer)),
    finished_instances_(0), messageType_(messageType)
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
    if(messageType_ == Writer::messenger)
    {
      std::cout << "Write messenger()\n";
      writeMessenger();
    }
    else if(messageType_ == Writer::data)
    {
      std::cout << "Write data()\n";
      writeData();
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

/**
 * @brief a helper function for svc to write the Messenger::Message
 */ 
void Writer::writeMessenger()
{
  Messenger::MessageDataWriter_var message_dw
    = Messenger::MessageDataWriter::_narrow(writer_.in());

  if (CORBA::is_nil(message_dw.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: svc()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
  }

  Messenger::Message message;
  message.subject_id = 3145790066;

  DDS::InstanceHandle_t handle = message_dw->register_instance(message);

  message.from         = "Alice";
  message.subject      = "Message";
  message.body         = "It's a secret to everybody.";
  message.count        = 0;

  for (int i = 0; i < num_messages; i++) {
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

    if(message.count % (Messenger::BASE_2 + 1) == 0) {
      message.base.on_off(true);
    }
    if(message.count % (Messenger::BASE_10 + 1) == 0) {
      message.base.decimal(42);
    }
    if(message.count % (Messenger::BASE_16 + 1) == 0) {
      Messenger::hexadecimal_t data = {'0', 'F'};
      message.base.hexadecimal(data);
    }
  }
}

/**
 * @brief a helper function for svc to write the Messenger::Data
 */ 
void Writer::writeData()
{
  Messenger::DataDataWriter_var data_dw
    = Messenger::DataDataWriter::_narrow(writer_.in());

  if (CORBA::is_nil(data_dw.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: svc()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
  }

  Messenger::Data data;

  DDS::InstanceHandle_t handle = data_dw->register_instance(data);

  for (int i = 0; i < num_messages; i++) {
    DDS::ReturnCode_t error;
    do {
      error = data_dw->write(data, handle);
    } while (error == DDS::RETCODE_TIMEOUT);

    if (error != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: svc()")
                 ACE_TEXT(" ERROR: write returned %d!\n"), error));
    }

    if(i % (Messenger::BASE_2 + 1) == 0) {
      data.on_off(true);
    }
    if(i % (Messenger::BASE_10 + 1) == 0) {
      data.decimal(42);
    }
    if(i % (Messenger::BASE_16 + 1) == 0) {
      Messenger::hexadecimal_t hex = {'0', 'F'};
      data.hexadecimal(hex);
    }
  }
}
