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

#include <sstream>

const int num_instances_per_writer = 1;
bool reliable = false;
bool wait_for_acks = false;

Writer::Writer(DDS::DataWriter_ptr writer)
  : writer_(DDS::DataWriter::_duplicate(writer))
  , finished_instances_(0)
  , writer_phase_(0)
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
Writer::increment_phase()
{
  ++writer_phase_;
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

    message.from         = "Comic Book Guy";
    message.subject      = "Review";
    message.text         = "Worst. Movie. Ever.";
    message.count        = 0;
    message.phase_number = 0;

    int sleep_between_writes_ms = 250000;
    int writes_per_second = 1000000 / sleep_between_writes_ms;
    int num_messages = (stub_kills + 1) * (stub_duration+5) * writes_per_second;
    for (int i = 0; i < num_messages; i++) {
      DDS::ReturnCode_t error;

      message.phase_number = writer_phase_.value();
      do {
        error = message_dw->write(message, handle);
      } while (error == DDS::RETCODE_TIMEOUT);

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write returned %d!\n"), error));
      }

      message.count++;
      ACE_OS::sleep(ACE_Time_Value(0, sleep_between_writes_ms));
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
