/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#ifndef OPENDDS_NO_MULTI_TOPIC

#include "MultiTopicDataReaderBase.h"

#include "DomainParticipantImpl.h"
#include "Marked_Default_Qos.h"
#include "SubscriberImpl.h"
#include "TypeSupportImpl.h"

#include <stdexcept>

namespace {
  struct MatchesIncomingName { // predicate for std::find_if()
    const OPENDDS_STRING& look_for_;
    explicit MatchesIncomingName(const OPENDDS_STRING& s) : look_for_(s) {}
    bool operator()(const OpenDDS::DCPS::MultiTopicImpl::SubjectFieldSpec& sfs)
      const {
      return sfs.incoming_name_ == look_for_;
    }
  };

  class Listener
    : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
  public:
    explicit Listener(OpenDDS::DCPS::MultiTopicDataReaderBase* outer)
      : outer_(outer)
    {}

    void on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/,
      const DDS::RequestedDeadlineMissedStatus& /*status*/){}

    void on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/,
      const DDS::RequestedIncompatibleQosStatus& /*status*/){}

    void on_sample_rejected(DDS::DataReader_ptr /*reader*/,
      const DDS::SampleRejectedStatus& /*status*/){}

    void on_liveliness_changed(DDS::DataReader_ptr /*reader*/,
      const DDS::LivelinessChangedStatus& /*status*/){}

    void on_data_available(DDS::DataReader_ptr reader){
      try {
        outer_->data_available(reader);
      } catch (std::exception& e) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReaderBase::Listener::on_data_available: %C\n"),
          e.what()));
      }
    }

    void on_subscription_matched(DDS::DataReader_ptr /*reader*/,
      const DDS::SubscriptionMatchedStatus& /*status*/){}

    void on_sample_lost(DDS::DataReader_ptr /*reader*/,
      const DDS::SampleLostStatus& /*status*/){}

    /// Increment the reference count.
    virtual void _add_ref (void){
      outer_->_add_ref();
    }

    /// Decrement the reference count.
    virtual void _remove_ref (void){
      outer_->_remove_ref();
    }

    /// Get the refcount
    virtual CORBA::ULong _refcount_value (void) const{
      return outer_->_refcount_value();
    }

  private:
    OpenDDS::DCPS::MultiTopicDataReaderBase* outer_;
  };
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void MultiTopicDataReaderBase::init(const DDS::DataReaderQos& dr_qos,
  DDS::DataReaderListener_ptr a_listener, DDS::StatusMask mask,
  SubscriberImpl* parent, MultiTopicImpl* multitopic)
{
  using namespace std;
  DDS::DataReader_var dr = multitopic->get_type_support()->create_datareader();
  resulting_reader_ = DataReaderEx::_narrow(dr);
  DataReaderImpl* resulting_impl =
    dynamic_cast<DataReaderImpl*>(resulting_reader_.in());

  if (!resulting_impl) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReaderBase::init: ")
      ACE_TEXT("Failed to get DataReaderImpl.\n")));
    return;
  }

  resulting_impl->enable_multi_topic(multitopic);
  resulting_impl->raw_latency_buffer_size() = parent->raw_latency_buffer_size();
  resulting_impl->raw_latency_buffer_type() = parent->raw_latency_buffer_type();

  DDS::DomainParticipant_var participant = parent->get_participant();
  DomainParticipantImpl* dpi = dynamic_cast<DomainParticipantImpl*>(participant.in());
  if (!dpi) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReaderBase::init: ")
      ACE_TEXT("Failed to get DomainParticipantImpl.\n")));
    return;
  }
  resulting_impl->init(multitopic, dr_qos, a_listener, mask, dpi, parent);

  init_typed(resulting_reader_);

  std::map<OPENDDS_STRING, OPENDDS_STRING> fieldToTopic;

  // key: name of field that's a key for the 'join'
  // mapped: set of topicNames that have this key in common
  std::map<OPENDDS_STRING, set<OPENDDS_STRING> > joinKeys;

  listener_.reset(new Listener(this));

  const vector<OPENDDS_STRING>& selection = multitopic->get_selection();
  for (size_t i = 0; i < selection.size(); ++i) {

    const DDS::Duration_t no_wait = {0, 0};
    DDS::Topic_var t = participant->find_topic(selection[i].c_str(), no_wait);
    if (!t.in()) {
      throw runtime_error("Topic: " + selection[i] + " not found.");
    }


    DDS::DataReader_var incoming =
      parent->create_datareader(t, DATAREADER_QOS_USE_TOPIC_QOS,
                                listener_.get(), ALL_STATUS_MASK);
    if (!incoming.in()) {
      throw runtime_error("Could not create incoming DataReader "
        + selection[i]);
    }

    QueryPlan& qp = query_plans_[selection[i]];
    qp.data_reader_ = incoming;
    try {
      const MetaStruct& meta = metaStructFor(incoming);

      for (const char** names = meta.getFieldNames(); *names; ++names) {
        if (fieldToTopic.count(*names)) { // already seen this field name
          set<OPENDDS_STRING>& topics = joinKeys[*names];
          topics.insert(fieldToTopic[*names]);
          topics.insert(selection[i]);
        } else {
          fieldToTopic[*names] = selection[i];
        }
      }
    } catch (const std::runtime_error& e) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReaderBase::init: %C\n"), e.what()));
      throw std::runtime_error("Failed to obtain metastruct for incoming.");
    }
  }

  const vector<SubjectFieldSpec>& aggregation = multitopic->get_aggregation();
  if (aggregation.size() == 0) { // "SELECT * FROM ..."
    const MetaStruct& meta = getResultingMeta();
    for (const char** names = meta.getFieldNames(); *names; ++names) {
      std::map<OPENDDS_STRING, OPENDDS_STRING>::const_iterator found =
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
      std::map<OPENDDS_STRING, OPENDDS_STRING>::const_iterator found =
        fieldToTopic.find(aggregation[i].incoming_name_);
      if (found == fieldToTopic.end()) {
        throw std::runtime_error("Projected field " +
          aggregation[i].incoming_name_ + " has no incoming field.");
      } else {
        query_plans_[found->second].projection_.push_back(aggregation[i]);
      }
    }
  }

  typedef std::map<OPENDDS_STRING, set<OPENDDS_STRING> >::const_iterator iter_t;
  for (iter_t iter = joinKeys.begin(); iter != joinKeys.end(); ++iter) {
    const OPENDDS_STRING& field = iter->first;
    const set<OPENDDS_STRING>& topics = iter->second;
    for (set<OPENDDS_STRING>::const_iterator iter2 = topics.begin();
         iter2 != topics.end(); ++iter2) {
      const OPENDDS_STRING& topic = *iter2;
      QueryPlan& qp = query_plans_[topic];
      if (find_if(qp.projection_.begin(), qp.projection_.end(),
                  MatchesIncomingName(field)) == qp.projection_.end()) {
        qp.keys_projected_out_.push_back(field);
      }
      for (set<OPENDDS_STRING>::const_iterator iter3 = topics.begin();
           iter3 != topics.end(); ++iter3) {
        if (topic != *iter3) { // other topics
          qp.adjacent_joins_.insert(pair<const OPENDDS_STRING, OPENDDS_STRING>(*iter3, field));
        }
      }
    }
  }
}

