/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "BuiltInTopicUtils.h"

#include "BuiltInTopicDataReaderImpls.h"
#include "BitPubListenerImpl.h"
#include "Logging.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const char* const BUILT_IN_PARTICIPANT_TOPIC = "DCPSParticipant";
const char* const BUILT_IN_PARTICIPANT_TOPIC_TYPE = "PARTICIPANT_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_TOPIC_TOPIC = "DCPSTopic";
const char* const BUILT_IN_TOPIC_TOPIC_TYPE = "TOPIC_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_SUBSCRIPTION_TOPIC = "DCPSSubscription";
const char* const BUILT_IN_SUBSCRIPTION_TOPIC_TYPE = "SUBSCRIPTION_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_PUBLICATION_TOPIC = "DCPSPublication";
const char* const BUILT_IN_PUBLICATION_TOPIC_TYPE = "PUBLICATION_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_PARTICIPANT_LOCATION_TOPIC = "OpenDDSParticipantLocation";
const char* const BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE = "PARTICIPANT_LOCATION_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_CONNECTION_RECORD_TOPIC = "OpenDDSConnectionRecord";
const char* const BUILT_IN_CONNECTION_RECORD_TOPIC_TYPE = "CONNECTION_RECORD_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_INTERNAL_THREAD_TOPIC = "OpenDDSInternalThread";
const char* const BUILT_IN_INTERNAL_THREAD_TOPIC_TYPE = "INTERNAL_THREAD_BUILT_IN_TOPIC_TYPE";

DDS::InstanceHandle_t BitSubscriber::add_participant(const DDS::ParticipantBuiltinTopicData& part,
                                                     DDS::ViewStateKind view_state)
{
  return add_i<ParticipantBuiltinTopicDataDataReaderImpl>(BUILT_IN_PARTICIPANT_TOPIC, part, view_state);
}

void BitSubscriber::remove_participant(DDS::InstanceHandle_t part_ih,
                                       DDS::InstanceHandle_t loc_ih)
{
#ifndef DDS_HAS_MINIMUM_BIT
  remove_i(BUILT_IN_PARTICIPANT_TOPIC, part_ih);
  remove_i(BUILT_IN_PARTICIPANT_LOCATION_TOPIC, loc_ih);
#else
  ACE_UNUSED_ARG(part_ih);
  ACE_UNUSED_ARG(loc_ih);
#endif
}

DDS::ReturnCode_t BitSubscriber::get_discovered_participant_data(DDS::ParticipantBuiltinTopicData& participant_data,
                                                                 DDS::InstanceHandle_t participant_handle)
{
#ifndef DDS_HAS_MINIMUM_BIT
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, DDS::RETCODE_NO_DATA);

  if (!bit_subscriber_) {
    return DDS::RETCODE_NO_DATA;
  }

  DDS::SampleInfoSeq info;
  DDS::ParticipantBuiltinTopicDataSeq data;
  DDS::DataReader_var dr = bit_subscriber_->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);
  DDS::ParticipantBuiltinTopicDataDataReader_var bit_part_dr =
    DDS::ParticipantBuiltinTopicDataDataReader::_narrow(dr);

  const DDS::ReturnCode_t ret = bit_part_dr->read_instance(data,
                                                           info,
                                                           1,
                                                           participant_handle,
                                                           DDS::ANY_SAMPLE_STATE,
                                                           DDS::ANY_VIEW_STATE,
                                                           DDS::ANY_INSTANCE_STATE);

  if (ret == DDS::RETCODE_OK) {
    if (info[0].valid_data) {
      participant_data = data[0];
    } else {
      return DDS::RETCODE_NO_DATA;
    }
  }

  return ret;
#else
  ACE_UNUSED_ARG(participant_data);
  ACE_UNUSED_ARG(participant_handle);
  return DDS::RETCODE_NO_DATA;
#endif
}

DDS::ReturnCode_t BitSubscriber::get_discovered_topic_data(DDS::TopicBuiltinTopicData& topic_data,
                                                           DDS::InstanceHandle_t topic_handle)
{
#ifndef DDS_HAS_MINIMUM_BIT
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, DDS::RETCODE_NO_DATA);

  if (!bit_subscriber_) {
    return DDS::RETCODE_NO_DATA;
  }

  DDS::SampleInfoSeq info;
  DDS::TopicBuiltinTopicDataSeq data;
  DDS::DataReader_var dr = bit_subscriber_->lookup_datareader(BUILT_IN_TOPIC_TOPIC);
  DDS::TopicBuiltinTopicDataDataReader_var bit_topic_dr =
    DDS::TopicBuiltinTopicDataDataReader::_narrow(dr);

  const DDS::ReturnCode_t ret = bit_topic_dr->read_instance(data,
                                                            info,
                                                            1,
                                                            topic_handle,
                                                            DDS::ANY_SAMPLE_STATE,
                                                            DDS::ANY_VIEW_STATE,
                                                            DDS::ANY_INSTANCE_STATE);

  if (ret == DDS::RETCODE_OK) {
    if (info[0].valid_data) {
      topic_data = data[0];
    } else {
      return DDS::RETCODE_NO_DATA;
    }
  }

  return ret;
