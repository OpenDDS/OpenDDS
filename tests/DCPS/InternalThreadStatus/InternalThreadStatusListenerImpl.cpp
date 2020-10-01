/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "InternalThreadStatusListenerImpl.h"
#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <ace/streams.h>
#include <string>

// Implementation skeleton constructor
InternalThreadStatusListenerImpl::InternalThreadStatusListenerImpl(OPENDDS_STRING id, callback_t callback) :
  id_(id), callback_(callback), count_(0), done_(false)
{
}

// Implementation skeleton destructor
InternalThreadStatusListenerImpl::~InternalThreadStatusListenerImpl()
{
}

void InternalThreadStatusListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader_var builtin_dr =
    OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader::_narrow(reader);
  if (0 == builtin_dr)
    {
      std::cerr << "InternalThreadStatusListenerImpl::"
                << "on_data_available: _narrow failed." << std::endl;
      ACE_OS::exit(1);
    }

  OpenDDS::DCPS::InternalThreadBuiltinTopicData thread_info;
  DDS::SampleInfo si;

  for (DDS::ReturnCode_t status = builtin_dr->read_next_sample(thread_info, si);
       status == DDS::RETCODE_OK;
       status = builtin_dr->read_next_sample(thread_info, si)) {

    // copy octet[] to guid
    OpenDDS::DCPS::RepoId guid;
    std::memcpy(&guid, &thread_info.guid, sizeof(guid));

    std::cout << "== " << id_ << " Thread Info ==" << std::endl;
    std::cout
    << "  guid: " << guid << std::endl
    << "   tid: " << thread_info.thread_id << std::endl
    << "  time: " << thread_info.timestamp.sec << std::endl;

    ++count_;
  }

  if (!done_ && count_ >= 10) {
    std::cout << id_ << " received " << count_ << " internal thread updates." << std::endl;
    callback_();
    done_ = true; // only call callback once.
  }
}

void InternalThreadStatusListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
{
  std::cerr << "InternalThreadStatusListenerImpl::"
    << "on_requested_deadline_missed" << std::endl;
}

void InternalThreadStatusListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
{
  std::cerr << "InternalThreadStatusListenerImpl::"
    << "on_requested_incompatible_qos" << std::endl;
}

void InternalThreadStatusListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus&)
{
  std::cerr << "InternalThreadStatusListenerImpl::"
    << "on_liveliness_changed" << std::endl;
}

void InternalThreadStatusListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
{
  std::cerr << "InternalThreadStatusListenerImpl::"
    << "on_subscription_matched" << std::endl;
}

void InternalThreadStatusListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
{
  std::cerr << "InternalThreadStatusListenerImpl::"
    << "on_sample_rejected" << std::endl;
}

void InternalThreadStatusListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  std::cerr << "InternalThreadStatusListenerImpl::"
    << "on_sample_lost" << std::endl;
}
