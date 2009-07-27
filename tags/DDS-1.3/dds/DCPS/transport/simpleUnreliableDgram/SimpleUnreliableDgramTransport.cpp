// -*- C++ -*-
//
// $Id$

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUnreliableDgramTransport.h"
#include "SimpleUnreliableDgramSendStrategy.h"
#include "SimpleUnreliableDgramReceiveStrategy.h"
#include "SimpleUnreliableDgramSynchResource.h"
#include "SimpleUnreliableDgramSocket.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/DirectPriorityMapper.h"
#include <vector>


#if !defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramTransport.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::SimpleUnreliableDgramTransport::~SimpleUnreliableDgramTransport()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramTransport","~SimpleUnreliableDgramTransport",6);
}


OpenDDS::DCPS::DataLink*
OpenDDS::DCPS::SimpleUnreliableDgramTransport::find_or_create_datalink
                         (const TransportInterfaceInfo& remote_info,
                          int                           connect_as_publisher,
                          int                           priority)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramTransport","find_or_create_datalink",6);

  // For MCAST, we don't care why we are connecting.
  ACE_UNUSED_ARG(connect_as_publisher);

  // Get the remote address from the "blob" in the remote_info struct.
  NetworkAddress network_order_address;

  ACE_InputCDR cdr ((const char*)remote_info.data.get_buffer(), remote_info.data.length ());

  if (cdr >> network_order_address == 0)
  {  
    ACE_ERROR ((LM_ERROR, "(%P|%t)SimpleUnreliableDgramTransport::find_or_create_datalink failed "
      "to de-serialize the NetworkAddress\n"));
    return 0;
  }

  ACE_INET_Addr remote_address;

  network_order_address.to_addr(remote_address);


  VDBG_LVL ((LM_DEBUG, "(%P|%t)SimpleUnreliableDgramTransport::find_or_create_datalink remote addr str \"%s\"\n",  
    network_order_address.addr_.c_str ()), 2);


  // First, we have to try to find an existing (connected) DataLink
  // that suits the caller's needs.
  GuardType guard(this->links_lock_);

  SimpleUnreliableDgramDataLink_rch link;

  if (this->links_.find( PriorityKey( priority, remote_address), link) == 0)
    {
      // This means we found a suitable DataLink.
      // We can return it now since we are done.
      return link._retn();
    }

  // The "find" part of the find_or_create_datalink has been attempted, and
  // we failed to find a suitable DataLink.  This means we need to move on
  // and attempt the "create" part of "find_or_create_datalink".

  // Here is where we actually create the DataLink.
  link = new SimpleUnreliableDgramDataLink(remote_address, this);

  // Attempt to bind the SimpleUnreliableDgramDataLink to our links_ map.
  if (this->links_.bind( PriorityKey( priority, remote_address), link) != 0)
    {
      // We failed to bind the new DataLink into our links_ map.
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to bind new SimpleUnreliableDgramDataLink to "
                 "SimpleUnreliableDgramTransport in links_ map.\n"));

      // On error, we return a NULL pointer.
      return 0;
    }

  // Set the DiffServ codepoint according to the TRANSPORT_PRIORITY
  // policy value.
  DirectPriorityMapper mapping( priority);
  link->set_dscp_codepoint( mapping.codepoint(), this->socket_->socket());

  TransportSendStrategy_rch send_strategy 
    = new SimpleUnreliableDgramSendStrategy(
            this->config_.in(),
            remote_address,
            this->socket_.in(),
            new SimpleUnreliableDgramSynchResource(
              this->socket_.in(),
              this,
              this->config_->max_output_pause_period_
            ),
            priority
          );

  if (link->connect(send_strategy.in()) != 0)
    {
      return 0;
    }

  // That worked.  No need for any connection establishment logic since
  // we are dealing with a "connection-less" protocol (MCAST).
  return link._retn();
}


