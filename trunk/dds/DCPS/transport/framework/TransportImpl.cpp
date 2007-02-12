// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportImpl.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "tao/debug.h"

#if !defined (__ACE_INLINE__)
#include "TransportImpl.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::TransportImpl::~TransportImpl()
{
  DBG_ENTRY_LVL("TransportImpl","~TransportImpl",5);
  {
    PublicationObjectMap::ENTRY* entry;
    for (PublicationObjectMap::ITERATOR itr(dw_map_);
      itr.next(entry);
      itr.advance ())
    {
      entry->int_id_->_remove_ref ();
    }
  }

  {
    SubscriptionObjectMap::ENTRY* entry;
    for (SubscriptionObjectMap::ITERATOR itr(dr_map_);
      itr.next(entry);
      itr.advance ())
    {
      entry->int_id_->_remove_ref ();
    }
  }

  // The DL Cleanup task belongs to the Transportimpl object.
  // Cleanup before leaving the house.
  this->dl_clean_task_.close (1);
  this->dl_clean_task_.wait ();
}


void
TAO::DCPS::TransportImpl::shutdown()
{
  DBG_ENTRY_LVL("TransportImpl","shutdown",5);

  this->pre_shutdown_i();

  {
    GuardType guard(this->lock_);

    if (this->config_.is_nil())
      {
        // This TransportImpl is already shutdown.
//MJM: So, I read here that config_i() actually "starts" us?
        return;
      }

    InterfaceMapType::ENTRY* entry;

    for (InterfaceMapType::ITERATOR itr(this->interfaces_);
         itr.next(entry);
         itr.advance())
      {
        entry->int_id_->transport_detached();
      }

    // Clear our collection of TransportInterface pointers.
    this->interfaces_.unbind_all();

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


TAO::DCPS::DataLink*
TAO::DCPS::TransportImpl::reserve_datalink
                      (const TransportInterfaceInfo& remote_subscriber_info,
                       RepoId                        subscriber_id,
                       RepoId                        publisher_id,
                       CORBA::Long                   priority)
{
  DBG_SUB_ENTRY("TransportImpl","reserve_datalink",5);

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


TAO::DCPS::DataLink*
TAO::DCPS::TransportImpl::reserve_datalink
                      (const TransportInterfaceInfo& remote_publisher_info,
                       RepoId                        publisher_id,
                       RepoId                        subscriber_id,
                       TransportReceiveListener*     receive_listener,
                       CORBA::Long                   priority)
{
  DBG_SUB_ENTRY("TransportImpl","reserve_datalink",5);

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
TAO::DCPS::AttachStatus
TAO::DCPS::TransportImpl::attach_interface(TransportInterface* interface)
{
  DBG_ENTRY_LVL("TransportImpl","attach_interface",5);

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

  if (this->interfaces_.bind(interface,interface) != 0)
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
TAO::DCPS::TransportImpl::register_publication (TAO::DCPS::RepoId pub_id,
                                                TAO::DCPS::DataWriterImpl* dw)
{
  DBG_ENTRY_LVL("TransportImpl","register_publication",5);
  GuardType guard(this->lock_);

  int ret = this->dw_map_.bind (pub_id, dw);
  if (ret != -1)
    {
      dw->_add_ref ();
    }

  // It's possiable this function is called after the
  // add_association is handled and also the FULLY_ASSOCIATED
  // ack is received by the publisher side, we need check the
  // map to see if it's the case. If it is,the datawriter will be
  // notified fully associated at this time.
  TAO::DCPS::RepoIdSet_rch pending_subs
    = this->pending_sub_map_.find (pub_id);
  if (! pending_subs.is_nil () && this->acked (pub_id))
    this->fully_associated (pub_id);

  return ret;
}


int
TAO::DCPS::TransportImpl::unregister_publication (TAO::DCPS::RepoId pub_id)
{
  DBG_ENTRY_LVL("TransportImpl","unregister_publication",5);
  GuardType guard(this->lock_);
  DataWriterImpl* dw = 0;
  int result = this->dw_map_.unbind (pub_id, dw);
  if (dw != 0)
    dw->_remove_ref ();

  return result;
}


TAO::DCPS::DataWriterImpl*
TAO::DCPS::TransportImpl::find_publication (TAO::DCPS::RepoId pub_id, bool safe_cpy)
{
  DBG_ENTRY_LVL("TransportImpl","find_publication",5);
  GuardType guard(this->lock_);
  TAO::DCPS::DataWriterImpl* dw = 0;
  if (this->dw_map_.find (pub_id, dw) == -1)
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, "(%P|%t)TransportImpl::find_publication   pub(%d) "
            "not found\n", pub_id));
        }
    }

  if (safe_cpy) {
    dw->_add_ref ();
  }

  return dw;
}


int
TAO::DCPS::TransportImpl::register_subscription (TAO::DCPS::RepoId sub_id,
                                                 TAO::DCPS::DataReaderImpl* dr)
{
  DBG_ENTRY_LVL("TransportImpl","register_subscription",5);
  GuardType guard(this->lock_);

  int ret = this->dr_map_.bind (sub_id, dr);
  if (ret != -1)
  {
    dr->_add_ref ();
  }

  return ret;
}


int
TAO::DCPS::TransportImpl::unregister_subscription (TAO::DCPS::RepoId sub_id)
{
  DBG_ENTRY_LVL("TransportImpl","unregister_subscription",5);
  GuardType guard(this->lock_);

  DataReaderImpl* dr = 0;
  int result = this->dr_map_.unbind (sub_id, dr);
  if (dr != 0)
    dr->_remove_ref ();

  return result;
}


TAO::DCPS::DataReaderImpl*
TAO::DCPS::TransportImpl::find_subscription (TAO::DCPS::RepoId sub_id, bool safe_cpy)
{
  DBG_ENTRY_LVL("TransportImpl","find_subscription",5);
  GuardType guard(this->lock_);
  TAO::DCPS::DataReaderImpl* dr = 0;
  if (this->dr_map_.find (sub_id, dr) == -1)
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, "(%P|%t)TransportImpl::find_subscription   sub(%d) not found\n",
            sub_id));
        }
    }

  if (safe_cpy) {
    dr->_add_ref ();
  }

  return dr;
}


