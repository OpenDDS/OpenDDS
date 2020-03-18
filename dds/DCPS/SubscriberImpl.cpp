/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "debug.h"
#include "SubscriberImpl.h"
#include "FeatureDisabledQosCheck.h"
#include "DomainParticipantImpl.h"
#include "Qos_Helper.h"
#include "GuidConverter.h"
#include "TopicImpl.h"
#include "MonitorFactory.h"
#include "DataReaderImpl.h"
#include "Service_Participant.h"
#include "dds/DdsDcpsTypeSupportExtC.h"
#include "TopicDescriptionImpl.h"
#include "Marked_Default_Qos.h"
#include "Transient_Kludge.h"
#include "ContentFilteredTopicImpl.h"
#include "MultiTopicImpl.h"
#include "GroupRakeData.h"
#include "MultiTopicDataReaderBase.h"
#include "Util.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/DataLinkSet.h"

#include "tao/debug.h"

#include "ace/Auto_Ptr.h"
#include "ace/Vector_T.h"

#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

SubscriberImpl::SubscriberImpl(DDS::InstanceHandle_t       handle,
                               const DDS::SubscriberQos &  qos,
                               DDS::SubscriberListener_ptr a_listener,
                               const DDS::StatusMask&      mask,
                               DomainParticipantImpl*      participant)
  : handle_(handle),
  qos_(qos),
  default_datareader_qos_(TheServiceParticipant->initial_DataReaderQos()),
  listener_mask_(mask),
  participant_(*participant),
  domain_id_(participant->get_domain_id()),
  raw_latency_buffer_size_(0),
  raw_latency_buffer_type_(DataCollector<double>::KeepOldest),
  access_depth_ (0)
{
  //Note: OK to duplicate a nil.
  listener_ = DDS::SubscriberListener::_duplicate(a_listener);

  monitor_.reset(TheServiceParticipant->monitor_factory_->create_subscriber_monitor(this));
}

SubscriberImpl::~SubscriberImpl()
{
  // The datareaders should be deleted already before calling delete
  // subscriber.
  if (!is_clean()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("SubscriberImpl::~SubscriberImpl, ")
               ACE_TEXT("%B datareaders still exist.\n"),
               datareader_map_.size ()));
  }
}

DDS::InstanceHandle_t
SubscriberImpl::get_instance_handle()
{
  return handle_;
}

bool
SubscriberImpl::contains_reader(DDS::InstanceHandle_t a_handle)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->si_lock_,
                   false);

  for (DataReaderMap::iterator it(datareader_map_.begin());
       it != datareader_map_.end(); ++it) {
    if (a_handle == it->second->get_instance_handle()) {
      return true;
    }
  }

  return false;
}

DDS::DataReader_ptr
SubscriberImpl::create_datareader(
  DDS::TopicDescription_ptr   a_topic_desc,
  const DDS::DataReaderQos &  qos,
  DDS::DataReaderListener_ptr a_listener,
  DDS::StatusMask             mask)
{
  if (CORBA::is_nil(a_topic_desc)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("SubscriberImpl::create_datareader, ")
               ACE_TEXT("topic desc is nil.\n")));
    return DDS::DataReader::_nil();
  }

  DDS::DataReaderQos dr_qos;
  RcHandle<DomainParticipantImpl> participant = this->participant_.lock();
  if (!participant)
    return DDS::DataReader::_nil();

  TopicImpl* topic_servant = dynamic_cast<TopicImpl*>(a_topic_desc);

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  ContentFilteredTopicImpl* cft = 0;
#endif
#ifndef OPENDDS_NO_MULTI_TOPIC
  MultiTopicImpl* mt = 0;
#else
  bool mt = false;
