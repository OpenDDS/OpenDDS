/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "PublisherImpl.h"
#include "DataWriterImpl.h"
#include "DataWriterRemoteImpl.h"
#include "DomainParticipantImpl.h"
#include "DataWriterImpl.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "RepoIdConverter.h"
#include "Marked_Default_Qos.h"
#include "TopicImpl.h"
#include "MonitorFactory.h"
#include "dds/DdsDcpsTypeSupportExtS.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "AssociationData.h"
#include "dds/DCPS/transport/framework/TransportInterface.h"
#include "dds/DCPS/transport/framework/DataLinkSet.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "tao/debug.h"

namespace OpenDDS {
namespace DCPS {

// Implementation skeleton constructor
PublisherImpl::PublisherImpl(DDS::InstanceHandle_t handle,
                             const DDS::PublisherQos& qos,
                             DDS::PublisherListener_ptr a_listener,
                             const DDS::StatusMask& mask,
                             DomainParticipantImpl* participant)
  : handle_(handle),
    qos_(qos),
    default_datawriter_qos_(TheServiceParticipant->initial_DataWriterQos()),
    listener_mask_(mask),
    listener_(DDS::PublisherListener::_duplicate(a_listener)),
    fast_listener_(0),
    change_depth_(0),
    domain_id_(participant->get_domain_id()),
    participant_(participant),
    suspend_depth_count_(0),
    sequence_number_(),
    aggregation_period_start_(ACE_Time_Value::zero),
    reverse_pi_lock_(pi_lock_),
    monitor_(0)
{
  if (!CORBA::is_nil(a_listener)) {
    fast_listener_ = listener_.in();
  }
  monitor_ = TheServiceParticipant->monitor_factory_->create_publisher_monitor(this);
}

// Implementation skeleton destructor
PublisherImpl::~PublisherImpl()
{
  // Tell the transport to detach this
  // Publisher/TransportInterface.
  this->detach_transport();

  //The datawriters should be deleted already before calling delete
  //publisher.
  if (!is_clean()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("PublisherImpl::~PublisherImpl, ")
               ACE_TEXT("some datawriters still exist.\n")));
  }
}

DDS::InstanceHandle_t
PublisherImpl::get_instance_handle()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return handle_;
}

bool
PublisherImpl::contains_writer(DDS::InstanceHandle_t a_handle)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   DDS::RETCODE_ERROR);

  for (DataWriterMap::iterator it(datawriter_map_.begin());
       it != datawriter_map_.end(); ++it) {
    if (a_handle
        == it->second->local_writer_objref_->get_instance_handle())
      return true;
  }

  return false;
}