int
TAO::DCPS::TransportImpl::add_pending_association (RepoId  pub_id,
                                                   size_t                  num_remote_associations,
                                                   const AssociationData*  remote_associations)
{
  DBG_ENTRY_LVL("TransportImpl","add_pending_association",5);

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

  PendingAssociationsMap::ENTRY* entry;
  if (this->pending_association_sub_map_.find (pub_id, entry) == 0)
    entry->int_id_->push_back (info);
  else {
    AssociationInfoList* infos = new AssociationInfoList;
    infos->push_back (info);
    if (this->pending_association_sub_map_.bind (pub_id, infos) == -1)
      ACE_ERROR_RETURN ((LM_ERROR,
      "(%P|%t) ERROR: add_pending_association: Failed to add pending associations for pub %d\n",
      pub_id),
      -1);
  }

  if (this->acked (pub_id))
    this->fully_associated (pub_id);

  return 0;
}


int
TAO::DCPS::TransportImpl::demarshal_acks (ACE_Message_Block* acks, bool byte_order)
{
  DBG_ENTRY_LVL("TransportImpl","demarshal",5);

  int status = this->acked_sub_map_.demarshal (acks, byte_order);
  if (status == -1)
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TransportImpl::demarshal_acks failed\n"),
                      -1);

  GuardType guard(this->lock_);

  RepoIdSet acked_pubs;

  this->acked_sub_map_.get_keys (acked_pubs);

  RepoIdSet::MapType::ENTRY* entry;

  for (RepoIdSet::MapType::ITERATOR itr(acked_pubs.map ());
    itr.next(entry);
    itr.advance())
  {
    TAO::DCPS::RepoIdSet_rch pending_subs
      = this->pending_sub_map_.find (entry->ext_id_);

    if (! pending_subs.is_nil () && this->acked (entry->ext_id_))
      this->fully_associated (entry->ext_id_);
  }
  return 0;
}


void
TAO::DCPS::TransportImpl::fully_associated (RepoId pub_id)
{
  DBG_ENTRY_LVL("TransportImpl","fully_associated",5);

  TAO::DCPS::DataWriterImpl* dw = 0;
  int ret = this->dw_map_.find (pub_id, dw);

  AssociationInfoList* remote_associations = 0;
  int status = this->pending_association_sub_map_.find (pub_id, remote_associations);
  size_t len = remote_associations->size ();

  if (ret == 0 && status == 0)
  {
    for (size_t i = 0; i < len; ++i)
    {
      dw->fully_associated (pub_id,
      (*remote_associations)[i].num_associations_,
      (*remote_associations)[i].association_data_);
    }
    RepoIdSet_rch tmp = this->pending_sub_map_.remove_set (pub_id);
    tmp = this->acked_sub_map_.remove_set (pub_id);
    delete remote_associations;
    this->pending_association_sub_map_.unbind (pub_id);
  }
  else if (status == 0)
  {
    for (size_t i = 0; i < len; ++i)
      (*remote_associations)[i].status_ = Fully_Associated;
  }
}


bool
TAO::DCPS::TransportImpl::acked (RepoId pub_id)
{
  return this->pending_sub_map_.equal (this->acked_sub_map_, pub_id);
}

bool
TAO::DCPS::TransportImpl::release_link_resources (DataLink* link)
{
  DBG_ENTRY_LVL("TransportImpl", "release_link_resources", 5);

  // Create a smart pointer without ownership (bumps up ref count)
  DataLink_rch dl (link, false);

  dl_clean_task_.add (dl);

  return true;
}
