// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "PublisherImpl.h"
#include "DataWriterImpl.h"
#include "DataWriterRemoteImpl.h"
#include "DomainParticipantImpl.h"
#include "DataWriterImpl.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "Marked_Default_Qos.h"
#include "TopicImpl.h"
#include "dds/DdsDcpsTypeSupportExtS.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "AssociationData.h"
#include "dds/DCPS/transport/framework/TransportInterface.h"
#include "dds/DCPS/transport/framework/DataLinkSet.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "tao/debug.h"

namespace OpenDDS
{
  namespace DCPS
  {

#if 0
    // Emacs trick to align code with first column
    // This will cause emacs to emit bogus alignment message
    // For now just disregard them.
  }}
#endif

const CoherencyGroup DEFAULT_GROUP_ID = 0;

//TBD - add check for enabled in most methods.
//      currently this is not needed because auto_enable_created_entities
//      cannot be false.

// Implementation skeleton constructor
PublisherImpl::PublisherImpl (const ::DDS::PublisherQos &   qos,
                              ::DDS::PublisherListener_ptr a_listener,
                              DomainParticipantImpl*       participant)
  : qos_(qos),
    default_datawriter_qos_(TheServiceParticipant->initial_DataWriterQos ()),
    listener_mask_(DEFAULT_STATUS_KIND_MASK),
    listener_ (::DDS::PublisherListener::_duplicate(a_listener)),
    fast_listener_ (0),
    group_id_ (DEFAULT_GROUP_ID),
    repository_ (
      TheServiceParticipant->get_repository (participant->get_domain_id())),
    participant_ (participant),
    suspend_depth_count_ (0),
    sequence_number_ (),
    aggregation_period_start_ (ACE_Time_Value::zero)
{
  if (! CORBA::is_nil (a_listener))
    {
      fast_listener_ = listener_.in ();
    }
}

// Implementation skeleton destructor
PublisherImpl::~PublisherImpl (void)
{
  // Tell the transport to detach this
  // Publisher/TransportInterface.
  this->detach_transport ();

  //The datawriters should be deleted already before calling delete
  //publisher.
  if (! is_clean ())
    {
      ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("PublisherImpl::~PublisherImpl, ")
      ACE_TEXT("some datawriters still exist.\n")));
    }
}