DDS::DataWriter_ptr
PublisherImpl::create_datawriter(
  DDS::Topic_ptr a_topic,
  const DDS::DataWriterQos & qos,
  DDS::DataWriterListener_ptr a_listener,
  DDS::StatusMask mask)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (CORBA::is_nil(a_topic)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("PublisherImpl::create_datawriter, ")
               ACE_TEXT("topic is nil.\n")));
    return DDS::DataWriter::_nil();
  }

  DDS::DataWriterQos dw_qos;

  if (qos == DATAWRITER_QOS_DEFAULT) {
    this->get_default_datawriter_qos(dw_qos);

  } else if (qos == DATAWRITER_QOS_USE_TOPIC_QOS) {
    DDS::TopicQos topic_qos;
    a_topic->get_qos(topic_qos);

    this->get_default_datawriter_qos(dw_qos);

    this->copy_from_topic_qos(dw_qos, topic_qos);

  } else {
    dw_qos = qos;
  }

  if (!Qos_Helper::valid(dw_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("PublisherImpl::create_datawriter, ")
               ACE_TEXT("invalid qos.\n")));
    return DDS::DataWriter::_nil();
  }

  if (!Qos_Helper::consistent(dw_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("PublisherImpl::create_datawriter, ")
               ACE_TEXT("inconsistent qos.\n")));
    return DDS::DataWriter::_nil();
  }

  TopicImpl* topic_servant = dynamic_cast<TopicImpl*>(a_topic);

  OpenDDS::DCPS::TypeSupport_ptr typesupport =
    topic_servant->get_type_support();

  if (typesupport == 0) {
    CORBA::String_var name = topic_servant->get_name();
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("PublisherImpl::create_datawriter, ")
               ACE_TEXT("typesupport(topic_name=%C) is nil.\n"),
               name.in()));
    return DDS::DataWriter::_nil();
  }

  DDS::DataWriter_var dw_obj = typesupport->create_datawriter();

  DataWriterImpl* dw_servant =
    dynamic_cast <DataWriterImpl*>(dw_obj.in());

  DataWriterRemoteImpl* writer_remote_impl = 0;
  ACE_NEW_RETURN(writer_remote_impl,
                 DataWriterRemoteImpl(dw_servant),
                 DDS::DataWriter::_nil());

  //this is taking ownership of the DataWriterRemoteImpl (server side)
  //allocated above
  PortableServer::ServantBase_var writer_remote(writer_remote_impl);

  //this is the client reference to the DataWriterRemoteImpl
  OpenDDS::DCPS::DataWriterRemote_var dw_remote_obj =
    servant_to_remote_reference(writer_remote_impl);

  dw_servant->init(a_topic,
                   topic_servant,
                   dw_qos,
                   a_listener,
                   mask,
                   participant_,
                   this,
                   dw_obj.in(),
                   dw_remote_obj.in());

  if (this->enabled_ == true
      && qos_.entity_factory.autoenable_created_entities == 1) {
    DDS::ReturnCode_t ret = dw_servant->enable();

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("PublisherImpl::create_datawriter, ")
                 ACE_TEXT("enable failed.\n")));
      return DDS::DataWriter::_nil();
    }
  }

  return DDS::DataWriter::_duplicate(dw_obj.in());
}

