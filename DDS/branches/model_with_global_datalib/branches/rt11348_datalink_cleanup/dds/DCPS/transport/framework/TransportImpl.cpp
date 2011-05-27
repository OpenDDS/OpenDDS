// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportImpl.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/Util.h"
#include "tao/debug.h"

#if !defined (__ACE_INLINE__)
#include "TransportImpl.inl"
#endif /* __ACE_INLINE__ */

namespace
{
  template <typename Container>
  void clear(Container& c)
  {
    Container copy;
    copy.swap(c);
    for (typename Container::iterator itr = copy.begin();
      itr != copy.end();
      ++itr)
    {
      itr->second->_remove_ref ();
    }
  }
}

OpenDDS::DCPS::TransportImpl::~TransportImpl()
{
  DBG_ENTRY_LVL("TransportImpl","~TransportImpl",6);
  PendingAssociationsMap::iterator penditer =
    pending_association_sub_map_.begin();

  while(penditer != pending_association_sub_map_.end())
  {
    delete (penditer->second);
    ++penditer;
  }

  clear(dw_map_);
  clear(dr_map_);

  // The DL Cleanup task belongs to the Transportimpl object.
  // Cleanup before leaving the house.
  this->dl_clean_task_.close (1);
  this->dl_clean_task_.wait ();
}


void
OpenDDS::DCPS::TransportImpl::shutdown()
{
  DBG_ENTRY_LVL("TransportImpl","shutdown",6);

  if (! this->reactor_task_.is_nil ())
  {
    this->reactor_task_->stop ();
    this->reactor_task_ = 0;
  }

  this->pre_shutdown_i();

  {
    GuardType guard(this->lock_);

    if (this->config_.is_nil())
      {
        // This TransportImpl is already shutdown.
//MJM: So, I read here that config_i() actually "starts" us?
        return;
      }

    for (InterfaceMapType::iterator itr = interfaces_.begin();
         itr != interfaces_.end();
         ++itr)
      {
        itr->second->transport_detached();
      }

    // Clear our collection of TransportInterface pointers.
    interfaces_.clear();

    // Drop our references to the config_ and reactor_task_.
    this->config_ = 0;
    this->reactor_task_ = 0;

//MJM: Won't you need to ACE_UNUSED_ARG here since you are depending on
//MJM: side effects here?

    // We can release our lock_ now.
  }

  // Tell our subclass about the "shutdown event".
  this->shutdown_i();
}


OpenDDS::DCPS::DataLink*
OpenDDS::DCPS::TransportImpl::reserve_datalink
                      (const TransportInterfaceInfo& remote_subscriber_info,
                       RepoId                        subscriber_id,
                       RepoId                        publisher_id,
                       CORBA::Long                   priority)
{
  DBG_ENTRY_LVL("TransportImpl","reserve_datalink",6);

  // Not used right now - not sure how it would apply either.
  ACE_UNUSED_ARG(priority);

  // Ask our concrete subclass to find or create a (concrete) DataLink
  // that matches the supplied criterea.

  // Note that we pass-in a 1 as the second argument.  This means that
  // if a new DataLink needs to be created (ie, the find operation fails),
  // then the connection establishment logic will treat the local endpoint
  // as a publisher.  This knowledge dictates whether a passive or active
  // connection establishment procedure should be followed.
  DataLink_rch link = this->find_or_create_datalink(remote_subscriber_info, 1);

  if (link.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportImpl subclass was unable to find "
                        "or create a DataLink for local publisher_id (%d) "
                        "to remote subscriber_id (%d).\n",
                        publisher_id, subscriber_id),
                       0);
    }

  link->make_reservation(subscriber_id,publisher_id);

  return link._retn();
}