::DDS::DataWriter_ptr
PublisherImpl::create_datawriter (
    ::DDS::Topic_ptr a_topic,
    const ::DDS::DataWriterQos & qos,
    ::DDS::DataWriterListener_ptr a_listener)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (CORBA::is_nil (a_topic))
    {
      ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("PublisherImpl::create_datawriter, ")
      ACE_TEXT("topic is nil.\n")));
      return ::DDS::DataWriter::_nil();
    }

  ::DDS::DataWriterQos dw_qos;
  if (qos == DATAWRITER_QOS_DEFAULT)
    {
      this->get_default_datawriter_qos(dw_qos);
    }
  else if (qos == DATAWRITER_QOS_USE_TOPIC_QOS)
    {
      ::DDS::TopicQos topic_qos;
      a_topic->get_qos (topic_qos);

      this->get_default_datawriter_qos(dw_qos);

      this->copy_from_topic_qos (dw_qos, topic_qos);
    }
  else
    {
      dw_qos = qos;
    }

  if (! Qos_Helper::valid (dw_qos))
    {
      ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("PublisherImpl::create_datawriter, ")
      ACE_TEXT("invalid qos.\n")));
      return ::DDS::DataWriter::_nil();
    }

  if (! Qos_Helper::consistent (dw_qos))
    {
      ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("PublisherImpl::create_datawriter, ")
      ACE_TEXT("inconsistent qos.\n")));
      return ::DDS::DataWriter::_nil();
    }

  TopicImpl* topic_servant = dynamic_cast<TopicImpl*> (a_topic);

  OpenDDS::DCPS::TypeSupport_ptr typesupport =
    topic_servant->get_type_support();

  if (typesupport == 0)
    {
      CORBA::String_var name = topic_servant->get_name ();
      ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("PublisherImpl::create_datawriter, ")
      ACE_TEXT("typesupport(topic_name=%s) is nil.\n"),
      name.in ()));
      return ::DDS::DataWriter::_nil ();
    }

  ::DDS::DataWriter_var dw_obj = typesupport->create_datawriter ();

  DataWriterImpl* dw_servant =
    dynamic_cast <DataWriterImpl*> (dw_obj.in ());

  DataWriterRemoteImpl* writer_remote_impl = 0;
  ACE_NEW_RETURN(writer_remote_impl,
                 DataWriterRemoteImpl(dw_servant),
                 ::DDS::DataWriter::_nil());

  //this is taking ownership of the DataWriterRemoteImpl (server side)
  //allocated above
  PortableServer::ServantBase_var writer_remote(writer_remote_impl);

  //this is the client reference to the DataWriterRemoteImpl
  ::OpenDDS::DCPS::DataWriterRemote_var dw_remote_obj =
      servant_to_remote_reference(writer_remote_impl);

  dw_servant->init (a_topic,
                    topic_servant,
                    dw_qos,
                    a_listener,
                    participant_,
                    this,
                    dw_obj.in (),
                    dw_remote_obj.in ());

  if (this->enabled_ == true
      && qos_.entity_factory.autoenable_created_entities == 1)
    {
      ::DDS::ReturnCode_t ret = dw_servant->enable ();

      if (ret != ::DDS::RETCODE_OK)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("PublisherImpl::create_datawriter, ")
                      ACE_TEXT("enable failed.\n")));
          return ::DDS::DataWriter::_nil ();
        }
    }

  OpenDDS::DCPS::TransportImpl_rch impl = this->get_transport_impl();
  if (impl.is_nil ())
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("PublisherImpl::create_datawriter, ")
                  ACE_TEXT("the publisher has not been attached to ")
                  ACE_TEXT("the TransportImpl.\n")));
      return ::DDS::DataWriter::_nil ();
    }
  // Register the DataWriterImpl object with the TransportImpl.
  else if (impl->register_publication (dw_servant->get_publication_id(),
               dw_servant) == -1)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("PublisherImpl::create_datawriter, ")
                  ACE_TEXT("failed to register datawriter %d with ")
                  ACE_TEXT("TransportImpl.\n"),
                  dw_servant->get_publication_id()));
      return ::DDS::DataWriter::_nil ();
    }
  return ::DDS::DataWriter::_duplicate (dw_obj.in ());
}