OPENDDS_STRING MultiTopicDataReaderBase::topicNameFor(DDS::DataReader_ptr reader)
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
  if (tdi) {
    TypeSupportImpl* ts = dynamic_cast<TypeSupportImpl*>(tdi->get_type_support());
    if (ts) {
      return ts->getMetaStructForType();
    }
  }
  throw std::runtime_error("Failed to obtain type support for incoming DataReader");
}

void MultiTopicDataReaderBase::data_available(DDS::DataReader_ptr reader)
{
  using namespace std;
  using namespace DDS;

  const OPENDDS_STRING topic = topicNameFor(reader);
  DataReaderImpl* dri = dynamic_cast<DataReaderImpl*>(reader);
  if (!dri) {
    throw runtime_error("Incoming DataReader for " + topic +
      " could not be cast to DataReaderImpl.");
  }
  DataReaderImpl::GenericBundle gen;
  const ReturnCode_t rc = dri->read_generic(gen,
    NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, false);
  if (rc == RETCODE_NO_DATA) {
    return;
  } else if (rc != RETCODE_OK) {
    throw runtime_error("Incoming DataReader for " + topic +
      " could not be read: " + retcode_to_string(rc));
  }
  try {
    const MetaStruct& meta = metaStructFor(reader);
    const QueryPlan& qp = query_plans_[topic];
    for (CORBA::ULong i = 0; i < gen.samples_.size(); ++i) {
      if (gen.info_[i].valid_data) {
        incoming_sample(gen.samples_[i], gen.info_[i], topic.c_str(), meta);
      } else if (gen.info_[i].instance_state != ALIVE_INSTANCE_STATE) {
        DataReaderImpl* resulting_impl =
          dynamic_cast<DataReaderImpl*>(resulting_reader_.in());
        if (resulting_impl) {
          set<pair<InstanceHandle_t, InstanceHandle_t> >::const_iterator
            iter = qp.instances_.begin();
          while (iter != qp.instances_.end() &&
            iter->first != gen.info_[i].instance_handle) ++iter;
          for (; iter != qp.instances_.end() &&
            iter->first == gen.info_[i].instance_handle; ++iter) {
            resulting_impl->set_instance_state(iter->second,
              gen.info_[i].instance_state);
          }
        } else {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReaderBase::data_available:")
            ACE_TEXT(" failed to obtain DataReaderImpl.\n")));
        }
      }
    }
  } catch (const std::runtime_error& e) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReaderBase::data_available: %C\n"), e.what()));
  }
}

