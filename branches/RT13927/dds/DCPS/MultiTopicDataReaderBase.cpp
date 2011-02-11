/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "MultiTopicDataReaderBase.h"
#include "SubscriberImpl.h"
#include "DomainParticipantImpl.h"
#include "TypeSupportImpl.h"

#include <stdexcept>
#include <sstream>

namespace {
  struct MatchesIncomingName { // predicate for std::find_if()
    const std::string& look_for_;
    explicit MatchesIncomingName(const std::string& s) : look_for_(s) {}
    bool operator()(const OpenDDS::DCPS::MultiTopicImpl::SubjectFieldSpec& sfs)
      const {
      return sfs.incoming_name_ == look_for_;
    }
  };
}


namespace OpenDDS {
namespace DCPS {

void MultiTopicDataReaderBase::init(const DDS::DataReaderQos& dr_qos,
  const DataReaderQosExt& ext_qos, DDS::DataReaderListener_ptr a_listener,
  DDS::StatusMask mask, SubscriberImpl* parent, MultiTopicImpl* multitopic)
{
  using namespace std;
  DDS::DataReader_var dr = multitopic->get_type_support()->create_datareader();
  resulting_reader_ = DataReaderEx::_narrow(dr);
  DataReaderImpl* resulting_impl =
    dynamic_cast<DataReaderImpl*>(resulting_reader_.in());

  resulting_impl->raw_latency_buffer_size() = parent->raw_latency_buffer_size();
  resulting_impl->raw_latency_buffer_type() = parent->raw_latency_buffer_type();

  DDS::DomainParticipant_var participant = parent->get_participant();
  resulting_impl->init(multitopic, dr_qos, ext_qos, a_listener, mask,
    dynamic_cast<DomainParticipantImpl*>(participant.in()), parent,
    resulting_reader_, 0 /*no remote object*/);

  init_typed(resulting_reader_);
  listener_ = new Listener(this);

  std::map<string, string> fieldToTopic;

  // key: name of field that's a key for the 'join'
  // mapped: set of topicNames that have this key in common
  std::map<string, set<string> > joinKeys;

  const vector<string>& selection = multitopic->get_selection();
  for (size_t i = 0; i < selection.size(); ++i) {

    const DDS::Duration_t no_wait = {0, 0};
    DDS::Topic_var t = participant->find_topic(selection[i].c_str(), no_wait);
    if (!t.in()) {
      throw runtime_error("Topic: " + selection[i] + " not found.");
    }

    DDS::DataReader_var incoming = parent->create_opendds_datareader(t, dr_qos,
      ext_qos, listener_, ALL_STATUS_MASK);
    if (!incoming.in()) {
      throw runtime_error("Could not create incoming DataReader "
        + selection[i]);
    }

    QueryPlan& qp = query_plans_[selection[i]];
    qp.data_reader_ = incoming;
    const MetaStruct& meta = metaStructFor(incoming);

    for (const char** names = meta.getFieldNames(); *names; ++names) {
      if (fieldToTopic.count(*names)) { // already seen this field name
        set<string>& topics = joinKeys[*names];
        topics.insert(fieldToTopic[*names]);
        topics.insert(selection[i]);
      } else {
        fieldToTopic[*names] = selection[i];
      }
    }
  }

  const vector<SubjectFieldSpec>& aggregation = multitopic->get_aggregation();
  if (aggregation.size() == 0) { // "SELECT * FROM ..."
    const MetaStruct& meta = getResultingMeta();
    for (const char** names = meta.getFieldNames(); *names; ++names) {
      std::map<string, string>::const_iterator found =
        fieldToTopic.find(*names);
      if (found == fieldToTopic.end()) {
        if (DCPS_debug_level > 1) {
          ACE_DEBUG((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ")
                     ACE_TEXT("MultiTopicDataReaderBase::init(), in SELECT * ")
                     ACE_TEXT("resulting field %C has no corresponding ")
                     ACE_TEXT("incoming field.\n"), *names));
        }
      } else {
        query_plans_[found->second].projection_.push_back(
          SubjectFieldSpec(*names));
      }
    }
  } else { // "SELECT A, B FROM ..."
    for (size_t i = 0; i < aggregation.size(); ++i) {
      std::map<string, string>::const_iterator found =
        fieldToTopic.find(aggregation[i].incoming_name_);
      if (found == fieldToTopic.end()) {
        throw std::runtime_error("Projected field " +
          aggregation[i].incoming_name_ + " has no incoming field.");
      } else {
        query_plans_[found->second].projection_.push_back(aggregation[i]);
      }
    }
  }

  typedef std::map<string, set<string> >::const_iterator iter_t;
  for (iter_t iter = joinKeys.begin(); iter != joinKeys.end(); ++iter) {
    const string& field = iter->first;
    const set<string>& topics = iter->second;
    for (set<string>::const_iterator iter2 = topics.begin();
         iter2 != topics.end(); ++iter2) {
      const string& topic = *iter2;
      QueryPlan& qp = query_plans_[topic];
      if (find_if(qp.projection_.begin(), qp.projection_.end(),
                  MatchesIncomingName(field)) == qp.projection_.end()) {
        qp.keys_projected_out_.push_back(field);
      }
      for (set<string>::const_iterator iter3 = topics.begin();
           iter3 != topics.end(); ++iter3) {
        if (topic != *iter3) { // other topics
          qp.adjacent_joins_.insert(pair<const string, string>(*iter3, field));
        }
      }
    }
  }
}

std::string MultiTopicDataReaderBase::topicNameFor(DDS::DataReader_ptr reader)
{
  DDS::TopicDescription_var td = reader->get_topicdescription();
  CORBA::String_var topic = td->get_name();
  return topic.in();
}

const MetaStruct&
MultiTopicDataReaderBase::metaStructFor(DDS::DataReader_ptr reader)
{
  DDS::TopicDescription_var td = reader->get_topicdescription();
  TopicDescriptionImpl* tdi = dynamic_cast<TopicDescriptionImpl*>(td.in());
  TypeSupportImpl* ts = dynamic_cast<TypeSupportImpl*>(tdi->get_type_support());
  return ts->getMetaStructForType();
}

void MultiTopicDataReaderBase::data_available(DDS::DataReader_ptr reader)
{
  using namespace std;
  using namespace DDS;

  const string topic = topicNameFor(reader);
  DataReaderImpl* dri = dynamic_cast<DataReaderImpl*>(reader);
  DataReaderImpl::GenericBundle gen;
  ReturnCode_t rc = dri->read_generic(gen, NOT_READ_SAMPLE_STATE,
                                      ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  if (rc == RETCODE_NO_DATA) {
    return;
  } else if (rc != RETCODE_OK) {
    ostringstream rc_ss;
    rc_ss << rc;
    throw runtime_error("Incoming DataReader for " + topic +
      " could not be read, error #" + rc_ss.str());
  }

  const MetaStruct& meta = metaStructFor(reader);
  const QueryPlan& qp = query_plans_[topic];
  for (CORBA::ULong i = 0; i < gen.samples_.size(); ++i) {
    if (gen.info_[i].valid_data) {
      incoming_sample(gen.samples_[i], gen.info_[i], topic.c_str(), meta);
    } else if (gen.info_[i].instance_state != ALIVE_INSTANCE_STATE) {
      DataReaderImpl* resulting_impl =
        dynamic_cast<DataReaderImpl*>(resulting_reader_.in());
      set<pair<InstanceHandle_t, InstanceHandle_t> >::const_iterator
        iter = qp.instances_.begin();
      while (iter != qp.instances_.end() &&
             iter->first != gen.info_[i].instance_handle) ++iter;
      for (; iter != qp.instances_.end() &&
           iter->first == gen.info_[i].instance_handle; ++iter) {
        resulting_impl->set_instance_state(iter->second,
                                           gen.info_[i].instance_state);
      }
    }
  }
}

void MultiTopicDataReaderBase::Listener::on_requested_deadline_missed(
  DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&)
  ACE_THROW_SPEC((CORBA::SystemException))
{
}

void MultiTopicDataReaderBase::Listener::on_requested_incompatible_qos(
  DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&)
  ACE_THROW_SPEC((CORBA::SystemException))
{
}

void MultiTopicDataReaderBase::Listener::on_sample_rejected(
  DDS::DataReader_ptr, const DDS::SampleRejectedStatus&)
  ACE_THROW_SPEC((CORBA::SystemException))
{
}

void MultiTopicDataReaderBase::Listener::on_liveliness_changed(
  DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&)
  ACE_THROW_SPEC((CORBA::SystemException))
{
}

void MultiTopicDataReaderBase::Listener::on_data_available(
  DDS::DataReader_ptr reader)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  try {
    outer_->data_available(reader);
  } catch (std::exception& e) {
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) MultiTopicDataReaderBase::Listener::"
                 "on_data_available(): %C", e.what()));
    }
  }
}