#endif

  if (!topic_servant) {
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    cft = dynamic_cast<ContentFilteredTopicImpl*>(a_topic_desc);
    if (cft) {
      DDS::Topic_var related;
      related = cft->get_related_topic();
      topic_servant = dynamic_cast<TopicImpl*>(related.in());
    }
    else
#endif
    {
#ifndef OPENDDS_NO_MULTI_TOPIC
      mt = dynamic_cast<MultiTopicImpl*>(a_topic_desc);
#endif
    }
  }

  if (!validate_datareader_qos (qos, default_datareader_qos_, topic_servant, dr_qos, mt))
    return DDS::DataReader::_nil();

#ifndef OPENDDS_NO_MULTI_TOPIC
  if (mt) {
    try {
      DDS::DataReader_var dr =
        mt->get_type_support()->create_multitopic_datareader();
      MultiTopicDataReaderBase* mtdr =
        dynamic_cast<MultiTopicDataReaderBase*>(dr.in());
      mtdr->init(dr_qos, a_listener, mask, this, mt);
      if (enabled_.value() && qos_.entity_factory.autoenable_created_entities) {
        if (dr->enable() != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: ")
                     ACE_TEXT("SubscriberImpl::create_datareader, ")
                     ACE_TEXT("enable of MultiTopicDataReader failed.\n")));
          return DDS::DataReader::_nil();
        }
        multitopic_reader_enabled(dr);
      }
      return dr._retn();
    } catch (const std::exception& e) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("SubscriberImpl::create_datareader, ")
                 ACE_TEXT("creation of MultiTopicDataReader failed: %C.\n"),
                 e.what()));
    }
    return DDS::DataReader::_nil();
  }
#endif

  OpenDDS::DCPS::TypeSupport_ptr typesupport =
    topic_servant->get_type_support();

  if (0 == typesupport) {
    CORBA::String_var name = a_topic_desc->get_name();
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("SubscriberImpl::create_datareader, ")
               ACE_TEXT("typesupport(topic_name=%C) is nil.\n"),
               name.in()));
    return DDS::DataReader::_nil();
  }

  DDS::DataReader_var dr_obj = typesupport->create_datareader();

  DataReaderImpl* dr_servant =
    dynamic_cast<DataReaderImpl*>(dr_obj.in());

  if (dr_servant == 0) {
    ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("SubscriberImpl::create_datareader, ")
        ACE_TEXT("servant is nil.\n")));
    return DDS::DataReader::_nil();
  }

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  if (cft) {
    dr_servant->enable_filtering(cft);
  }
#endif

  // Propagate the latency buffer data collection configuration.
  // @TODO: Determine whether we want to exclude the Builtin Topic
  //        readers from data gathering.
  dr_servant->raw_latency_buffer_size() = this->raw_latency_buffer_size_;
  dr_servant->raw_latency_buffer_type() = this->raw_latency_buffer_type_;


  dr_servant->init(topic_servant,
                   dr_qos,
                   a_listener,
                   mask,
                   participant.in(),
                   this);

  if ((this->enabled_ == true) && (qos_.entity_factory.autoenable_created_entities)) {
    const DDS::ReturnCode_t ret = dr_servant->enable();

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("SubscriberImpl::create_datareader, ")
                 ACE_TEXT("enable failed.\n")));
      return DDS::DataReader::_nil();
    }
  } else {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, si_lock_, 0);
    readers_not_enabled_.insert(rchandle_from(dr_servant));
  }

  // add created data reader to this' data reader container -
  // done in enable_reader
  return DDS::DataReader::_duplicate(dr_obj.in());
}

