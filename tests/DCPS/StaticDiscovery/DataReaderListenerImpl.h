/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include <ace/Global_Macros.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/Definitions.h>

#include <string>
#include <vector>
#include <set>
#include <map>

typedef void (*callback_t)(bool);

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl(const std::string& id, bool reliable, bool expect_all_samples, const std::vector<std::string>& writers, const int total_writers, const int expected_samples, callback_t done_callback, DDS::Subscriber_ptr subscriber, bool check_bits)
    : id_(id)
    , reliable_(reliable)
    , expect_all_samples_(expect_all_samples)
    , writers_(writers)
    , total_writers_(total_writers)
    , expected_samples_(expected_samples)
    , previous_count_(0)
    , received_samples_(0)
    , done_callback_(done_callback)
    , builtin_read_error_(false)
#ifndef DDS_HAS_MINIMUM_BIT
    , check_bits_(check_bits)
#endif
  {
    ACE_UNUSED_ARG(subscriber);
#ifdef DDS_HAS_MINIMUM_BIT
    ACE_UNUSED_ARG(check_bits);
#endif
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Starting DataReader %C\n", id.c_str()));
  }

  ~DataReaderListenerImpl();

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

#ifndef DDS_HAS_MINIMUM_BIT
  void set_builtin_datareader (DDS::DataReader_ptr builtin);

  bool builtin_read_errors () const {
    return builtin_read_error_;
  }
#endif /* DDS_HAS_MINIMUM_BIT */

private:
  std::string id_;
  bool reliable_;
  bool expect_all_samples_;
  const std::vector<std::string>& writers_;
  const int total_writers_;
  const int expected_samples_;
  int previous_count_;
  int received_samples_;
  typedef std::set<int> SampleSet;
  typedef std::map<int, SampleSet> SampleSetMap;
  SampleSetMap ph_received_samples_;
  callback_t done_callback_;
  bool builtin_read_error_;
#ifndef DDS_HAS_MINIMUM_BIT
  bool check_bits_;
  DDS::DataReader_var builtin_;
#endif /* DDS_HAS_MINIMUM_BIT */
};

#endif /* DATAREADER_LISTENER_IMPL_H */
