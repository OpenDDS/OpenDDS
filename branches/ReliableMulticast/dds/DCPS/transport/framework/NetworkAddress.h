// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_NETWORKADDRESS_H
#define TAO_DCPS_NETWORKADDRESS_H

#include  "dds/DCPS/dcps_export.h"
#include  "ace/INET_Addr.h"


namespace TAO
{

  namespace DCPS
  {

    /**
     * @struct NetworkAddress
     *
     * @brief Defines a wrapper around ip/port always held in network order.
     *
     * This is used to send/receive an ACE_INET_Addr through the transport.
     */
    struct TAO_DdsDcps_Export NetworkAddress
    {
      /// Default Ctor
      NetworkAddress();

      /// Ctor using ACE_INET_Addr to obtain ip and port values.
      NetworkAddress(const ACE_INET_Addr& addr);

      /// Accessor to populate the provided ACE_INET_Addr object from the
      /// network order ip and port number.  The resulting ACE_INET_Addr
      /// object will be in local order.
      void to_addr(ACE_INET_Addr& addr) const;

      /// The network order ip address (4 bytes)
      ACE_UINT32 ip_;

      /// The network order port number (2 bytes)
      u_short port_;
    };

  }

}

// CLEANUP
#if 0
//MJM: would it help to overload insertion and extraction operators
//MJM: to/from message block chains at this point?
#endif

#if defined(__ACE_INLINE__)
#include "NetworkAddress.inl"
#endif /* __ACE_INLINE__ */

#endif /* TAO_DCPS_NETWORKADDRESS_H */