#else
  ACE_UNUSED_ARG(topic_data);
  ACE_UNUSED_ARG(topic_handle);
  return DDS::RETCODE_NO_DATA;
#endif
}

DDS::InstanceHandle_t BitSubscriber::add_publication(const DDS::PublicationBuiltinTopicData& pub,
                                                     DDS::ViewStateKind view_state)
{
  return add_i<PublicationBuiltinTopicDataDataReaderImpl>(BUILT_IN_PUBLICATION_TOPIC, pub, view_state);
}

void BitSubscriber::remove_publication(DDS::InstanceHandle_t pub_ih)
{
  remove_i(BUILT_IN_PUBLICATION_TOPIC, pub_ih);
}

DDS::InstanceHandle_t BitSubscriber::add_subscription(const DDS::SubscriptionBuiltinTopicData& sub,
                                                      DDS::ViewStateKind view_state)
{
  return add_i<SubscriptionBuiltinTopicDataDataReaderImpl>(BUILT_IN_SUBSCRIPTION_TOPIC, sub, view_state);
}

void BitSubscriber::remove_subscription(DDS::InstanceHandle_t sub_ih)
{
  remove_i(BUILT_IN_SUBSCRIPTION_TOPIC, sub_ih);
}

DDS::InstanceHandle_t BitSubscriber::add_participant_location(const ParticipantLocationBuiltinTopicData& loc,
                                                              DDS::ViewStateKind view_state)
{
  return add_i<ParticipantLocationBuiltinTopicDataDataReaderImpl>(BUILT_IN_PARTICIPANT_LOCATION_TOPIC, loc, view_state);
}

DDS::InstanceHandle_t BitSubscriber::add_connection_record(const ConnectionRecord& cr,
                                                           DDS::ViewStateKind view_state)
{
  return add_i<ConnectionRecordDataReaderImpl>(BUILT_IN_CONNECTION_RECORD_TOPIC, cr, view_state);
}

void BitSubscriber::remove_connection_record(const ConnectionRecord& cr)
{
#ifndef DDS_HAS_MINIMUM_BIT
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (!bit_subscriber_) {
    return;
  }

  DDS::DataReader_var d = bit_subscriber_->lookup_datareader(BUILT_IN_CONNECTION_RECORD_TOPIC);

  if (!d) {
    return;
  }

  ConnectionRecordDataReaderImpl* bit = dynamic_cast<ConnectionRecordDataReaderImpl*>(d.in());
  if (!bit) {
    return;
  }

  bit->set_instance_state(bit->lookup_instance(cr), DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
#else
  ACE_UNUSED_ARG(cr);
#endif
}

DDS::InstanceHandle_t BitSubscriber::add_thread_status(const InternalThreadBuiltinTopicData& ts,
                                                       DDS::ViewStateKind view_state,
                                                       const SystemTimePoint& timestamp)
{
#ifndef DDS_HAS_MINIMUM_BIT
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, DDS::HANDLE_NIL);

  if (!bit_subscriber_) {
    return DDS::HANDLE_NIL;
  }

  DDS::DataReader_var d = bit_subscriber_->lookup_datareader(BUILT_IN_INTERNAL_THREAD_TOPIC);
  if (!d) {
    return DDS::HANDLE_NIL;
  }

  InternalThreadBuiltinTopicDataDataReaderImpl* bit = dynamic_cast<InternalThreadBuiltinTopicDataDataReaderImpl*>(d.in());
  if (!bit) {
    return DDS::HANDLE_NIL;
  }

  return bit->store_synthetic_data(ts, view_state, timestamp);
#else
  ACE_UNUSED_ARG(ts);
  ACE_UNUSED_ARG(view_state);
  ACE_UNUSED_ARG(timestamp);
  return DDS::HANDLE_NIL;
#endif /* DDS_HAS_MINIMUM_BIT */
}

