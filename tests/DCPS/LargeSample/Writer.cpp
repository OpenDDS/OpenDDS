/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */


#include "MessengerTypeSupportC.h"
#include "Writer.h"
#include "../../Utils/StatusMatching.h"
#include "common.h"

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/DCPS_Utils.h>

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <sstream>
#include <iomanip>

Writer::Writer(DataWriters& datawriters, int my_pid)
: datawriters_(datawriters)
, timeout_writes_(0)
, my_pid_(my_pid)
{
}

void Writer::write(bool reliable, int num_messages, unsigned data_field_length_offset)
{
  DDS::InstanceHandleSeq handles;

  try {
    for (size_t datawriter_i = 0; datawriter_i < datawriters_.size(); ++datawriter_i) {
      DDS::DataWriter_var& datawriter = datawriters_[datawriter_i];

      // Block until Subscriber is available
      Utils::wait_match(datawriter, 1, Utils::GTE);
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writers matched\n")));

      // Write samples
      Messenger::MessageDataWriter_var message_dw
        = Messenger::MessageDataWriter::_narrow(datawriter);
      if (!message_dw) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%N:%l: svc()")
                     ACE_TEXT(" ERROR: _narrow datawriter failed!\n")));
          ACE_OS::exit(1);
      }

      Messenger::Message message;
      message.writer_id = static_cast<int>(datawriter_i + 1);
      message.from = "Comic Book Guy";
      message.process_id = my_pid_;
      message.participant_id = 0;
      message.text = "Worst. Movie. Ever.";
      message.sample_id = 0;

      DDS::InstanceHandle_t handle = message_dw->register_instance(message);

      for (int i = 0; i < num_messages; i++) {

        // Because the reader does not have infinite buffer space
        if (!reliable) {
          ACE_OS::sleep(ACE_Time_Value(0,100000));
        }

        // If there are 2 writers and there are 10 samples per writer, lengths
        // will vary from 15k to 165k.
        // Over 65K will require multiple fragments for udp/mcast.
        message.data.length(
          expected_data_field_length(
            data_field_length_offset, message.writer_id, message.sample_id));
        for (CORBA::ULong j = 0; j < message.data.length(); ++j) {
          message.data[j] = expected_data_field_element(message.writer_id, message.sample_id, j);
        }

        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t)%N:%l: Sending Message: process_id = %d ")
                   ACE_TEXT("writer_id = %d ")
                   ACE_TEXT("sample_id = %d ")
                   ACE_TEXT("extra data length = %d\n"),
                   message.process_id,
                   message.writer_id,
                   message.sample_id,
                   message.data.length()));
        DDS::ReturnCode_t error;
        do {
          error = message_dw->write(message, handle);

          if (error != DDS::RETCODE_OK) {
            if (error == DDS::RETCODE_TIMEOUT) {
              ++timeout_writes_;
            } else {
              ACE_ERROR((LM_ERROR,
                         ACE_TEXT("%N:%l: svc()")
                         ACE_TEXT(" ERROR: write dw returned %C!\n"),
                         OpenDDS::DCPS::retcode_to_string(error)));
            }
          }
        } while (error == DDS::RETCODE_TIMEOUT);

        ++message.sample_id;
      }
    }

    for (size_t datawriter_i = 0; datawriter_i < datawriters_.size(); ++datawriter_i) {
      // Let readers disconnect first, once they either get the data or
      // give up and time-out.  This allows the writer to be alive while
      // processing requests for retransmission from the readers.
      Utils::wait_match(datawriters_[datawriter_i], 0);
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in svc():");
  }
}

int
Writer::get_timeout_writes() const
{
  return timeout_writes_.value();
}