DDS::ReturnCode_t
SubscriberImpl::delete_datareader(::DDS::DataReader_ptr a_datareader)
{
  DBG_ENTRY_LVL("SubscriberImpl", "delete_datareader", 6);

  DataReaderImpl_rch dr_servant = rchandle_from(dynamic_cast<DataReaderImpl*>(a_datareader));

  if (dr_servant) { // for MultiTopic this will be false
    const ACE_TCHAR* reason = ACE_TEXT(" (unknown reason)");
    DDS::ReturnCode_t rc = DDS::RETCODE_OK;
    DDS::Subscriber_var dr_subscriber(dr_servant->get_subscriber());
    if (dr_subscriber.in() != this) {
      reason = ACE_TEXT("doesn't belong to this subscriber.");
      rc = DDS::RETCODE_PRECONDITION_NOT_MET;
    } else if (dr_servant->has_zero_copies()) {
      reason = ACE_TEXT("has outstanding zero-copy samples loaned out.");
      rc = DDS::RETCODE_PRECONDITION_NOT_MET;
    } else if (!dr_servant->read_conditions_.empty()) {
      reason = ACE_TEXT("has read conditions attached.");
      rc = DDS::RETCODE_PRECONDITION_NOT_MET;
    }
    if (rc != DDS::RETCODE_OK) {
      if (DCPS_debug_level) {
        GuidConverter converter(dr_servant->get_subscription_id());
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) SubscriberImpl::delete_datareader(%C): ")
          ACE_TEXT("will return \"%C\" because datareader %s\n"),
          OPENDDS_STRING(converter).c_str(), retcode_to_string(rc),
          reason));
      }
      return rc;
    }
  }
  if (dr_servant) {
    // marks entity as deleted and stops future associating
    dr_servant->prepare_to_delete();
  }

  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->si_lock_,
                     DDS::RETCODE_ERROR);

    DataReaderMap::iterator it;

    for (it = datareader_map_.begin();
         it != datareader_map_.end();
         ++it) {
      if (it->second == dr_servant) {
        break;
      }
    }

    if (it == datareader_map_.end()) {
      DDS::TopicDescription_var td = a_datareader->get_topicdescription();
      CORBA::String_var topic_name = td->get_name();
#ifndef OPENDDS_NO_MULTI_TOPIC
      OPENDDS_MAP(OPENDDS_STRING, DDS::DataReader_var)::iterator mt_iter =
        multitopic_reader_map_.find(topic_name.in());
      if (mt_iter != multitopic_reader_map_.end()) {
        DDS::DataReader_ptr ptr = mt_iter->second;
        MultiTopicDataReaderBase* mtdrb = dynamic_cast<MultiTopicDataReaderBase*>(ptr);
        if (!mtdrb) {
          ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: ")
            ACE_TEXT("SubscriberImpl::delete_datareader: ")
            ACE_TEXT("datareader(topic_name=%C)")
            ACE_TEXT("failed to obtain MultiTopicDataReaderBase.\n"),
            topic_name.in()), ::DDS::RETCODE_ERROR);
        }
        mtdrb->cleanup();
        multitopic_reader_map_.erase(mt_iter);
        return DDS::RETCODE_OK;
      }