DDS::ReturnCode_t
PublisherImpl::delete_datawriter(DDS::DataWriter_ptr a_datawriter)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DataWriterImpl* dw_servant =
    dynamic_cast <DataWriterImpl*>(a_datawriter);

  {
    DDS::Publisher_var dw_publisher(dw_servant->get_publisher());

    if (dw_publisher.in()!= this) {
      RepoId id = dw_servant->get_publication_id();
      RepoIdConverter converter(id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) PublisherImpl::delete_datareader: ")
                 ACE_TEXT("the data writer %C doesn't ")
                 ACE_TEXT("belong to this subscriber \n"),
                 std::string(converter).c_str()));
      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }
  }

  // Trigger data to be persisted, i.e. made durable, if so
  // configured. This needs be called before unregister_instances
  // because unregister_instances may cause instance dispose.
  if (!dw_servant->persist_data()
      && DCPS_debug_level >= 2) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("PublisherImpl::delete_datawriter, ")
               ACE_TEXT("failed to make data durable.\n")));
  }

  // Unregister all registered instances prior to deletion.
  DDS::Time_t source_timestamp = time_value_to_time(ACE_OS::gettimeofday());
  dw_servant->unregister_instances(source_timestamp);

  CORBA::String_var topic_name = dw_servant->get_topic_name();
  DataWriterImpl* local_writer = 0;
  RepoId publication_id  = GUID_UNKNOWN;
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->pi_lock_,
                     DDS::RETCODE_ERROR);

    publication_id = dw_servant->get_publication_id();
    PublicationMap::iterator it = publication_map_.find(publication_id);

    if (it == publication_map_.end()) {
      RepoIdConverter converter(publication_id);
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("PublisherImpl::delete_datawriter, ")
                        ACE_TEXT("datawriter %C not found.\n"),
                        std::string(converter).c_str()), DDS::RETCODE_ERROR);
    }

    local_writer = it->second->local_writer_impl_;

    PublisherDataWriterInfo* dw_info = it->second;

    // We can not erase the datawriter from datawriter map by the topic name
    // because the map might have multiple datawriters with the same topic
    // name.
    // Find the iterator to the datawriter in the datawriter map and erase
    // by the iterator.
    DataWriterMap::iterator writ;
    DataWriterMap::iterator the_writ = datawriter_map_.end();

    for (writ = datawriter_map_.begin();
         writ != datawriter_map_.end();
         ++writ) {
      if (writ->second == it->second) {
        the_writ = writ;
        break;
      }
    }

    if (the_writ != datawriter_map_.end()) {
      datawriter_map_.erase(the_writ);
    }

    publication_map_.erase(publication_id);

    // Release pi_lock_ before making call to transport layer to avoid
    // some deadlock situations that threads acquire locks(PublisherImpl
    // pi_lock_, TransportInterface reservation_lock and TransportImpl
    // lock_) in reverse order.
    ACE_GUARD_RETURN(reverse_lock_type, reverse_monitor, this->reverse_pi_lock_,
                     DDS::RETCODE_ERROR);

    // Wait for pending samples to drain prior to removing associations
    // and unregistering the publication.
    dw_servant->wait_pending();

    // Call remove association before unregistering the datawriter
    // with the transport, otherwise some callbacks resulted from
    // remove_association may lost.
    dw_servant->remove_all_associations();

    OpenDDS::DCPS::TransportImpl_rch impl = this->get_transport_impl();

    if (impl.is_nil()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("PublisherImpl::delete_datawriter, ")
                 ACE_TEXT("the publisher has not been attached ")
                 ACE_TEXT("to the TransportImpl.\n")));
      return DDS::RETCODE_ERROR;
    }

    // Unregister the DataWriterImpl object with the TransportImpl.
    else if (impl->unregister_publication(publication_id) == -1) {
      RepoIdConverter converter(publication_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("PublisherImpl::delete_datawriter: ")
                 ACE_TEXT("failed to unregister datawriter %C ")
                 ACE_TEXT("with TransportImpl.\n"),
                 std::string(converter).c_str()));
      return DDS::RETCODE_ERROR;
    }

    delete dw_info;

    dw_servant->cleanup();
  }

  // not just unregister but remove any pending writes/sends.
  local_writer->unregister_all();

  try {
    DCPSInfo_var repo = TheServiceParticipant->get_repository(this->domain_id_);
    repo->remove_publication(
      this->domain_id_,
      this->participant_->get_id(),
      publication_id);

  } catch (const CORBA::SystemException& sysex) {
    sysex._tao_print_exception(
      "ERROR: System Exception"
      " in PublisherImpl::delete_datawriter");
    return DDS::RETCODE_ERROR;

  } catch (const CORBA::UserException& userex) {
    userex._tao_print_exception(
      "ERROR: User Exception"
      " in PublisherImpl::delete_datawriter");
    return DDS::RETCODE_ERROR;
  }

  // Decrease ref count after the servant is removed from the
  // map.
  local_writer->_remove_ref();

  return DDS::RETCODE_OK;
}

DDS::DataWriter_ptr
PublisherImpl::lookup_datawriter(const char * topic_name)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   DDS::DataWriter::_nil());

  // If multiple entries whose key is "topic_name" then which one is
  // returned ? Spec does not limit which one should give.
  DataWriterMap::iterator it = datawriter_map_.find(topic_name);

  if (it == datawriter_map_.end()) {
    if (DCPS_debug_level >= 2) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) ")
                 ACE_TEXT("PublisherImpl::lookup_datawriter, ")
                 ACE_TEXT("The datawriter(topic_name=%C) is not found\n"),
                 topic_name));
    }

    return DDS::DataWriter::_nil();

  } else {
    return DDS::DataWriter::_duplicate(it->second->local_writer_objref_);
  }
}