::DDS::ReturnCode_t
PublisherImpl::delete_datawriter (::DDS::DataWriter_ptr a_datawriter)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: PublisherImpl::delete_datawriter, ")
       ACE_TEXT(" Entity is not enabled. \n")),
      ::DDS::RETCODE_NOT_ENABLED);
    }

  DataWriterImpl* dw_servant =
    dynamic_cast <DataWriterImpl*> (a_datawriter);

  {
    ::DDS::Publisher_var dw_publisher(dw_servant->get_publisher());
    if (dw_publisher.in()!= this)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) PublisherImpl::delete_datareader ")
                  ACE_TEXT("the data writer (pubId=%d) doesn't ")
                  ACE_TEXT("belong to this subscriber \n"),
                  dw_servant->get_publication_id()));
      return ::DDS::RETCODE_PRECONDITION_NOT_MET;
    }
  }

  CORBA::String_var topic_name = dw_servant->get_topic_name ();
  DataWriterImpl* local_writer = 0;
  RepoId publication_id  = 0;
  {
    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
          guard,
          this->pi_lock_,
          ::DDS::RETCODE_ERROR);

    publication_id = dw_servant->get_publication_id ();
    PublicationMap::iterator it = publication_map_.find (publication_id);

    if (it == publication_map_.end ())
      {
  ACE_ERROR_RETURN ((LM_ERROR,
         ACE_TEXT("(%P|%t) ERROR: ")
         ACE_TEXT("PublisherImpl::delete_datawriter, ")
         ACE_TEXT("The datawriter(repoid=%d) is not found\n"),
         publication_id),
        ::DDS::RETCODE_ERROR);
      }
    local_writer = it->second->local_writer_impl_;

    PublisherDataWriterInfo* dw_info = it->second;

    // We can not erase the datawriter from datawriter map by the topic name
    // because the map might have multiple datawriters with the same topic
    // name.
    // Find the iterator to the datawriter in the datawriter map and erase
    // by the iterator.
    DataWriterMap::iterator writ;
    DataWriterMap::iterator the_writ = datawriter_map_.end ();

    for (writ = datawriter_map_.begin ();
         writ != datawriter_map_.end ();
         ++writ)
    {
      if (writ->second == it->second)
      {
        the_writ = writ;
        break;
      }
    }

    if (the_writ != datawriter_map_.end ())
    {
      datawriter_map_.erase (the_writ);
    }

    publication_map_.erase (publication_id);

    // Call remove association before unregistering the datawriter
    // with the transport, otherwise some callbacks resulted from
    // remove_association may lost.

    dw_servant->remove_all_associations();


    OpenDDS::DCPS::TransportImpl_rch impl = this->get_transport_impl();
    if (impl.is_nil ())
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("PublisherImpl::delete_datawriter, ")
                  ACE_TEXT("the publisher has not been attached ")
                  ACE_TEXT("to the TransportImpl.\n")));
      return ::DDS::RETCODE_ERROR;
    }
    // Unregister the DataWriterImpl object with the TransportImpl.
    else if (impl->unregister_publication (publication_id) == -1)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("PublisherImpl::delete_datawriter, ")
                  ACE_TEXT("failed to unregister datawriter %d ")
                  ACE_TEXT("with TransportImpl.\n"),
                  publication_id));
      return ::DDS::RETCODE_ERROR;
    }

    delete dw_info;

    dw_servant->cleanup ();
  }

  // Trigger data to be persisted, i.e. made durable, if so
  // configured.
  if (!local_writer->persist_data()
      && DCPS_debug_level >= 2)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: ")
                ACE_TEXT("PublisherImpl::delete_datawriter, ")
                ACE_TEXT("failed to make data durable.\n")));
  }

  // not just unregister but remove any pending writes/sends.
  local_writer->unregister_all ();

  try
    {
      this->repository_->remove_publication(
              participant_->get_domain_id (),
              participant_->get_id (),
              publication_id);
    }
  catch (const CORBA::SystemException& sysex)
    {
      sysex._tao_print_exception (
        "ERROR: System Exception"
        " in PublisherImpl::delete_datawriter");
      return ::DDS::RETCODE_ERROR;
    }
  catch (const CORBA::UserException& userex)
    {
      userex._tao_print_exception (
        "ERROR: User Exception"
        " in PublisherImpl::delete_datawriter");
      return ::DDS::RETCODE_ERROR;
    }

  // Decrease ref count after the servant is removed from the
  // map.
  local_writer->_remove_ref ();

  return ::DDS::RETCODE_OK;
}

::DDS::DataWriter_ptr
PublisherImpl::lookup_datawriter (const char * topic_name)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (enabled_ == false)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: PublisherImpl::lookup_datawriter, ")
                  ACE_TEXT(" Entity is not enabled. \n")));
      return ::DDS::DataWriter::_nil ();
    }

  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    this->pi_lock_,
                    ::DDS::DataWriter::_nil ());

  // If multiple entries whose key is "topic_name" then which one is
  // returned ? Spec does not limit which one should give.
  DataWriterMap::iterator it = datawriter_map_.find (topic_name);
  if (it == datawriter_map_.end ())
    {
      if (DCPS_debug_level >= 2)
        {
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT("(%P|%t) ")
                      ACE_TEXT("PublisherImpl::lookup_datawriter, ")
                      ACE_TEXT("The datawriter(topic_name=%s) is not found\n"),
                      topic_name));
        }
      return ::DDS::DataWriter::_nil ();
    }
  else
    {
      return ::DDS::DataWriter::_duplicate (it->second->local_writer_objref_);
    }
}

::DDS::ReturnCode_t
PublisherImpl::delete_contained_entities ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: PublisherImpl::delete_contained_entities, ")
       ACE_TEXT(" Entity is not enabled. \n")),
      ::DDS::RETCODE_NOT_ENABLED);
    }

  // mark that the entity is being deleted
  set_deleted (true);

  DataWriterMap::iterator it;
  DataWriterMap::iterator next;

  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
        guard,
        this->pi_lock_,
        ::DDS::RETCODE_ERROR);

  for (it = datawriter_map_.begin (); it != datawriter_map_.end (); )
    {
      // Get the iterator for next entry before erasing current entry since
      // the iterator will be invalid after deletion.
      next = it;
      next ++;
      ::DDS::ReturnCode_t ret =
          delete_datawriter (it->second->local_writer_objref_);
      if (ret != ::DDS::RETCODE_OK)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT("(%P|%t) ERROR: ")
                             ACE_TEXT("PublisherImpl::")
                             ACE_TEXT("delete_contained_entities, ")
                             ACE_TEXT("failed to delete ")
                             ACE_TEXT("datawriter(publication_id=%d)\n"),
                             it->second->publication_id_), ret);
        }
      it = next;
    }

  // the publisher can now start creating new publications
  set_deleted (false);

  return ::DDS::RETCODE_OK;
}

