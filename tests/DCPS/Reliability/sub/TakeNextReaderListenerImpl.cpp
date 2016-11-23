/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TakeNextReaderListenerImpl.h"
#include "Boilerplate.h"
#include <iostream>

TakeNextReaderListenerImpl::TakeNextReaderListenerImpl()
{
  std::cout << "Using TakeNextReaderListenerImpl" << std::endl;
}

void
TakeNextReaderListenerImpl::take_samples(
  Reliability::MessageDataReader_var reader_i)
{
  Reliability::Message msg;
  DDS::SampleInfo info;

  // Remove (take) the next sample from the data reader
  DDS::ReturnCode_t error = reader_i->take_next_sample(msg, info);

  // Make sure take was successful
  if (error == DDS::RETCODE_OK) {
    // Make sure this is not a sample dispose message
    if (info.valid_data) {
      on_sample(msg);
    }
  } else {
    ACE_ERROR((LM_ERROR,
    ACE_TEXT("ERROR: %N:%l: on_data_available() -")
    ACE_TEXT(" take_next_sample failed!\n")));
  }
}