void MultiTopicDataReaderBase::set_status_changed_flag(DDS::StatusKind status,
  bool flag)
{
  DataReaderImpl* dri = dynamic_cast<DataReaderImpl*>(resulting_reader_.in());
  if (dri) {
    dri->set_status_changed_flag(status, flag);
  }
}

bool MultiTopicDataReaderBase::have_sample_states(
  DDS::SampleStateMask sample_states) const
{
  DataReaderImpl* dri = dynamic_cast<DataReaderImpl*>(resulting_reader_.in());
  if (dri) {
    return dri->have_sample_states(sample_states);
  } else {
    return false;
  }
}

void MultiTopicDataReaderBase::cleanup()
{
  DDS::Subscriber_var sub = resulting_reader_->get_subscriber();
  for (std::map<OPENDDS_STRING, QueryPlan>::iterator it = query_plans_.begin();
       it != query_plans_.end(); ++it) {
    sub->delete_datareader(it->second.data_reader_);
  }
  DataReaderImpl* dri = dynamic_cast<DataReaderImpl*>(resulting_reader_.in());
  SubscriberImpl* si = dynamic_cast<SubscriberImpl*>(sub.in());
  if (dri) {
    if (si) {
      si->remove_from_datareader_set(dri);
    }
    dri->cleanup();
  }
}

DDS::InstanceHandle_t MultiTopicDataReaderBase::get_instance_handle()
{
  return resulting_reader_->get_instance_handle();
}

DDS::ReturnCode_t MultiTopicDataReaderBase::enable()
{
  return resulting_reader_->enable();
}

DDS::StatusCondition_ptr MultiTopicDataReaderBase::get_statuscondition()
{
  return resulting_reader_->get_statuscondition();
}

DDS::StatusMask MultiTopicDataReaderBase::get_status_changes()
{
  return resulting_reader_->get_status_changes();
}