::DDS::ReturnCode_t
PublisherImpl::set_qos (const ::DDS::PublisherQos & qos)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos))
    {
      if (qos_ == qos)
        return ::DDS::RETCODE_OK;

      if (enabled_ == true)
      {
        if (! Qos_Helper::changeable (qos_, qos))
        {
          return ::DDS::RETCODE_IMMUTABLE_POLICY;
        }
        else
        {
          qos_ = qos;

          DwIdToQosMap idToQosMap;
          {
            ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
              guard,
              this->pi_lock_,
              ::DDS::RETCODE_ERROR);

            for (PublicationMap::iterator iter = publication_map_.begin();
              iter != publication_map_.end();
              ++iter)
            {
              ::DDS::DataWriterQos qos;
              iter->second->local_writer_impl_->get_qos(qos);
              RepoId id =
                iter->second->local_writer_impl_->get_publication_id();
              std::pair<DwIdToQosMap::iterator, bool> pair =
                idToQosMap.insert(DwIdToQosMap::value_type(id, qos));
              if (pair.second == false)
              {
                ACE_ERROR_RETURN ((LM_ERROR,
                                   ACE_TEXT("(%P|%t) ")
                                   ACE_TEXT("PublisherImpl::set_qos, ")
                                   ACE_TEXT("insert id(%d) to DwIdToQosMap ")
                                   ACE_TEXT("failed.\n"),
                                   id),
                                  ::DDS::RETCODE_ERROR);
              }
            }
          }

          DwIdToQosMap::iterator iter = idToQosMap.begin ();
          while (iter != idToQosMap.end())
          {
            try
            {
              CORBA::Boolean status
                = this->repository_->update_publication_qos (
                      participant_->get_domain_id(),
                      participant_->get_id (),
                      iter->first,
                      iter->second,
                      this->qos_);

              if (status == 0)
              {
                ACE_ERROR_RETURN ((LM_ERROR,
                  ACE_TEXT("(%P|%t) "
                  "PublisherImpl::set_qos, ")
                  ACE_TEXT("failed on compatiblity check. \n")),
                  ::DDS::RETCODE_ERROR);
              }
            }
            catch (const CORBA::SystemException& sysex)
            {
              sysex._tao_print_exception (
                "ERROR: System Exception"
                " in PublisherImpl::set_qos");
              return ::DDS::RETCODE_ERROR;
            }
            catch (const CORBA::UserException& userex)
            {
              userex._tao_print_exception (
                "ERROR:  Exception"
                " in PublisherImpl::set_qos");
              return ::DDS::RETCODE_ERROR;
            }
            ++iter;
          }
        }
      }
      else
        qos_ = qos;

      return ::DDS::RETCODE_OK;
    }
  else
    {
      return ::DDS::RETCODE_INCONSISTENT_POLICY;
    }
}

void
PublisherImpl::get_qos (::DDS::PublisherQos & qos)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  qos = qos_;
}

::DDS::ReturnCode_t
PublisherImpl::set_listener (::DDS::PublisherListener_ptr a_listener,
                             ::DDS::StatusKindMask mask)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  listener_mask_ = mask;
  //note: OK to duplicate  and reference_to_servant a nil object ref
  listener_ = ::DDS::PublisherListener::_duplicate(a_listener);
  fast_listener_ = listener_.in ();
  return ::DDS::RETCODE_OK;
}

::DDS::PublisherListener_ptr
PublisherImpl::get_listener ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  return ::DDS::PublisherListener::_duplicate (listener_.in ());
}

::DDS::ReturnCode_t
PublisherImpl::suspend_publications ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("PublisherImpl::suspend_publications, ")
                         ACE_TEXT(" Entity is not enabled. \n")),
                        ::DDS::RETCODE_NOT_ENABLED);
    }

  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    this->pi_lock_,
                    ::DDS::RETCODE_ERROR);
  suspend_depth_count_ ++;
  return ::DDS::RETCODE_OK;
}