void BitSubscriber::remove_thread_status(const InternalThreadBuiltinTopicData& ts)
{
#ifndef DDS_HAS_MINIMUM_BIT
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (!bit_subscriber_) {
    return;
  }

  DDS::DataReader_var d = bit_subscriber_->lookup_datareader(BUILT_IN_INTERNAL_THREAD_TOPIC);

  if (!d) {
    return;
  }

  InternalThreadBuiltinTopicDataDataReaderImpl* bit = dynamic_cast<InternalThreadBuiltinTopicDataDataReaderImpl*>(d.in());
  if (!bit) {
    return;
  }

  bit->set_instance_state(bit->lookup_instance(ts), DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
#else
  ACE_UNUSED_ARG(ts);
#endif
}

void BitSubscriber::bit_pub_listener_hack(DomainParticipantImpl* participant)
{
#ifndef DDS_HAS_MINIMUM_BIT
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (!bit_subscriber_) {
    return;
  }

  DDS::DataReader_var dr =
    bit_subscriber_->lookup_datareader(BUILT_IN_PUBLICATION_TOPIC);
  DDS::PublicationBuiltinTopicDataDataReader_var bit_pub_dr =
    DDS::PublicationBuiltinTopicDataDataReader::_narrow(dr);

  if (bit_pub_dr) {
    DDS::DataReaderListener_var listener = bit_pub_dr->get_listener();
    if (!listener) {
      DDS::DataReaderListener_var bit_pub_listener =
        new BitPubListenerImpl(participant);
      bit_pub_dr->set_listener(bit_pub_listener, DDS::DATA_AVAILABLE_STATUS);
      // Must call on_data_available when attaching a listener late - samples may be waiting
      DataReaderImpl* reader = dynamic_cast<DataReaderImpl*>(bit_pub_dr.in());
      if (!reader) {
        return;
      }
      TheServiceParticipant->job_queue()->enqueue(make_rch<DataReaderImpl::OnDataAvailable>(bit_pub_listener, rchandle_from(reader), true, false, false));
    }
  }
#else
  ACE_UNUSED_ARG(participant);
#endif
}

template <typename DataReaderImpl, typename Sample>
DDS::InstanceHandle_t BitSubscriber::add_i(const char* topic_name,
                                           const Sample& sample,
                                           DDS::ViewStateKind view_state)
{
#ifndef DDS_HAS_MINIMUM_BIT
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, DDS::HANDLE_NIL);

  if (!bit_subscriber_) {
    if (log_bits) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: BitSubscriber::add_i: %@ bit_subscriber_ is null for topic %C, returning nil\n", this, topic_name));
    }
    return DDS::HANDLE_NIL;
  }

  DDS::DataReader_var d = bit_subscriber_->lookup_datareader(topic_name);
  if (!d) {
    if (log_bits) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: BitSubscriber::add_i: %@ DataReader is null for topic %C, returning nil\n", this, topic_name));
    }
    return DDS::HANDLE_NIL;
  }

  DataReaderImpl* bit = dynamic_cast<DataReaderImpl*>(d.in());
  if (!bit) {
    if (log_bits) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: BitSubscriber::add_i: %@ dynamic_cast failed for topic %C, returning nil\n", this, topic_name));
    }
    return DDS::HANDLE_NIL;
  }

  const DDS::InstanceHandle_t ih = bit->store_synthetic_data(sample, view_state);
  if (log_bits) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: BitSubscriber::add_i: %@ returning instance handle %d for topic %C\n", this, ih, topic_name));
  }
  return ih;
#else
  if (log_bits) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: BitSubscriber::add_i: %@ DDS_HAS_MINIMUM_BIT is not defined, returning nil\n", this, topic_name));
  }
  ACE_UNUSED_ARG(sample);
  ACE_UNUSED_ARG(view_state);
  return DDS::HANDLE_NIL;
#endif /* DDS_HAS_MINIMUM_BIT */
}

void BitSubscriber::remove_i(const char* topic_name,
                             DDS::InstanceHandle_t ih)
{
#ifndef DDS_HAS_MINIMUM_BIT
  if (ih != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (!bit_subscriber_) {
      return;
    }

    DDS::DataReader_var d = bit_subscriber_->lookup_datareader(topic_name);
    if (!d) {
      return;
    }

    DataReaderImpl* bit = dynamic_cast<DataReaderImpl*>(d.in());
    if (!bit) {
      return;
    }
    bit->set_instance_state(ih, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  }
#else
  ACE_UNUSED_ARG(topic_name);
  ACE_UNUSED_ARG(ih);
#endif
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