OpenDDS::DCPS::DataLink*
OpenDDS::DCPS::TransportImpl::reserve_datalink
                      (const TransportInterfaceInfo& remote_publisher_info,
                       RepoId                        publisher_id,
                       RepoId                        subscriber_id,
                       TransportReceiveListener*     receive_listener,
                       CORBA::Long                   priority)
{
  DBG_ENTRY_LVL("TransportImpl","reserve_datalink",6);

  // Not used right now - not sure how it would apply either.
  ACE_UNUSED_ARG(priority);

  // Ask our concrete subclass to find or create a DataLink (actually, a
  // concrete subclass of DataLink) that matches the supplied criterea.
  // Since find_or_create() is pure virtual, the concrete subclass must
  // provide an implementation for us to use.

  // Note that we pass-in a 0 as the second argument.  This means that
  // if a new DataLink needs to be created (ie, the find operation fails),
  // then the connection establishment logic will treat the local endpoint
  // as a subscriber.  This knowledge dictates whether a passive or active
  // connection establishment procedure should be followed.
  DataLink_rch link = this->find_or_create_datalink(remote_publisher_info,0);

  if (link.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportImpl subclass was unable to find "
                        "or create a DataLink for local publisher_id (%d) "
                        "to remote subscriber_id (%d).\n",
                        publisher_id, subscriber_id),
                       0);
    }

  link->make_reservation(publisher_id,
                         subscriber_id,
                         receive_listener);

  // This is called on the subscriber side to let the concrete
  // datalink to do some necessary work such as SimpleTcp will
  // send the FULLY_ASSOCIATED ack to the publisher.
  link->fully_associated ();

  return link._retn();
}


/// This is called by a TransportInterface object when it is handling
/// its own request to attach_transport(TransportImpl*), and this
/// TransportImpl object is the one to which it should be attached.
OpenDDS::DCPS::AttachStatus
OpenDDS::DCPS::TransportImpl::attach_interface(TransportInterface* interface)
{
  DBG_ENTRY_LVL("TransportImpl","attach_interface",6);

  GuardType guard(this->lock_);

  if (this->config_.is_nil())
    {
      // Can't attach to a TransportImpl that isn't currently configured.
      // This could mean that this TransportImpl object has never had its
      // configure() method called, or it could mean that this TransportImpl
      // object was shutdown() after the configure() method was called.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Cannot attach_listener() to TransportImpl "
                        "object because TransportImpl is not configured, "
                        "or has been shutdown.\n"),
                       ATTACH_BAD_TRANSPORT);
    }

    if (bind(interfaces_, interface, interface) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Cannot attach_listener() to TransportImpl "
                        "object because TransportImpl thinks the "
                        "TransportInterface object is already attached.\n"),
                       ATTACH_BAD_TRANSPORT);
    }

  // Everything worked.  Return success code.
  return ATTACH_OK;
}


int
OpenDDS::DCPS::TransportImpl::register_publication (OpenDDS::DCPS::RepoId pub_id,
                                                OpenDDS::DCPS::DataWriterImpl* dw)
{
  DBG_ENTRY_LVL("TransportImpl","register_publication",6);
  GuardType guard(this->lock_);

  int ret =
    bind(dw_map_, pub_id, dw);

  if (ret != -1)
    {
      dw->_add_ref ();
    }

  // It's possiable this function is called after the
  // add_association is handled and also the FULLY_ASSOCIATED
  // ack is received by the publisher side, we need check the
  // map to see if it's the case. If it is,the datawriter will be
  // notified fully associated at this time.
  OpenDDS::DCPS::RepoIdSet_rch pending_subs
    = this->pending_sub_map_.find (pub_id);
  if (! pending_subs.is_nil () && this->acked (pub_id))
    this->fully_associated (pub_id);

  if (::OpenDDS::DCPS::Transport_debug_level > 4)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) transport %x added publication %d.\n"),
        this, pub_id
      ));
    }

  return ret;
}