::DDS::ReturnCode_t
PublisherImpl::resume_publications ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("PublisherImpl::resume_publications, ")
                         ACE_TEXT(" Entity is not enabled. \n")),
                        ::DDS::RETCODE_NOT_ENABLED);
    }

  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    this->pi_lock_,
                    ::DDS::RETCODE_ERROR);

  suspend_depth_count_ --;
  if (suspend_depth_count_ < 0)
    {
      suspend_depth_count_ = 0;
      return ::DDS::RETCODE_PRECONDITION_NOT_MET;
    }

  if (suspend_depth_count_ == 0)
    {
      this->send (available_data_list_);
      available_data_list_.reset ();
    }

  return ::DDS::RETCODE_OK;
}

::DDS::ReturnCode_t
 PublisherImpl::begin_coherent_changes ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  //NOT REQUIRED FOR FIRST IMPLEMENTATION
  return ::DDS::RETCODE_UNSUPPORTED;
}

::DDS::ReturnCode_t
PublisherImpl::end_coherent_changes ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  //NOT REQUIRED FOR FIRST IMPLEMENTATION
  return ::DDS::RETCODE_UNSUPPORTED;
}

::DDS::DomainParticipant_ptr
PublisherImpl::get_participant ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  return ::DDS::DomainParticipant::_duplicate (participant_);
}

::DDS::ReturnCode_t
PublisherImpl::set_default_datawriter_qos (const ::DDS::DataWriterQos & qos)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos))
    {
      default_datawriter_qos_ = qos;
      return ::DDS::RETCODE_OK;
    }
  else
    {
      return ::DDS::RETCODE_INCONSISTENT_POLICY;
    }
}

void
PublisherImpl::get_default_datawriter_qos (::DDS::DataWriterQos & qos)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  qos = default_datawriter_qos_;
}

::DDS::ReturnCode_t
PublisherImpl::copy_from_topic_qos (::DDS::DataWriterQos & a_datawriter_qos,
                                    const ::DDS::TopicQos & a_topic_qos)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (Qos_Helper::valid(a_topic_qos)
      && Qos_Helper::consistent(a_topic_qos))
    {
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

      return ::DDS::RETCODE_OK;
    }
  else
    {
      return ::DDS::RETCODE_INCONSISTENT_POLICY;
    }
}

::DDS::ReturnCode_t
PublisherImpl::enable ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  //TDB - check if factory is enables and then enable all entities
  // (don't need to do it for now because
  //  entity_factory.autoenable_created_entities is always = 1)

  //if (factory not enabled)
  //{
  //  return ::DDS::RETCODE_PRECONDITION_NOT_MET;
  //}

  this->set_enabled ();
  return ::DDS::RETCODE_OK;
}

::DDS::StatusKindMask
PublisherImpl::get_status_changes ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  return EntityImpl::get_status_changes ();
}


int
PublisherImpl::is_clean () const
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    this->pi_lock_,
                    -1);
  return datawriter_map_.empty () && publication_map_.empty ();
}

void PublisherImpl::add_associations (const ReaderAssociationSeq & readers,
                                      DataWriterImpl* writer,
                                      const ::DDS::DataWriterQos writer_qos)
{
  if (entity_deleted_ == true)
    {
      if (DCPS_debug_level >= 1)
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT("(%P|%t) PublisherImpl::add_associations ")
                    ACE_TEXT("This is a deleted publisher, ")
                    ACE_TEXT("ignoring add.\n")));
      return;
    }

  size_t length = readers.length ();
  AssociationData* associations = new AssociationData[length];
  for (size_t i = 0; i < length; ++i)
    {
      associations[i].remote_id_ = readers[i].readerId;
      associations[i].remote_data_ = readers[i].readerTransInfo;
    }

  this->add_subscriptions (writer->get_publication_id (),
                           writer_qos.transport_priority.value,
                           length,
                           associations);
  // TransportInterface does not take ownership of the associations.
  // The associations will be deleted when transport inform
  // datawriter fully associated (in DataWriterImpl::fully_associated()).
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