DDS::ReturnCode_t
PublisherImpl::delete_contained_entities()
ACE_THROW_SPEC((CORBA::SystemException))
{
  // mark that the entity is being deleted
  set_deleted(true);

  DataWriterMap::iterator it;
  DataWriterMap::iterator cur;

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   DDS::RETCODE_ERROR);

  for (it = datawriter_map_.begin(); it != datawriter_map_.end();) {
    // Get the iterator for next entry before erasing current entry since
    // the iterator will be invalid after deletion.
    cur = it;
    ++ it;

    PublicationId pub_id = cur->second->publication_id_;
    DDS::ReturnCode_t ret =
      delete_datawriter(cur->second->local_writer_objref_);

    if (ret != DDS::RETCODE_OK) {
      RepoIdConverter converter(pub_id);
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("PublisherImpl::")
                        ACE_TEXT("delete_contained_entities: ")
                        ACE_TEXT("failed to delete ")
                        ACE_TEXT("datawriter %C.\n"),
                        std::string(converter).c_str()),ret);
    }
  }

  // the publisher can now start creating new publications
  set_deleted(false);

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::set_qos(const DDS::PublisherQos & qos)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (qos_ == qos)
      return DDS::RETCODE_OK;

    // for the not changeable qos, it can be changed before enable
    if (!Qos_Helper::changeable(qos_, qos) && enabled_ == true) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      qos_ = qos;

      DwIdToQosMap idToQosMap;
      {
        ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                         guard,
                         this->pi_lock_,
                         DDS::RETCODE_ERROR);

        for (PublicationMap::iterator iter = publication_map_.begin();
             iter != publication_map_.end();
             ++iter) {
          DDS::DataWriterQos qos;
          iter->second->local_writer_impl_->get_qos(qos);
          RepoId id =
            iter->second->local_writer_impl_->get_publication_id();
          std::pair<DwIdToQosMap::iterator, bool> pair =
            idToQosMap.insert(DwIdToQosMap::value_type(id, qos));

          if (pair.second == false) {
            RepoIdConverter converter(id);
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ")
                              ACE_TEXT("PublisherImpl::set_qos: ")
                              ACE_TEXT("insert id %d to DwIdToQosMap ")
                              ACE_TEXT("failed.\n"),
                              std::string(converter).c_str()), DDS::RETCODE_ERROR);
          }
        }
      }

      DwIdToQosMap::iterator iter = idToQosMap.begin();

      while (iter != idToQosMap.end()) {
        try {
          DCPSInfo_var repo = TheServiceParticipant->get_repository(this->domain_id_);
          CORBA::Boolean status
          = repo->update_publication_qos(
              participant_->get_domain_id(),
              participant_->get_id(),
              iter->first,
              iter->second,
              this->qos_);

          if (status == 0) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) PublisherImpl::set_qos, ")
                              ACE_TEXT("failed on compatiblity check. \n")),
                             DDS::RETCODE_ERROR);
          }

        } catch (const CORBA::SystemException& sysex) {
          sysex._tao_print_exception(
            "ERROR: System Exception"
            " in PublisherImpl::set_qos");
          return DDS::RETCODE_ERROR;

        } catch (const CORBA::UserException& userex) {
          userex._tao_print_exception(
            "ERROR:  Exception"
            " in PublisherImpl::set_qos");
          return DDS::RETCODE_ERROR;
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
PublisherImpl::get_qos(DDS::PublisherQos & qos)
ACE_THROW_SPEC((CORBA::SystemException))
{
  qos = qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::set_listener(DDS::PublisherListener_ptr a_listener,
                            DDS::StatusMask mask)
ACE_THROW_SPEC((CORBA::SystemException))
{
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = DDS::PublisherListener::_duplicate(a_listener);
  fast_listener_ = listener_.in();
  return DDS::RETCODE_OK;
}

DDS::PublisherListener_ptr
PublisherImpl::get_listener()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return DDS::PublisherListener::_duplicate(listener_.in());
}

DDS::ReturnCode_t
PublisherImpl::suspend_publications()
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("PublisherImpl::suspend_publications, ")
                      ACE_TEXT(" Entity is not enabled. \n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   DDS::RETCODE_ERROR);
  suspend_depth_count_ ++;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::resume_publications()
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("PublisherImpl::resume_publications, ")
                      ACE_TEXT(" Entity is not enabled. \n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   DDS::RETCODE_ERROR);

  suspend_depth_count_ --;

  if (suspend_depth_count_ < 0) {
    suspend_depth_count_ = 0;
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  if (suspend_depth_count_ == 0) {
    this->send(available_data_list_);
    available_data_list_.reset();
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::begin_coherent_changes()
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: PublisherImpl::begin_coherent_changes:")
                      ACE_TEXT(" Publisher is not enabled!\n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  if (!qos_.presentation.coherent_access) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: PublisherImpl::begin_coherent_changes:")
                      ACE_TEXT(" QoS policy does not support coherent access!\n")),
                     DDS::RETCODE_ERROR);
  }

  if (qos_.presentation.access_scope == DDS::GROUP_PRESENTATION_QOS) {
    // GROUP access scope is not yet supported.
    return DDS::RETCODE_UNSUPPORTED;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   DDS::RETCODE_ERROR);

  ++this->change_depth_;

  if (qos_.presentation.access_scope == DDS::INSTANCE_PRESENTATION_QOS) {
    // INSTANCE access scope essentially behaves
    // as a no-op. (see: 7.1.3.6)
    return DDS::RETCODE_OK;
  }

  // We should only notify publications on the first
  // and last change to the current change set:
  if (this->change_depth_ == 1) {
    for (PublicationMap::iterator it = this->publication_map_.begin();
         it != this->publication_map_.end(); ++it) {
      it->second->local_writer_impl_->
      begin_coherent_changes();
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::end_coherent_changes()
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: PublisherImpl::end_coherent_changes:")
                      ACE_TEXT(" Publisher is not enabled!\n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  if (!qos_.presentation.coherent_access) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: PublisherImpl::end_coherent_changes:")
                      ACE_TEXT(" QoS policy does not support coherent access!\n")),
                     DDS::RETCODE_ERROR);
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   DDS::RETCODE_ERROR);

  if (this->change_depth_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: PublisherImpl::end_coherent_changes:")
                      ACE_TEXT(" No matching call to begin_coherent_changes!\n")),
                     DDS::RETCODE_PRECONDITION_NOT_MET);
  }

  --this->change_depth_;

  if (qos_.presentation.access_scope == DDS::INSTANCE_PRESENTATION_QOS) {
    // INSTANCE access scope essentially behaves
    // as a no-op. (see: 7.1.3.6)
    return DDS::RETCODE_OK;
  }

  // We should only notify publications on the first
  // and last change to the current change set:
  if (this->change_depth_ == 0) {
    for (PublicationMap::iterator it = this->publication_map_.begin();
         it != this->publication_map_.end(); ++it) {
      it->second->local_writer_impl_->
      end_coherent_changes();
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::wait_for_acknowledgments(
  const DDS::Duration_t& max_wait)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: PublisherImpl::wait_for_acknowledgments, ")
                      ACE_TEXT("Entity is not enabled.\n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  DataWriterAckMap ack_writers;
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->pi_lock_,
                     DDS::RETCODE_ERROR);

    // Collect writers to request acks
    for (DataWriterMap::iterator it(this->datawriter_map_.begin());
         it != this->datawriter_map_.end(); ++it) {
      DataWriterImpl* writer = it->second->local_writer_impl_;

      if (writer->should_ack()) {
        DataWriterImpl::AckToken token = writer->create_ack_token(max_wait);

        std::pair<DataWriterAckMap::iterator, bool> pair =
          ack_writers.insert(DataWriterAckMap::value_type(writer, token));

        if (!pair.second) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: PublisherImpl::wait_for_acknowledgments, ")
                            ACE_TEXT("Unable to insert AckToken into DataWriterAckMap!\n")),
                           DDS::RETCODE_ERROR);
        }
      }
    }
  }

  if (ack_writers.empty()) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) PublisherImpl::wait_for_acknowledgments() - ")
                 ACE_TEXT("not blocking due to no writers requiring acks.\n")));
    }

    return DDS::RETCODE_OK;
  }

  // Send ack requests to all associated readers
  for (DataWriterAckMap::iterator it(ack_writers.begin());
       it != ack_writers.end(); ++it) {
    it->first->send_ack_requests(it->second);
  }

  // Wait for ack responses from all associated readers
  for (DataWriterAckMap::iterator it(ack_writers.begin());
       it != ack_writers.end(); ++it) {
    DataWriterImpl::AckToken token = it->second;

    if (token.deadline() <= ACE_OS::gettimeofday()) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: PublisherImpl::wait_for_acknowledgments, ")
                        ACE_TEXT("Timed out waiting for acknowledgments!\n")),
                       DDS::RETCODE_TIMEOUT);
    }

    it->first->wait_for_ack_responses(token);
  }

  return DDS::RETCODE_OK;
}

