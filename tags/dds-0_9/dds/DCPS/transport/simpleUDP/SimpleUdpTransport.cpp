// -*- C++ -*-
//
// $Id$

#include  "SimpleUdp_pch.h"
#include  "SimpleUdpTransport.h"
#include  "SimpleUdpSendStrategy.h"
#include  "SimpleUdpReceiveStrategy.h"
#include  "dds/DCPS/transport/framework/NetworkAddress.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUdpTransport.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::SimpleUdpTransport::~SimpleUdpTransport()
{
  DBG_ENTRY("SimpleUdpTransport","~SimpleUdpTransport");
}


TAO::DCPS::DataLink*
TAO::DCPS::SimpleUdpTransport::find_or_create_datalink
                         (const TransportInterfaceInfo& remote_info,
                          int                           connect_as_publisher)
{
  DBG_ENTRY("SimpleUdpTransport","find_or_create_datalink");

  // For UDP, we don't care why we are connecting.
  ACE_UNUSED_ARG(connect_as_publisher);

  // TBD SOON - Add method(s) to NetworkAddress to do this ugly stuff.
  // Get the remote address from the "blob" in the remote_info struct.
  NetworkAddress* network_order_address =
                          (NetworkAddress*)(remote_info.data.get_buffer());

  ACE_INET_Addr remote_address;
  network_order_address->to_addr(remote_address);

  // First, we have to try to find an existing (connected) DataLink
  // that suits the caller's needs.
  GuardType guard(this->links_lock_);

  SimpleUdpDataLink_rch link;

  if (this->links_.find(remote_address,link) == 0)
    {
      // This means we found a suitable DataLink.
      // We can return it now since we are done.
      return link._retn();
    }

  // The "find" part of the find_or_create_datalink has been attempted, and
  // we failed to find a suitable DataLink.  This means we need to move on
  // and attempt the "create" part of "find_or_create_datalink".

  // Here is where we actually create the DataLink.
  link = new SimpleUdpDataLink(remote_address, this);

  // Attempt to bind the SimpleUdpDataLink to our links_ map.
  if (this->links_.bind(remote_address,link) != 0)
    {
      // We failed to bind the new DataLink into our links_ map.
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to bind new SimpleUdpDataLink to "
                 "SimpleUdpTransport in links_ map.\n"));

      // On error, we return a NULL pointer.
      return 0;
    }

  TransportSendStrategy_rch send_strategy =
                       new SimpleUdpSendStrategy(this->udp_config_.in(),
                                                 remote_address,
                                                 this->socket_.in());

  if (link->connect(send_strategy.in()) != 0)
    {
      return 0;
    }

  // That worked.  No need for any connection establishment logic since
  // we are dealing with a "connection-less" protocol (UDP).
  return link._retn();
}


int
TAO::DCPS::SimpleUdpTransport::configure_i(TransportConfiguration* config)
{
  DBG_ENTRY("SimpleUdpTransport","configure_i");

  // Downcast the config argument to a SimpleUdpConfiguration*
  SimpleUdpConfiguration* udp_config = ACE_static_cast(SimpleUdpConfiguration*,
                                                       config);

  if (udp_config == 0)
    {
      // The downcast failed.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed downcast from TransportConfiguration "
                        "to SimpleUdpConfiguration.\n"),
                       -1);
    }

  // Ask our base class for a "copy" of the reference to the reactor task.
  this->reactor_task_ = reactor_task();

  if (this->reactor_task_.is_nil())
    {
      // It looks like our base class has either been shutdown, or it has
      // erroneously never been supplied with the reactor task.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleUdpTransport requires a reactor in "
                        "order to open its acceptor_.\n"),
                       -1);
    }

  // Make a "copy" of the reference for ourselves.
  udp_config->_add_ref();
  this->udp_config_ = udp_config;

  this->local_address_ = this->udp_config_->local_address_;

  // Open our socket using the local_address_ from our udp_config_ object.
  if (this->socket_->open_socket(this->local_address_) != 0)
    {
      // Remember to drop our reference to the udp_config_ object since
      // we are about to return -1 here, which means we are not supposed
      // to keep a copy after all.
      this->udp_config_ = 0;

      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Acceptor failed to open %s:%d: %p\n",
                        this->local_address_.get_host_addr (), 
                        this->local_address_.get_port_number (),
                        "open"),
                       -1);
    }

  // Create and start the receive strategy now.
  this->receive_strategy_ =
                      new SimpleUdpReceiveStrategy(this,
                                                   this->socket_.in(),
                                                   this->reactor_task_.in());
  if (this->receive_strategy_->start() == 0)
    {
      // Ahhh...  The sweet smell of success!
      return 0;
    }

  // Crud.  We had a problem starting the receive strategy.

  // Drop our reference to the TransportReceiveStrategy object.
  this->receive_strategy_ = 0;

  // Drop our reference to the SimpleUdpConfiguration object.
  this->udp_config_ = 0;

  // Close the socket.
  this->socket_->close_socket();

  // Drop our reference to the TransportReactorTask
  this->reactor_task_ = 0;

  // Drop our reference to the SimpleUdpSocket object.
  this->socket_ = 0;

  // The sour taste of failure!
  return -1;
}


void
TAO::DCPS::SimpleUdpTransport::shutdown_i()
{
  DBG_ENTRY("SimpleUdpTransport","shutdown_i");

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

  // Drop our reference to the SimpleUdpConfiguration object.
  this->udp_config_ = 0;

  // Drop our reference to the TransportReactorTask
  this->reactor_task_ = 0;

  // Drop our reference to the socket
  this->socket_ = 0;

#if 0
  // Drop our reference to the TransportReceiveStrategy object.
  this->receive_strategy_ = 0;
#endif
}


int
TAO::DCPS::SimpleUdpTransport::connection_info_i
                                   (TransportInterfaceInfo& local_info) const
{
  DBG_ENTRY("SimpleUdpTransport","connection_info_i");

  NetworkAddress network_order_address(this->local_address_);

  // Allow DCPSInfo to check compatibility of transport implemenations.
  local_info.transport_id = 2; // TBD Change magic number into a enum or constant value.
  // TBD SOON - Move the local_info.data "population" to the NetworkAddress.
  local_info.data = TAO::DCPS::TransportInterfaceBLOB
                                    (sizeof(NetworkAddress),
                                     sizeof(NetworkAddress),
                                     (CORBA::Octet*)(&network_order_address));

  return 0;
}


void
TAO::DCPS::SimpleUdpTransport::release_datalink_i(DataLink* link)
{
  DBG_ENTRY("SimpleUdpTransport","release_datalink_i");

  SimpleUdpDataLink* udp_link = ACE_static_cast(SimpleUdpDataLink*,link);

  if (udp_link == 0)
    {
      // Really an assertion failure
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) INTERNAL ERROR - Failed to downcast DataLink to "
                 "SimpleUdpDataLink.\n"));
      return;
    }

  // Get the remote address from the SimpleUdpDataLink to be used as a key.
  ACE_INET_Addr remote_address = udp_link->remote_address();

  SimpleUdpDataLink_rch released_link;

  GuardType guard(this->links_lock_);

  // Attempt to remove the SimpleUdpDataLink from our links_ map.
  if (this->links_.unbind(remote_address, released_link) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to locate DataLink in order to "
                 "release it.\n"));
    }
}

