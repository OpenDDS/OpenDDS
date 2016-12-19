/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SeqReaderListenerImpl.h"
#include "Boilerplate.h"
#include <iostream>

using namespace examples::boilerplate;

SeqReaderListenerImpl::SeqReaderListenerImpl() :
  messages_(10), infos_(10)
{
  std::cout << "Using SeqReaderListenerImpl" << std::endl;
}

void
SeqReaderListenerImpl::take_samples(
  Reliability::MessageDataReader_var reader_i
)
{

  while (true) {
    // Remove (take) the next sample from the data reader
    DDS::ReturnCode_t error = reader_i->take(messages_, infos_, 10,
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    // Make sure take was successful
    if (error == DDS::RETCODE_OK) {
      for (unsigned int i = 0; i < messages_.length(); ++i) {
        // Make sure this is not a sample dispose message
        if (infos_[i].valid_data) {
          on_sample(messages_[i]);
        } else {
          ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Got invalid %d\n")));
        }
      }
    } else if (error == DDS::RETCODE_NO_DATA) {
      break;
    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                 ACE_TEXT(" take_next_sample failed!\n")));
    }
  }
}