DDS::DomainParticipant_ptr
PublisherImpl::get_participant()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return DDS::DomainParticipant::_duplicate(participant_);
}

DDS::ReturnCode_t
PublisherImpl::set_default_datawriter_qos(const DDS::DataWriterQos & qos)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    default_datawriter_qos_ = qos;
    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
PublisherImpl::get_default_datawriter_qos(DDS::DataWriterQos & qos)
ACE_THROW_SPEC((CORBA::SystemException))
{
  qos = default_datawriter_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::copy_from_topic_qos(DDS::DataWriterQos & a_datawriter_qos,
                                   const DDS::TopicQos & a_topic_qos)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (Qos_Helper::valid(a_topic_qos)
      && Qos_Helper::consistent(a_topic_qos)) {
    // Some members in the DataWriterQos are not contained
    // in the TopicQos. The caller needs initialize them.
    a_datawriter_qos.durability = a_topic_qos.durability;
    a_datawriter_qos.durability_service = a_topic_qos.durability_service;
    a_datawriter_qos.deadline = a_topic_qos.deadline;
    a_datawriter_qos.latency_budget = a_topic_qos.latency_budget;
    a_datawriter_qos.liveliness = a_topic_qos.liveliness;
    a_datawriter_qos.reliability = a_topic_qos.reliability;
    a_datawriter_qos.destination_order = a_topic_qos.destination_order;
    a_datawriter_qos.history = a_topic_qos.history;
    a_datawriter_qos.resource_limits = a_topic_qos.resource_limits;
    a_datawriter_qos.transport_priority = a_topic_qos.transport_priority;
    a_datawriter_qos.lifespan = a_topic_qos.lifespan;

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
PublisherImpl::enable()
ACE_THROW_SPEC((CORBA::SystemException))
{
  //According spec:
  // - Calling enable on an already enabled Entity returns OK and has no
  // effect.
  // - Calling enable on an Entity whose factory is not enabled will fail
  // and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

  if (this->participant_->is_enabled() == false) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  if (this->monitor_) {
    this->monitor_->report();
  }

  this->set_enabled();
  return DDS::RETCODE_OK;
}

int
PublisherImpl::is_clean() const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   -1);
  return datawriter_map_.empty() && publication_map_.empty();
}