::DDS::ReturnCode_t
PublisherImpl::writer_enabled(
  ::OpenDDS::DCPS::DataWriterRemote_ptr remote_writer,
  ::DDS::DataWriter_ptr    local_writer,
  const char*              topic_name,
  //BuiltinTopicKey_t topic_key
  RepoId                   topic_id)
{
  PublisherDataWriterInfo* info = new PublisherDataWriterInfo;
  info->remote_writer_objref_ = remote_writer;
  info->local_writer_objref_ = local_writer;
  info->local_writer_impl_ =
    dynamic_cast<DataWriterImpl*> (local_writer);

  info->topic_id_      = topic_id;
  // all other info memebers default in constructor

  /// Load the publication into the repository and get the
  /// publication_id_ in return.
  try
    {
      ::DDS::DataWriterQos qos;
      info->local_writer_objref_->get_qos(qos);

      OpenDDS::DCPS::TransportInterfaceInfo trans_conf_info =
        connection_info ();

      info->publication_id_ =
        this->repository_->add_publication(
          participant_->get_domain_id (), // Loaded during Publisher
                                          // construction
          participant_->get_id (),  // Loaded during Publisher construction.
          info->topic_id_, // Loaded during DataWriter construction.
          info->remote_writer_objref_,
          qos,
          trans_conf_info ,   // Obtained during setup.
          qos_);
      info->local_writer_impl_->set_publication_id (info->publication_id_);
    }
  catch (const CORBA::SystemException& sysex)
    {
      sysex._tao_print_exception (
        "ERROR: System Exception"
        " in PublisherImpl::writer_enabled");
      return ::DDS::RETCODE_ERROR;
    }
  catch (const CORBA::UserException& userex)
    {
      userex._tao_print_exception (
        "ERROR:  Exception"
        " in PublisherImpl::writer_enabled");
      return ::DDS::RETCODE_ERROR;
    }

  {
    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                      guard,
                      this->pi_lock_,
                      ::DDS::RETCODE_ERROR);
    DataWriterMap::iterator it =
      datawriter_map_.insert(DataWriterMap::value_type(topic_name, info));
    if (it == datawriter_map_.end ())
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) ERROR: ")
                           ACE_TEXT("PublisherImpl::writer_enabled, ")
                           ACE_TEXT("insert datawriter(topic_name=%s) ")
                           ACE_TEXT("failed.\n"),
                           topic_name),
                          ::DDS::RETCODE_ERROR);
      }

    std::pair<PublicationMap::iterator, bool> pair =
      publication_map_.insert(
        PublicationMap::value_type(info->publication_id_, info));

    if (pair.second == false)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) ERROR: ")
                           ACE_TEXT("PublisherImpl::writer_enabled, ")
                           ACE_TEXT("insert publication(id=%d) failed.\n"),
                           info->publication_id_),
                          ::DDS::RETCODE_ERROR);
      }

    // Increase ref count when the servant is added to the
    // datawriter/publication map.
    info->local_writer_impl_->_add_ref ();
  }

  return ::DDS::RETCODE_OK;
}

::DDS::ReturnCode_t
PublisherImpl::data_available(DataWriterImpl* writer,
                              bool resend)
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    this->pi_lock_,
                    ::DDS::RETCODE_ERROR);

  DataSampleList list;

  if (resend)
    {
      list = writer->get_resend_data();
    }
  else
    {
      list = writer->get_unsent_data();
    }

  if( this->suspend_depth_count_ > 0)
    {
      // append list to the avaliable data list.
      // Collect samples from all of the Publisher's Datawriters
      // in this list so when resume_publication is called
      // the Publisher does not have to iterate over its
      // DataWriters to get the unsent data samples.
      available_data_list_.enqueue_tail_next_send_sample (list);
    }
  else
    {
      // Do LATENCY_BUDGET processing here.
      // Do coherency processing here.
      // tell the transport to send the data sample(s).
      this->send(list);
    }

  return ::DDS::RETCODE_OK;
}


::DDS::PublisherListener*
PublisherImpl::listener_for (::DDS::StatusKind kind)
{
  // per 2.1.4.3.1 Listener Access to Plain Communication Status
  // use this entities factory if listener is mask not enabled
  // for this kind.
  if ((listener_mask_ & kind) == 0)
    {
      return participant_->listener_for (kind);
    }
  else
    {
      return fast_listener_;
    }
}

} // namespace DCPS
} // namespace OpenDDS


#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

template class std::multimap<ACE_CString, PublisherDataWriterInfo*>;
template class std::map< PublicationId, PublisherDataWriterInfo*>;

#elif defined(ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate std::multimap<ACE_CString, PublisherDataWriterInfo*>
#pragma instantiate std::map< PublicationId, PublisherDataWriterInfo*>

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