#endif
      if (!dr_servant) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("SubscriberImpl::delete_datareader: ")
                          ACE_TEXT("datareader(topic_name=%C)")
                          ACE_TEXT("for unknown repo id not found.\n"),
                          topic_name.in()), ::DDS::RETCODE_ERROR);
      }
      RepoId id = dr_servant->get_subscription_id();
      GuidConverter converter(id);
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("SubscriberImpl::delete_datareader: ")
                        ACE_TEXT("datareader(topic_name=%C) %C not found.\n"),
                        topic_name.in(),
                        OPENDDS_STRING(converter).c_str()),
                        ::DDS::RETCODE_ERROR);
    }

    datareader_map_.erase(it);
    datareader_set_.erase(dr_servant);
  }

  if (this->monitor_) {
    this->monitor_->report();
  }

  if (!dr_servant) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("SubscriberImpl::delete_datareader: ")
                      ACE_TEXT("could not remove unknown subscription.\n")),
                      ::DDS::RETCODE_ERROR);
  }

  RepoId subscription_id = dr_servant->get_subscription_id();
  Discovery_rch disco = TheServiceParticipant->get_discovery(this->domain_id_);
  if (!disco->remove_subscription(this->domain_id_,
                                  this->dp_id_,
                                  subscription_id)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("SubscriberImpl::delete_datareader: ")
                      ACE_TEXT(" could not remove subscription from discovery.\n")),
                      ::DDS::RETCODE_ERROR);
  }

  // Call remove association before unregistering the datareader from the transport,
  // otherwise some callbacks resulted from remove_association may be lost.
  dr_servant->remove_all_associations();
  dr_servant->cleanup();
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
SubscriberImpl::delete_contained_entities()
{
  // mark that the entity is being deleted
  set_deleted(true);

  ACE_Vector<DDS::DataReader_ptr> drs;

#ifndef OPENDDS_NO_MULTI_TOPIC
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->si_lock_,
                     DDS::RETCODE_ERROR);
    for (OPENDDS_MAP(OPENDDS_STRING, DDS::DataReader_var)::iterator mt_iter =
           multitopic_reader_map_.begin();
         mt_iter != multitopic_reader_map_.end(); ++mt_iter) {
      drs.push_back(mt_iter->second);
    }
  }

  for (size_t i = 0; i < drs.size(); ++i) {
    DDS::ReturnCode_t ret = drs[i]->delete_contained_entities();
    if (ret == DDS::RETCODE_OK) {
      ret = delete_datareader(drs[i]);
    }
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("SubscriberImpl::delete_contained_entities, ")
                        ACE_TEXT("failed to delete datareader\n")),
                       ret);
    }
  }
  drs.clear();
#endif

  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->si_lock_,
                     DDS::RETCODE_ERROR);
    DataReaderMap::iterator it;
    DataReaderMap::iterator itEnd = datareader_map_.end();

    for (it = datareader_map_.begin(); it != itEnd; ++it) {
      drs.push_back(it->second.in());
    }
  }

  for (size_t i = 0; i < drs.size(); ++i) {
    DDS::ReturnCode_t ret = drs[i]->delete_contained_entities();
    if (ret == DDS::RETCODE_OK) {
      ret = delete_datareader(drs[i]);
    }
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("SubscriberImpl::delete_contained_entities, ")
                        ACE_TEXT("failed to delete datareader\n")),
                       ret);
    }
  }

  // the subscriber can now start creating new publications
  set_deleted(false);

  return DDS::RETCODE_OK;
}

DDS::DataReader_ptr
SubscriberImpl::lookup_datareader(
  const char * topic_name)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->si_lock_,
                   DDS::DataReader::_nil());

  // If multiple entries whose key is "topic_name" then which one is
  // returned ? Spec does not limit which one should give.
  DataReaderMap::iterator it = datareader_map_.find(topic_name);

  if (it == datareader_map_.end()) {
#ifndef OPENDDS_NO_MULTI_TOPIC
    OPENDDS_MAP(OPENDDS_STRING, DDS::DataReader_var)::iterator mt_iter =
      multitopic_reader_map_.find(topic_name);
    if (mt_iter != multitopic_reader_map_.end()) {
      return DDS::DataReader::_duplicate(mt_iter->second);
    }
#endif

    if (DCPS_debug_level >= 2) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) ")
                 ACE_TEXT("SubscriberImpl::lookup_datareader, ")
                 ACE_TEXT("The datareader(topic_name=%C) is not found\n"),
                 topic_name));
    }

    return DDS::DataReader::_nil();

  } else {
    return DDS::DataReader::_duplicate(it->second.in());
  }
}