void PublisherImpl::add_associations(const ReaderAssociationSeq & readers,
                                     DataWriterImpl* writer,
                                     const DDS::DataWriterQos writer_qos)
{
  if (entity_deleted_ == true) {
    if (DCPS_debug_level >= 1)
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) PublisherImpl::add_associations ")
                 ACE_TEXT("This is a deleted publisher, ")
                 ACE_TEXT("ignoring add.\n")));

    return;
  }

  AssociationInfo info;
  info.num_associations_ = readers.length();

  // TransportInterface does not take ownership of the associations.
  // The associations will be deleted when transport inform
  // datawriter fully associated (in DataWriterImpl::fully_associated()).
  info.association_data_ = new AssociationData[info.num_associations_];

  for (ssize_t i = 0; i < info.num_associations_; ++i) {
    info.association_data_[i].remote_id_ = readers[i].readerId;
    info.association_data_[i].remote_data_ = readers[i].readerTransInfo;
  }

  if (DCPS_debug_level > 4) {
    RepoIdConverter converter(writer->get_publication_id());
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) PublisherImpl::add_associations(): ")
               ACE_TEXT("adding %d subscriptions to publication %C with priority %d.\n"),
               info.num_associations_,
               std::string(converter).c_str(),
               writer_qos.transport_priority.value));
  }

  this->add_subscriptions(writer->get_publication_id(),
                          info,
                          writer_qos.transport_priority.value,
                          writer);
}

