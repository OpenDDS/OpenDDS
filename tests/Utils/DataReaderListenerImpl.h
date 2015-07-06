/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TESTUTILS_DATAREADER_LISTENER_IMPL_H
#define TESTUTILS_DATAREADER_LISTENER_IMPL_H

#include <ace/Global_Macros.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/Definitions.h>
#include <ctime>

#include <iostream>

namespace TestUtils {

template<typename Message, typename MessageDataReader>
class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
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
    DDS::DataReader_ptr , const DDS::SampleLostStatus& status)
  {
    if (verbose_)
      std::cerr << "Lost sample: " << status.total_count_change << std::endl;
  }


  virtual void on_sample_rejected(
    DDS::DataReader_ptr , const DDS::SampleRejectedStatus& status)
  {
    if (verbose_)
      std::cerr << "Rejected sample: " << status.total_count_change << " Reason: "
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
      std::cerr << "Subscriber incompatible QOS" << std::endl;
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

  virtual void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&)
  {
  }

  virtual void on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&)
  {
  }

  virtual void on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&)
  {
  }

private:
  bool verbose_;
};

}
#endif /* TESTUTILS_DATAREADER_LISTENER_IMPL_H */