DDS::ReturnCode_t
SubscriberImpl::get_datareaders(
  DDS::DataReaderSeq &   readers,
  DDS::SampleStateMask   sample_states,
  DDS::ViewStateMask     view_states,
  DDS::InstanceStateMask instance_states)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->si_lock_,
                   DDS::RETCODE_ERROR);

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  // If access_scope is GROUP and ordered_access is true then return readers as
  // list which may contain same readers multiple times. Otherwise return readers
  // as set.
  if (this->qos_.presentation.access_scope == ::DDS::GROUP_PRESENTATION_QOS) {
    if (this->access_depth_ == 0 && this->qos_.presentation.coherent_access) {
      return ::DDS::RETCODE_PRECONDITION_NOT_MET;
    }

    if (this->qos_.presentation.ordered_access) {

      GroupRakeData data;
      for (DataReaderSet::const_iterator pos = datareader_set_.begin();
           pos != datareader_set_.end(); ++pos) {
        (*pos)->get_ordered_data (data, sample_states, view_states, instance_states);
      }

      // Return list of readers in the order of the source timestamp of the received
      // samples from readers.
      data.get_datareaders (readers);

      return DDS::RETCODE_OK;
    }
  }
#endif

  // Return set of datareaders.
  readers.length(0);

  for (DataReaderSet::const_iterator pos = datareader_set_.begin();
       pos != datareader_set_.end(); ++pos) {
    if ((*pos)->have_sample_states(sample_states) &&
        (*pos)->have_view_states(view_states) &&
        (*pos)->have_instance_states(instance_states)) {
      push_back(readers, DDS::DataReader::_duplicate(pos->in()));
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
SubscriberImpl::notify_datareaders()
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->si_lock_,
                   DDS::RETCODE_ERROR);

  DataReaderMap::iterator it;

  for (it = datareader_map_.begin(); it != datareader_map_.end(); ++it) {
    if (it->second->have_sample_states(DDS::NOT_READ_SAMPLE_STATE)) {
      DDS::DataReaderListener_var listener = it->second->get_listener();
      if (!CORBA::is_nil (listener)) {
        listener->on_data_available(it->second.in());
      }

      it->second->set_status_changed_flag(DDS::DATA_AVAILABLE_STATUS, false);
    }
  }

#ifndef OPENDDS_NO_MULTI_TOPIC
  for (OPENDDS_MAP(OPENDDS_STRING, DDS::DataReader_var)::iterator it =
         multitopic_reader_map_.begin(); it != multitopic_reader_map_.end();
       ++it) {
    MultiTopicDataReaderBase* dri =
      dynamic_cast<MultiTopicDataReaderBase*>(it->second.in());

    if (!dri) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: SubscriberImpl::notify_datareaders: ")
        ACE_TEXT("failed to obtain MultiTopicDataReaderBase.\n")),
        ::DDS::RETCODE_ERROR);
    }

    if (dri->have_sample_states(DDS::NOT_READ_SAMPLE_STATE)) {
      DDS::DataReaderListener_var listener = dri->get_listener();
      if (!CORBA::is_nil(listener)) {
        listener->on_data_available(dri);
      }
      dri->set_status_changed_flag(DDS::DATA_AVAILABLE_STATUS, false);
    }
  }
