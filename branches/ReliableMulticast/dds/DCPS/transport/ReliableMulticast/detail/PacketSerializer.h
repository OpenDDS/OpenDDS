// -*- C++ -*-
//

#ifndef TAO_DCPS_PACKETSERIALIZER_H
#define TAO_DCPS_PACKETSERIALIZER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        struct Packet;

        class ReliableMulticast_Export PacketSerializer
        {
        public:
          /// Create a buffer appropriately-sized for the input Packet and
          /// return it.  This buffer is owned by the caller!
          char* getBuffer(
            const Packet& packet,
            size_t& size
            ) const;

          void serializeFromTo(
            const Packet& packet,
            char* buffer,
            size_t size
            ) const;

          void serializeFromTo(
            const char* buffer,
            size_t size,
            Packet& packet
            ) const;
        };

      } /* namespace detail */

    } /* namespace ReliableMulticast */

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "PacketSerializer.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_PACKETSERIALIZER_H */
