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

#include "tests/Utils/StatusMatching.h"

#include "tests/DCPS/LargeSample/MessengerTypeSupportC.h"
#include "Writer.h"

Writer::Writer(const Options& options, Writers& writers)
  : options_(options)
  , writers_(writers)
{
}

bool
Writer::write()
{
  bool valid = true;
  try {
    typedef std::vector<DDS::InstanceHandle_t> Handles;
    Handles handles;
    const unsigned int subscribers = options_.num_sub_processes *
      options_.num_sub_participants * options_.num_readers;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%T (%P|%t) Writers wait for %d subscribers\n"),
               subscribers));

    // Block until Subscriber is available
    for (Writers::const_iterator writer = writers_.begin();
         writer != writers_.end();
         ++writer) {
      Utils::wait_match(writer->writer, subscribers, Utils::GTE);

      // we already have a ref count, no need to take another
      Messenger::MessageDataWriter_ptr message_dw =
        dynamic_cast<Messenger::MessageDataWriter*>(writer->writer.in());

      if (CORBA::is_nil(message_dw)) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%N:%l: svc()")
                     ACE_TEXT(" ERROR: _narrow dw1 failed!\n")));
          ACE_OS::exit(-1);
      }

      handles.push_back(message_dw->register_instance(writer->message));
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Writers matched\n")));

    const ACE_Time_Value delay(options_.delay_msec / 1000,
                               (options_.delay_msec % 1000) * 1000);

    for (unsigned int i = 0; i < options_.num_samples; i++) {
      Handles::iterator handle = handles.begin();
      for (Writers::iterator writer = writers_.begin();
           writer != writers_.end();
           ++writer, ++handle) {
        // we already have a ref count, no need to take another
        // Write samples
        Messenger::MessageDataWriter_ptr message_dw =
          dynamic_cast<Messenger::MessageDataWriter*>(writer->writer.in());

        ++writer->message.sample_id;

        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("%T (%P|%t) Writing Message: process_id = %C ")
                   ACE_TEXT("participant_id = %d ")
                   ACE_TEXT("writer_id = %d ")
                   ACE_TEXT("sample_id = %d \n"),
                   writer->message.process_id.in(),
                   writer->message.participant_id,
                   writer->message.writer_id,
                   writer->message.sample_id));

        DDS::ReturnCode_t error = message_dw->write(writer->message, *handle);

        if (error != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%N:%l: svc()")
                     ACE_TEXT(" ERROR: writer returned %d!\n"), error));
          valid = false;
        }
        ACE_OS::sleep(delay);
      }
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Messages written, waiting for readers to disconnect\n")));

    // Let readers disconnect first, once they either get the data or
    // give up and time-out.  This allows the writer to be alive while
    // processing requests for retransmission from the readers.
    for (Writers::const_iterator writer = writers_.begin();
         writer != writers_.end();
         ++writer) {
      Utils::wait_match(writer->writer, 0);
    }
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in svc():");
    valid = false;
  }

  return valid;
}