#endif

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
SubscriberImpl::set_qos(
  const DDS::SubscriberQos & qos)
{

  OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);

  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (qos_ == qos)
      return DDS::RETCODE_OK;

    // for the not changeable qos, it can be changed before enable
    if (!Qos_Helper::changeable(qos_, qos) && enabled_ == true) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      qos_ = qos;

      DrIdToQosMap idToQosMap;
      {
        ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                         guard,
                         this->si_lock_,
                         DDS::RETCODE_ERROR);
        // after FaceCTS bug 619 is fixed, make endIter and iter const iteratorsx
        DataReaderMap::iterator endIter = datareader_map_.end();

        for (DataReaderMap::iterator iter = datareader_map_.begin();
             iter != endIter; ++iter) {
          DataReaderImpl_rch reader = iter->second;
          reader->set_subscriber_qos (qos);
          DDS::DataReaderQos qos;
          reader->get_qos(qos);
          RepoId id = reader->get_subscription_id();
          std::pair<DrIdToQosMap::iterator, bool> pair
            = idToQosMap.insert(DrIdToQosMap::value_type(id, qos));

          if (pair.second == false) {
            GuidConverter converter(id);
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: SubscriberImpl::set_qos: ")
                              ACE_TEXT("insert %C to DrIdToQosMap failed.\n"),
                              OPENDDS_STRING(converter).c_str()),::DDS::RETCODE_ERROR);
          }
        }
      }

      DrIdToQosMap::iterator iter = idToQosMap.begin();

      while (iter != idToQosMap.end()) {
        Discovery_rch disco = TheServiceParticipant->get_discovery(this->domain_id_);
        const bool status
          = disco->update_subscription_qos(this->domain_id_,
                                           this->dp_id_,
                                           iter->first,
                                           iter->second,
                                           this->qos_);

        if (!status) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) SubscriberImpl::set_qos, ")
                            ACE_TEXT("failed. \n")),
                           DDS::RETCODE_ERROR);
        }

        ++iter;
      }
    }

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
SubscriberImpl::get_qos(
  DDS::SubscriberQos & qos)
{
  qos = qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
SubscriberImpl::set_listener(
  DDS::SubscriberListener_ptr a_listener,
  DDS::StatusMask             mask)
{
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = DDS::SubscriberListener::_duplicate(a_listener);
  return DDS::RETCODE_OK;
}

DDS::SubscriberListener_ptr
SubscriberImpl::get_listener()
{
  return DDS::SubscriberListener::_duplicate(listener_.in());
}

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE

DDS::ReturnCode_t
SubscriberImpl::begin_access()
{
  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: SubscriberImpl::begin_access:")
                      ACE_TEXT(" Subscriber is not enabled!\n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  if (qos_.presentation.access_scope != DDS::GROUP_PRESENTATION_QOS) {
    return DDS::RETCODE_OK;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->si_lock_,
                   DDS::RETCODE_ERROR);

  ++this->access_depth_;

  // We should only notify subscription on the first
  // and last change to the current change set:
  if (this->access_depth_ == 1) {
    for (DataReaderSet::iterator it = this->datareader_set_.begin();
         it != this->datareader_set_.end(); ++it) {
      (*it)->begin_access();
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
SubscriberImpl::end_access()
{
  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: SubscriberImpl::end_access:")
                      ACE_TEXT(" Publisher is not enabled!\n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  if (qos_.presentation.access_scope != DDS::GROUP_PRESENTATION_QOS) {
    return DDS::RETCODE_OK;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->si_lock_,
                   DDS::RETCODE_ERROR);

  if (this->access_depth_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: SubscriberImpl::end_access:")
                      ACE_TEXT(" No matching call to begin_coherent_changes!\n")),
                     DDS::RETCODE_PRECONDITION_NOT_MET);
  }

  --this->access_depth_;

  // We should only notify subscription on the first
  // and last change to the current change set:
  if (this->access_depth_ == 0) {
    for (DataReaderSet::iterator it = this->datareader_set_.begin();
         it != this->datareader_set_.end(); ++it) {
      (*it)->end_access();
    }
  }

  return DDS::RETCODE_OK;
}

#endif // OPENDDS_NO_OBJECT_MODEL_PROFILE

DDS::DomainParticipant_ptr
SubscriberImpl::get_participant()
{
  return participant_.lock()._retn();
}

DDS::ReturnCode_t
SubscriberImpl::set_default_datareader_qos(
  const DDS::DataReaderQos & qos)
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    default_datareader_qos_ = qos;
    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
SubscriberImpl::get_default_datareader_qos(
  DDS::DataReaderQos & qos)
{
  qos = default_datareader_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
SubscriberImpl::copy_from_topic_qos(
  DDS::DataReaderQos &  a_datareader_qos,
  const DDS::TopicQos & a_topic_qos)
{
  if (Qos_Helper::copy_from_topic_qos(a_datareader_qos, a_topic_qos) ) {
    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
SubscriberImpl::enable()
{
  //According spec:
  // - Calling enable on an already enabled Entity returns OK and has no
  // effect.
  // - Calling enable on an Entity whose factory is not enabled will fail
  // and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

  RcHandle<DomainParticipantImpl> participant = this->participant_.lock();
  if (!participant || participant->is_enabled() == false) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  dp_id_ = participant->get_id();

  if (this->monitor_) {
    this->monitor_->report();
  }

  this->set_enabled();

  if (qos_.entity_factory.autoenable_created_entities) {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, si_lock_, DDS::RETCODE_ERROR);
    DataReaderSet readers;
    readers_not_enabled_.swap(readers);
    for (DataReaderSet::iterator it = readers.begin(); it != readers.end(); ++it) {
      (*it)->enable();
    }
  }

  return DDS::RETCODE_OK;
}

bool
SubscriberImpl::is_clean() const
{
  const bool sub_is_clean = datareader_map_.empty();

  if (!sub_is_clean && !TheTransientKludge->is_enabled()) {
    // Four BIT datareaders.
    return datareader_map_.size() == 4;
  }

  return sub_is_clean;
}

void
SubscriberImpl::data_received(DataReaderImpl* reader)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard,
            this->si_lock_);
  datareader_set_.insert(rchandle_from(reader));
}

DDS::ReturnCode_t
SubscriberImpl::reader_enabled(const char*     topic_name,
                               DataReaderImpl* reader_ptr)
{
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) SubscriberImpl::reader_enabled, ")
               ACE_TEXT("datareader(topic_name=%C) enabled\n"),
               topic_name));
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, si_lock_, DDS::RETCODE_ERROR);
  DataReaderImpl_rch reader = rchandle_from(reader_ptr);
  readers_not_enabled_.erase(reader);

  this->datareader_map_.insert(DataReaderMap::value_type(topic_name, reader));

  if (this->monitor_) {
    this->monitor_->report();
  }

  return DDS::RETCODE_OK;
}