int
OpenDDS::DCPS::TransportImpl::unregister_publication (OpenDDS::DCPS::RepoId pub_id)
{
  DBG_ENTRY_LVL("TransportImpl","unregister_publication",6);
  GuardType guard(this->lock_);
  PublicationObjectMap::iterator iter = dw_map_.find(pub_id);
  int ret = -1;
  if (iter != dw_map_.end())
  {
    ret = 0;
    if (iter->second != 0)
      iter->second->_remove_ref ();
    dw_map_.erase(iter);
  }

  if (::OpenDDS::DCPS::Transport_debug_level > 4)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) transport %x released publication %d.\n"),
        this, pub_id
      ));
    }

  return ret;
}


OpenDDS::DCPS::DataWriterImpl*
OpenDDS::DCPS::TransportImpl::find_publication (OpenDDS::DCPS::RepoId pub_id, bool safe_cpy)
{
  DBG_ENTRY_LVL("TransportImpl","find_publication",6);
  GuardType guard(this->lock_);
  PublicationObjectMap::iterator iter = dw_map_.find(pub_id);
  if (iter == dw_map_.end())
    {
      if (::OpenDDS::DCPS::Transport_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, "(%P|%t)TransportImpl::find_publication   pub(%d) "
            "not found\n", pub_id));
        }
      return 0;
    }
  else if (safe_cpy && iter->second != 0) {
    iter->second->_add_ref ();
  }

  return iter->second;
}


int
OpenDDS::DCPS::TransportImpl::register_subscription (OpenDDS::DCPS::RepoId sub_id,
                                                 OpenDDS::DCPS::DataReaderImpl* dr)
{
  DBG_ENTRY_LVL("TransportImpl","register_subscription",6);
  GuardType guard(this->lock_);

  int ret =
    bind(dr_map_, sub_id, dr);

  if (ret != -1)
  {
    dr->_add_ref ();
  }

  if (::OpenDDS::DCPS::Transport_debug_level > 4)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) transport %x added subscription %d.\n"),
        this, sub_id
      ));
    }

  return ret;
}


int
OpenDDS::DCPS::TransportImpl::unregister_subscription (OpenDDS::DCPS::RepoId sub_id)
{
  DBG_ENTRY_LVL("TransportImpl","unregister_subscription",6);
  GuardType guard(this->lock_);

  SubscriptionObjectMap::iterator iter = dr_map_.find(sub_id);
  if (iter != dr_map_.end())
  {
    if (iter->second != 0)
      iter->second->_remove_ref ();
    dr_map_.erase(iter);

  } else {
    ACE_ERROR((LM_WARNING,
      ACE_TEXT("(%P|%t) WARNING: TransportImpl::unregister_subscription ")
      ACE_TEXT("subscription (id==%d) not found to unregister.\n"),
      sub_id
    ));
  }

  if (::OpenDDS::DCPS::Transport_debug_level > 4)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) transport %x released subscription %d.\n"),
        this, sub_id
      ));
    }

  // We can't fail here - at this point the subscription is _not_
  // registered with this transport.
  return 0;
}


OpenDDS::DCPS::DataReaderImpl*
OpenDDS::DCPS::TransportImpl::find_subscription (OpenDDS::DCPS::RepoId sub_id, bool safe_cpy)
{
  DBG_ENTRY_LVL("TransportImpl","find_subscription",6);
  GuardType guard(this->lock_);
  SubscriptionObjectMap::iterator iter = dr_map_.find(sub_id);
  if (iter == dr_map_.end())
    {
      if (::OpenDDS::DCPS::Transport_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, "(%P|%t)TransportImpl::find_subscription   sub(%d) "
            "not found\n", sub_id));
        }
      return 0;
    }
  else if (safe_cpy && iter->second != 0) {
    iter->second->_add_ref ();
  }

  return iter->second;
}