void
PublisherImpl::remove_associations(const ReaderIdSeq & readers,
                                   const RepoId&       writer)
{
  // Delegate to the (inherited) TransportInterface version.

  // TMB - I don't know why I have to do it this way, but the compiler
  //       on linux complains with an error otherwise.
  this->TransportInterface::remove_associations(readers.length(),
                                                readers.get_buffer(),
                                                writer,
                                                true); // as pub side
}

DDS::ReturnCode_t
PublisherImpl::writer_enabled(
  OpenDDS::DCPS::DataWriterRemote_ptr remote_writer,
  DDS::DataWriter_ptr    local_writer,
  const char*              topic_name,
  //BuiltinTopicKey_t topic_key
  RepoId                   topic_id)
{
  PublisherDataWriterInfo* info = new PublisherDataWriterInfo;
  info->remote_writer_objref_ = remote_writer;
  info->local_writer_objref_ = local_writer;
  info->local_writer_impl_ =
    dynamic_cast<DataWriterImpl*>(local_writer);

  info->topic_id_      = topic_id;
  // all other info memebers default in constructor

  /// Load the publication into the repository and get the
  /// publication_id_ in return.
  try {
    DDS::DataWriterQos qos;
    info->local_writer_objref_->get_qos(qos);

    OpenDDS::DCPS::TransportInterfaceInfo trans_conf_info =
      connection_info();
    trans_conf_info.publication_transport_priority = qos.transport_priority.value;

    DCPSInfo_var repo = TheServiceParticipant->get_repository(this->domain_id_);
    info->publication_id_ =
      repo->add_publication(
        this->domain_id_, // Loaded during Publisher
        // construction
        this->participant_->get_id(),   // Loaded during Publisher construction.
        info->topic_id_, // Loaded during DataWriter construction.
        info->remote_writer_objref_,
        qos,
        trans_conf_info ,   // Obtained during setup.
        this->qos_);

    if (info->publication_id_ == GUID_UNKNOWN) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: PublisherImpl::writer_enabled, ")
                 ACE_TEXT("add_publication returned invalid id. \n")));
      return DDS::RETCODE_ERROR;
    }

    info->local_writer_impl_->set_publication_id(info->publication_id_);

    OpenDDS::DCPS::TransportImpl_rch impl = this->get_transport_impl();

    if (impl.is_nil()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("PublisherImpl::writer_enabled, ")
                 ACE_TEXT("the publisher has not been attached to ")
                 ACE_TEXT("the TransportImpl.\n")));
      return DDS::RETCODE_ERROR;
    }

    // Register the DataWriterImpl object with the TransportImpl.
    else if (impl->register_publication(info->publication_id_,
                                        info->local_writer_impl_) == -1) {
      RepoIdConverter converter(info->publication_id_);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("PublisherImpl::writer_enabled: ")
                 ACE_TEXT("failed to register datawriter %C with ")
                 ACE_TEXT("TransportImpl.\n"),
                 std::string(converter).c_str()));
      return DDS::RETCODE_ERROR;
    }

  } catch (const CORBA::SystemException& sysex) {
    sysex._tao_print_exception(
      "ERROR: System Exception"
      " in PublisherImpl::writer_enabled");
    return DDS::RETCODE_ERROR;

  } catch (const CORBA::UserException& userex) {
    userex._tao_print_exception(
      "ERROR:  Exception"
      " in PublisherImpl::writer_enabled");
    return DDS::RETCODE_ERROR;
  }

  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->pi_lock_,
                     DDS::RETCODE_ERROR);
    DataWriterMap::iterator it =
      datawriter_map_.insert(DataWriterMap::value_type(topic_name, info));

    if (it == datawriter_map_.end()) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("PublisherImpl::writer_enabled, ")
                        ACE_TEXT("insert datawriter(topic_name=%C) ")
                        ACE_TEXT("failed.\n"),
                        topic_name),
                       DDS::RETCODE_ERROR);
    }

    std::pair<PublicationMap::iterator, bool> pair =
      publication_map_.insert(
        PublicationMap::value_type(info->publication_id_, info));

    if (pair.second == false) {
      RepoIdConverter converter(info->publication_id_);
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("PublisherImpl::writer_enabled: ")
                        ACE_TEXT("insert publication %C failed.\n"),
                        std::string(converter).c_str()), DDS::RETCODE_ERROR);
    }

    // Increase ref count when the servant is added to the
    // datawriter/publication map.
    info->local_writer_impl_->_add_ref();
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::data_available(DataWriterImpl* writer,
                              bool resend)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   DDS::RETCODE_ERROR);

  DataSampleList list;

  if (resend) {
    list = writer->get_resend_data();

  } else {
    list = writer->get_unsent_data();
  }

  if (this->suspend_depth_count_ > 0) {
    // append list to the avaliable data list.
    // Collect samples from all of the Publisher's Datawriters
    // in this list so when resume_publication is called
    // the Publisher does not have to iterate over its
    // DataWriters to get the unsent data samples.
    available_data_list_.enqueue_tail_next_send_sample(list);

  } else {
    // Do LATENCY_BUDGET processing here.
    // Do coherency processing here.
    // tell the transport to send the data sample(s).
    this->send(list);
  }

  return DDS::RETCODE_OK;
}