#ifndef OPENDDS_NO_MULTI_TOPIC
DDS::ReturnCode_t
SubscriberImpl::multitopic_reader_enabled(DDS::DataReader_ptr reader)
{
  DDS::TopicDescription_var td = reader->get_topicdescription();
  CORBA::String_var topic = td->get_name();
  multitopic_reader_map_[topic.in()] = DDS::DataReader::_duplicate(reader);
  return DDS::RETCODE_OK;
}

void
SubscriberImpl::remove_from_datareader_set(DataReaderImpl* reader)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, si_lock_);
  datareader_set_.erase(rchandle_from(reader));
}
#endif

DDS::SubscriberListener_ptr
SubscriberImpl::listener_for(::DDS::StatusKind kind)
{
  // per 2.1.4.3.1 Listener Access to Plain Communication Status
  // use this entities factory if listener is mask not enabled
  // for this kind.
  RcHandle<DomainParticipantImpl> participant = this->participant_.lock();
  if (! participant)
    return 0;

  if (CORBA::is_nil(listener_.in()) || (listener_mask_ & kind) == 0) {
    return participant->listener_for(kind);

  } else {
    return DDS::SubscriberListener::_duplicate(listener_.in());
  }
}

unsigned int&
SubscriberImpl::raw_latency_buffer_size()
{
  return this->raw_latency_buffer_size_;
}

DataCollector<double>::OnFull&
SubscriberImpl::raw_latency_buffer_type()
{
  return this->raw_latency_buffer_type_;
}