int
OpenDDS::DCPS::TransportImpl::add_pending_association (RepoId  pub_id,
                                                   size_t                  num_remote_associations,
                                                   const AssociationData*  remote_associations)
{
  DBG_ENTRY_LVL("TransportImpl","add_pending_association",6);

  GuardType guard(this->lock_);

  // Add the remote_id to the pending publication map.
  for (size_t i = 0; i < num_remote_associations; ++i)
  {
    if (this->pending_sub_map_.insert (pub_id, remote_associations [i].remote_id_) != 0)
      return 0;
  }

  AssociationInfo info;
  info.num_associations_ = num_remote_associations;
  info.association_data_ = remote_associations;
  info.status_ = Not_Fully_Associated;

  // Cache the Association data so it can be used for the callback
  // to notify datawriter on_publication_match.

  PendingAssociationsMap::iterator iter = pending_association_sub_map_.find(pub_id);
  if (iter != pending_association_sub_map_.end())
    iter->second->push_back (info);
  else {
    AssociationInfoList* infos = new AssociationInfoList;
    infos->push_back (info);
    if (bind(pending_association_sub_map_, pub_id, infos) == -1)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
      "(%P|%t) ERROR: add_pending_association: Failed to add pending associations for pub %d\n",
      pub_id),
      -1);
    }
  }

  if (this->acked (pub_id))
    this->fully_associated (pub_id);

  return 0;
}


int
OpenDDS::DCPS::TransportImpl::demarshal_acks (ACE_Message_Block* acks, bool byte_order)
{
  DBG_ENTRY_LVL("TransportImpl","demarshal",6);

  int status = this->acked_sub_map_.demarshal (acks, byte_order);
  if (status == -1)
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TransportImpl::demarshal_acks failed\n"),
                      -1);

  GuardType guard(this->lock_);

  RepoIdSet acked_pubs;

  this->acked_sub_map_.get_keys (acked_pubs);

  RepoIdSet::MapType& acked_pubs_map = acked_pubs.map();

  for (RepoIdSet::MapType::iterator itr = acked_pubs_map.begin();
    itr != acked_pubs_map.end();
    ++itr)
  {
    OpenDDS::DCPS::RepoIdSet_rch pending_subs
      = this->pending_sub_map_.find (itr->first);

    if (! pending_subs.is_nil () && this->acked (itr->first))
      this->fully_associated (itr->first);
  }
  return 0;
}


void
OpenDDS::DCPS::TransportImpl::fully_associated (RepoId pub_id)
{
  DBG_ENTRY_LVL("TransportImpl","fully_associated",6);

  PublicationObjectMap::iterator pubiter = dw_map_.find(pub_id);

  AssociationInfoList* remote_associations = 0;
  PendingAssociationsMap::iterator penditer =
    pending_association_sub_map_.find(pub_id);

  if (
    (pubiter != dw_map_.end()) &&
    (penditer != pending_association_sub_map_.end())
    )
  {
    size_t len = penditer->second->size();
    for (size_t i = 0; i < len; ++i)
    {
      pubiter->second->fully_associated (pub_id,
      (*penditer->second)[i].num_associations_,
      (*penditer->second)[i].association_data_);
    }
    RepoIdSet_rch tmp = this->pending_sub_map_.remove_set (pub_id);
    tmp = this->acked_sub_map_.remove_set (pub_id);
    remote_associations = penditer->second;
    pending_association_sub_map_.erase(pub_id);
    delete remote_associations;
  }
  else if (penditer != pending_association_sub_map_.end())
  {
    size_t len = penditer->second->size();
    for (size_t i = 0; i < len; ++i)
      (*penditer->second)[i].status_ = Fully_Associated;
  }
}


bool
OpenDDS::DCPS::TransportImpl::acked (RepoId pub_id)
{
  return this->pending_sub_map_.is_subset (this->acked_sub_map_, pub_id);
}

bool
OpenDDS::DCPS::TransportImpl::release_link_resources (DataLink* link)
{
  DBG_ENTRY_LVL("TransportImpl", "release_link_resources",6);

  // Create a smart pointer without ownership (bumps up ref count)
  DataLink_rch dl (link, false);

  dl_clean_task_.add (dl);

  return true;
}
