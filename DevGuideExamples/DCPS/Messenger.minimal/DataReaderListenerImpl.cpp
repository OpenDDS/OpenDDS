/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataReaderListenerImpl.h"
#include "Boilerplate.h"

void
DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
ACE_THROW_SPEC((CORBA::SystemException))
{
  // Safely downcast data reader to type-specific data reader
  Messenger::MessageDataReader_var reader_i = narrow(reader);

  Messenger::Message msg;
  DDS::SampleInfo info;

  // Remove (take) the next sample from the data reader
  DDS::ReturnCode_t error = reader_i->take_next_sample(msg, info);

  if (error == DDS::RETCODE_OK) {
    // Make sure this is not a sample disposed message
    if (info.valid_data) {
      std::cout << "Message: subject    = " << msg.subject.in() << std::endl
                << "         subject_id = " << msg.subject_id   << std::endl
                << "         from       = " << msg.from.in()    << std::endl
                << "         count      = " << msg.count        << std::endl
                << "         text       = " << msg.text.in()    << std::endl;
    }
  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" take_next_sample failed!\n")));
  }
}
