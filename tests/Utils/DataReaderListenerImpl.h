/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TESTUTILS_DATAREADER_LISTENER_IMPL_H
#define TESTUTILS_DATAREADER_LISTENER_IMPL_H

#include <tools/modeling/codegen/model/NullReaderListener.h>
#include <ace/Global_Macros.h>

#include <dds/DdsDcpsSubscriptionS.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/Definitions.h>
#include <ctime>

#include <iostream>

using OpenDDS::Model::NullReaderListener;

namespace TestUtils {

template<typename Message, typename MessageDataReader>
class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<NullReaderListener> {
public:
  typedef Message                             message_type;
  typedef MessageDataReader                   datareader_type;
  typedef TAO_Objref_Var_T<MessageDataReader> datareader_var;

  DataReaderListenerImpl(bool verbose = false)
    : verbose_(verbose)
  {
  }

  void verbose(bool verbose)
  {
    verbose_ = verbose;
  }

  bool verbose() const
  {
    return verbose_;
  }

  virtual void on_sample_lost(
    DDS::DataReader_ptr , DDS::SampleLostStatus status)
  {
    if (verbose_)
      std::cout << "Lost sample: " << status.total_count_change << std::endl;
  }


  virtual void on_sample_rejected(
    DDS::DataReader_ptr , DDS::SampleRejectedStatus status)
  {
    if (verbose_)
      std::cout << "Rejected sample: " << status.total_count_change << " Reason: "
                << status.last_reason << std::endl;
  }

  virtual void on_data_available(
    DDS::DataReader_ptr reader)
  {
    // Safely downcast data reader to type-specific data reader
    datareader_var reader_i = datareader_type::_narrow(reader);

    take_samples(reader_i);
  }

  virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr ,
    const DDS::RequestedIncompatibleQosStatus& )
  {
    if (verbose_)
      std::cout << "Subscriber incompatible QOS" << std::endl;
  }

  protected:
  void take_samples(
    datareader_var reader_i)
  {
    message_type msg;
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


  virtual void on_sample(const message_type& msg) = 0;

private:
  bool verbose_;
};

}
#endif /* TESTUTILS_DATAREADER_LISTENER_IMPL_H */