DDS::PublisherListener*
PublisherImpl::listener_for(DDS::StatusKind kind)
{
  // per 2.1.4.3.1 Listener Access to Plain Communication Status
  // use this entities factory if listener is mask not enabled
  // for this kind.
  if (fast_listener_ == 0 || (listener_mask_ & kind) == 0) {
    return participant_->listener_for(kind);

  } else {
    return fast_listener_;
  }
}

DDS::ReturnCode_t
PublisherImpl::assert_liveliness_by_participant()
{
  DDS::ReturnCode_t ret = DDS::RETCODE_OK;

  for (DataWriterMap::iterator it(datawriter_map_.begin());
       it != datawriter_map_.end(); ++it) {
    DDS::ReturnCode_t dw_ret
    = it->second->local_writer_impl_->assert_liveliness_by_participant();

    if (dw_ret != DDS::RETCODE_OK) {
      ret = dw_ret;
    }
  }

  return ret;
}

void
PublisherImpl::get_publication_ids(PublicationIdVec& pubs)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->pi_lock_,
                   );

  pubs.reserve(publication_map_.size());
  for (PublicationMap::iterator iter = publication_map_.begin();
       iter != publication_map_.end();
       ++iter) {
    pubs.push_back(iter->first);
  }
}

DDS::DomainId_t
PublisherImpl::get_domain_id() const
{
  return this->domain_id_;
}

RepoId
PublisherImpl::get_participant_id() const
{
  return this->participant_->get_id();
}

} // namespace DCPS
} // namespace OpenDDS

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

template class std::multimap<ACE_CString, PublisherDataWriterInfo*>;
template class std::map<PublicationId, PublisherDataWriterInfo*>;

#elif defined(ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate std::multimap<ACE_CString, PublisherDataWriterInfo*>
#pragma instantiate std::map<PublicationId, PublisherDataWriterInfo*>

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