int
OpenDDS::DCPS::SimpleUnreliableDgramTransport::configure_i(TransportConfiguration* config)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramTransport","configure_i",6);

  // Ask our base class for a "copy" of the reference to the reactor task.
  this->reactor_task_ = reactor_task();

  if (this->reactor_task_.is_nil())
    {
      // It looks like our base class has either been shutdown, or it has
      // erroneously never been supplied with the reactor task.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleUnreliableDgramTransport requires a reactor in "
                        "order to open its acceptor_.\n"),
                       -1);
    }

  config->_add_ref ();
  this->config_ = ACE_static_cast(SimpleUnreliableDgramConfiguration*, config);

  if (this->config_.is_nil ())
    {
      // The downcast failed.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed downcast from TransportConfiguration "
                        "to SimpleUnreliableDgramConfiguration.\n"),
                       -1);
    }

  if (this->configure_socket (config) == 0)
    {
        
      // Create and start the receive strategy now.
      this->receive_strategy_ =
                          new SimpleUnreliableDgramReceiveStrategy(this,
                                                      this->socket_.in(),
                                                      this->reactor_task_.in());
      if (this->receive_strategy_->start() == 0)
        {
          // Ahhh...  The sweet smell of success!
          return 0;
        }
    }

  // Crud.  We had a problem starting the receive strategy.

  // Drop our reference to the TransportReceiveStrategy object.
  this->receive_strategy_ = 0;

  // Drop our reference to the SimpleMcastConfiguration object.
  this->config_ = 0;

  // Close the socket.
  this->socket_->close_socket();

  // Drop our reference to the TransportReactorTask
  this->reactor_task_ = 0;

  // Drop our reference to the SimpleMcastSocket object.
  this->socket_ = 0;

  // The sour taste of failure!
  return -1;
}


void
OpenDDS::DCPS::SimpleUnreliableDgramTransport::shutdown_i()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramTransport","shutdown_i",6);

  // Stop the receive strategy
  this->receive_strategy_->stop();

  // Close our socket.
  this->socket_->close_socket();

  // Disconnect all of our DataLinks, and clear our links_ collection.
  {
    GuardType guard(this->links_lock_);

    AddrLinkMap::ENTRY* entry;

    for (AddrLinkMap::ITERATOR itr(this->links_);
         itr.next(entry);
         itr.advance())
      {
        entry->int_id_->transport_shutdown();
      }

    this->links_.unbind_all();
  }

  // Drop our reference to the socket
  this->socket_ = 0;

  // Drop our reference to the TransportReactorTask
  this->reactor_task_ = 0;

#if 0
  // Drop our reference to the TransportReceiveStrategy object.
  this->receive_strategy_ = 0;
#endif

  this->config_ = 0;
}


void
OpenDDS::DCPS::SimpleUnreliableDgramTransport::release_datalink_i(DataLink* link)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramTransport","release_datalink_i",6);

  SimpleUnreliableDgramDataLink* dgram_link = ACE_static_cast(SimpleUnreliableDgramDataLink*,link);

  if (dgram_link == 0)
    {
      // Really an assertion failure
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) INTERNAL ERROR - Failed to downcast DataLink to "
                 "SimpleUnreliableDgramDataLink.\n"));
      return;
    }

  // Get the remote address from the SimpleMcastDataLink to be used as a key.
  ACE_INET_Addr remote_address = dgram_link->remote_address();

  SimpleUnreliableDgramDataLink_rch released_link;

  GuardType guard(this->links_lock_);

  // Attempt to remove the SimpleMcastDataLink from our links_ map.
  PriorityKey key( dgram_link->transport_priority(), remote_address);
  if (this->links_.unbind( key, released_link) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to locate DataLink in order to "
                 "release it.\n"));
    }
}


void
OpenDDS::DCPS::SimpleUnreliableDgramTransport::notify_lost_on_backpressure_timeout ()
{
  typedef std::vector <SimpleUnreliableDgramDataLink_rch> LinkVector;
 
  LinkVector links;
  {
    GuardType guard(this->links_lock_);

    AddrLinkMap::ENTRY* entry;

    for (AddrLinkMap::ITERATOR itr(this->links_);
         itr.next(entry);
         itr.advance())
      links.push_back (entry->int_id_);
  }

  LinkVector::iterator itr_end = links.end ();

  for (LinkVector::iterator itr = links.begin ();
    itr != itr_end;
    ++ itr)
  {
    (*itr)->notify (DataLink::LOST);
    (*itr)->terminate_send ();
  }
}