DDS::ReadCondition_ptr MultiTopicDataReaderBase::create_readcondition(
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
{
  return resulting_reader_->create_readcondition(sample_states, view_states,
    instance_states);
}

#ifndef OPENDDS_NO_QUERY_CONDITION
DDS::QueryCondition_ptr MultiTopicDataReaderBase::create_querycondition(
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states, const char* query_expression,
  const DDS::StringSeq& query_parameters)
{
  return resulting_reader_->create_querycondition(sample_states, view_states,
    instance_states, query_expression, query_parameters);
}
#endif

DDS::ReturnCode_t MultiTopicDataReaderBase::delete_readcondition(
  DDS::ReadCondition_ptr a_condition)
{
  return resulting_reader_->delete_readcondition(a_condition);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::delete_contained_entities()
{
  return resulting_reader_->delete_contained_entities();
}

DDS::ReturnCode_t MultiTopicDataReaderBase::set_qos(
  const DDS::DataReaderQos& qos)
{
  return resulting_reader_->set_qos(qos);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_qos(DDS::DataReaderQos& qos)
{
  return resulting_reader_->get_qos(qos);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::set_listener(
  DDS::DataReaderListener_ptr a_listener, DDS::StatusMask mask)
{
  return resulting_reader_->set_listener(a_listener, mask);
}

DDS::DataReaderListener_ptr MultiTopicDataReaderBase::get_listener()
{
  return resulting_reader_->get_listener();
}

DDS::TopicDescription_ptr MultiTopicDataReaderBase::get_topicdescription()
{
  return resulting_reader_->get_topicdescription();
}

DDS::Subscriber_ptr MultiTopicDataReaderBase::get_subscriber()
{
  return resulting_reader_->get_subscriber();
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_sample_rejected_status(
  DDS::SampleRejectedStatus& status)
{
  return resulting_reader_->get_sample_rejected_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_liveliness_changed_status(
  DDS::LivelinessChangedStatus& status)
{
  return resulting_reader_->get_liveliness_changed_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_requested_deadline_missed_status(
  DDS::RequestedDeadlineMissedStatus& status)
{
  return resulting_reader_->get_requested_deadline_missed_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_requested_incompatible_qos_status(
  DDS::RequestedIncompatibleQosStatus& status)
{
  return resulting_reader_->get_requested_incompatible_qos_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_subscription_matched_status(
  DDS::SubscriptionMatchedStatus& status)
{
  return resulting_reader_->get_subscription_matched_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_sample_lost_status(
  DDS::SampleLostStatus& status)
{
  return resulting_reader_->get_sample_lost_status(status);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::wait_for_historical_data(
  const DDS::Duration_t& max_wait)
{
  return resulting_reader_->wait_for_historical_data(max_wait);
}

DDS::ReturnCode_t MultiTopicDataReaderBase::get_matched_publications(
  DDS::InstanceHandleSeq& publication_handles)
{
  return resulting_reader_->get_matched_publications(publication_handles);
}

#ifndef DDS_HAS_MINIMUM_BIT
DDS::ReturnCode_t MultiTopicDataReaderBase::get_matched_publication_data(
  DDS::PublicationBuiltinTopicData& publication_data,
  DDS::InstanceHandle_t publication_handle)
{
  return resulting_reader_->get_matched_publication_data(publication_data,
    publication_handle);
}
#endif

void MultiTopicDataReaderBase::get_latency_stats(LatencyStatisticsSeq& stats)
{
  resulting_reader_->get_latency_stats(stats);
}

void MultiTopicDataReaderBase::reset_latency_stats()
{
  resulting_reader_->reset_latency_stats();
}

CORBA::Boolean MultiTopicDataReaderBase::statistics_enabled()
{
  return resulting_reader_->statistics_enabled();
}

void MultiTopicDataReaderBase::statistics_enabled(
  CORBA::Boolean statistics_enabled)
{
  resulting_reader_->statistics_enabled(statistics_enabled);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