void
SubscriberImpl::get_subscription_ids(SubscriptionIdVec& subs)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->si_lock_,
                   );

  subs.reserve(datareader_map_.size());
  for (DataReaderMap::iterator iter = datareader_map_.begin();
       iter != datareader_map_.end();
       ++iter) {
    subs.push_back(iter->second->get_subscription_id());
  }
}

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
void
SubscriberImpl::update_ownership_strength (const PublicationId& pub_id,
                                           const CORBA::Long&   ownership_strength)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->si_lock_,
                   );

  for (DataReaderMap::iterator iter = datareader_map_.begin();
       iter != datareader_map_.end();
       ++iter) {
    if (!iter->second->is_bit()) {
      iter->second->update_ownership_strength(pub_id, ownership_strength);
    }
  }
}
#endif


#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
void
SubscriberImpl::coherent_change_received (RepoId&         publisher_id,
                                          DataReaderImpl* reader,
                                          Coherent_State& group_state)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard,
            this->si_lock_);

  // Verify if all readers complete the coherent changes. The result
  // is either COMPLETED or REJECTED.
  group_state = COMPLETED;
  DataReaderSet::const_iterator endIter = datareader_set_.end();
  for (DataReaderSet::const_iterator iter = datareader_set_.begin();
       iter != endIter; ++iter) {

    Coherent_State state = COMPLETED;
    (*iter)->coherent_change_received (publisher_id, state);
    if (state == NOT_COMPLETED_YET) {
      group_state = NOT_COMPLETED_YET;
      return;
    }
    else if (state == REJECTED) {
      group_state = REJECTED;
    }
  }

  PublicationId writerId = GUID_UNKNOWN;
  for (DataReaderSet::const_iterator iter = datareader_set_.begin();
       iter != endIter; ++iter) {
    if (group_state == COMPLETED) {
      (*iter)->accept_coherent (writerId, publisher_id);
    }
    else {   //REJECTED
      (*iter)->reject_coherent (writerId, publisher_id);
    }
  }

  if (group_state == COMPLETED) {
    for (DataReaderSet::const_iterator iter = datareader_set_.begin();
         iter != endIter; ++iter) {
      (*iter)->coherent_changes_completed (reader);
      (*iter)->reset_coherent_info (writerId, publisher_id);
    }
  }
}
#endif

RcHandle<EntityImpl>
SubscriberImpl::parent() const
{
  return this->participant_.lock();
}

bool
SubscriberImpl::validate_datareader_qos(const DDS::DataReaderQos & qos,
                                        const DDS::DataReaderQos & default_qos,
                                        DDS::Topic_ptr             a_topic,
                                        DDS::DataReaderQos &       dr_qos,
                                        bool                       mt)
{


  if (qos == DATAREADER_QOS_DEFAULT) {
    dr_qos = default_qos;

  } else if (qos == DATAREADER_QOS_USE_TOPIC_QOS) {

#ifndef OPENDDS_NO_MULTI_TOPIC
    if (mt) {
      if (DCPS_debug_level) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("SubscriberImpl::create_datareader, ")
                   ACE_TEXT("DATAREADER_QOS_USE_TOPIC_QOS can not be used ")
                   ACE_TEXT("to create a MultiTopic DataReader.\n")));
      }
      return DDS::DataReader::_nil();
    }
#else
    ACE_UNUSED_ARG(mt);
#endif
    DDS::TopicQos topic_qos;
    a_topic->get_qos(topic_qos);

    dr_qos = default_qos;

    Qos_Helper::copy_from_topic_qos(dr_qos,
                                    topic_qos);

  } else {
    dr_qos = qos;
  }

  OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(dr_qos, false);
  OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(dr_qos, false);
  OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT_COMPATIBILITY_CHECK(dr_qos, false);

  if (!Qos_Helper::valid(dr_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("SubscriberImpl::create_datareader, ")
               ACE_TEXT("invalid qos.\n")));
    return false;
  }

  if (!Qos_Helper::consistent(dr_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("SubscriberImpl::create_datareader, ")
               ACE_TEXT("inconsistent qos.\n")));
    return false;
  }


  return true;
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