void MultiTopicDataReaderBase::Listener::on_subscription_matched(
  DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&)
  ACE_THROW_SPEC((CORBA::SystemException))
{
}

void MultiTopicDataReaderBase::Listener::on_sample_lost(DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
  ACE_THROW_SPEC((CORBA::SystemException))
{
}

void MultiTopicDataReaderBase::set_status_changed_flag(DDS::StatusKind status,
  bool flag)
{
  dynamic_cast<DataReaderImpl*>(resulting_reader_.in())
    ->set_status_changed_flag(status, flag);
}

bool MultiTopicDataReaderBase::have_sample_states(
  DDS::SampleStateMask sample_states) const
{
  return dynamic_cast<DataReaderImpl*>(resulting_reader_.in())
    ->have_sample_states(sample_states);
}

void MultiTopicDataReaderBase::cleanup()
{
  for (std::map<std::string, QueryPlan>::iterator it = query_plans_.begin();
       it != query_plans_.end(); ++it) {
    it->second.data_reader_->set_listener(0, ALL_STATUS_MASK);
  }
  dynamic_cast<DataReaderImpl*>(resulting_reader_.in())->cleanup();
}

DDS::InstanceHandle_t MultiTopicDataReaderBase::get_instance_handle()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_instance_handle();
}

DDS::ReturnCode_t MultiTopicDataReaderBase::enable()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->enable();
}

DDS::StatusCondition_ptr MultiTopicDataReaderBase::get_statuscondition()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_statuscondition();
}

DDS::StatusMask MultiTopicDataReaderBase::get_status_changes()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_status_changes();
}

DDS::ReadCondition_ptr MultiTopicDataReaderBase::create_readcondition(
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->create_readcondition(sample_states, view_states,
    instance_states);
}

DDS::QueryCondition_ptr MultiTopicDataReaderBase::create_querycondition(
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states, const char* query_expression,
  const DDS::StringSeq& query_parameters)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->create_querycondition(sample_states, view_states,
    instance_states, query_expression, query_parameters);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::delete_readcondition(
  DDS::ReadCondition_ptr a_condition)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->delete_readcondition(a_condition);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::delete_contained_entities()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->delete_contained_entities();
}

DDS::ReturnCode_t MultiTopicDataReaderBase::set_qos(
  const DDS::DataReaderQos& qos)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->set_qos(qos);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_qos(DDS::DataReaderQos& qos)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_qos(qos);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::set_listener(
  DDS::DataReaderListener_ptr a_listener, DDS::StatusMask mask)
 ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->set_listener(a_listener, mask);
}

DDS::DataReaderListener_ptr MultiTopicDataReaderBase::get_listener()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_listener();
}

DDS::TopicDescription_ptr MultiTopicDataReaderBase::get_topicdescription()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_topicdescription();
}

DDS::Subscriber_ptr MultiTopicDataReaderBase::get_subscriber()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_subscriber();
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_sample_rejected_status(
  DDS::SampleRejectedStatus& status)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_sample_rejected_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_liveliness_changed_status(
  DDS::LivelinessChangedStatus& status)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_liveliness_changed_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_requested_deadline_missed_status(
  DDS::RequestedDeadlineMissedStatus& status)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_requested_deadline_missed_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_requested_incompatible_qos_status(
  DDS::RequestedIncompatibleQosStatus& status)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_requested_incompatible_qos_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_subscription_matched_status(
  DDS::SubscriptionMatchedStatus& status)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_subscription_matched_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_sample_lost_status(
  DDS::SampleLostStatus& status)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_sample_lost_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::wait_for_historical_data(
  const DDS::Duration_t& max_wait)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->wait_for_historical_data(max_wait);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_matched_publications(
  DDS::InstanceHandleSeq& publication_handles)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_matched_publications(publication_handles);
}

#ifndef DDS_HAS_MINIMUM_BIT
DDS::ReturnCode_t MultiTopicDataReaderBase::get_matched_publication_data(
  DDS::PublicationBuiltinTopicData& publication_data,
  DDS::InstanceHandle_t publication_handle)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->get_matched_publication_data(publication_data,
    publication_handle);
}
#endif

void MultiTopicDataReaderBase::get_latency_stats(LatencyStatisticsSeq& stats)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  resulting_reader_->get_latency_stats(stats);
}

void MultiTopicDataReaderBase::reset_latency_stats()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  resulting_reader_->reset_latency_stats();
}

CORBA::Boolean MultiTopicDataReaderBase::statistics_enabled()
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return resulting_reader_->statistics_enabled();
}

void MultiTopicDataReaderBase::statistics_enabled(
  CORBA::Boolean statistics_enabled)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  resulting_reader_->statistics_enabled(statistics_enabled);
}

}
}

#endif
