// -*- C++ -*-
//

#ifndef TAO_DCPS_PACKETSERIALIZER_H
#define TAO_DCPS_PACKETSERIALIZER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        class Packet;

        class PacketSerializer
        {
        public:
          PacketSerializer(const Packet& packet);
          PacketSerializer(Packet& packet);

          /// Create a buffer appropriately-sized for the input Packet and
          /// return it.  This buffer is owned by the caller!
          char* getBuffer(const Packet& packet) const;

          void serializeFromTo(
            const Packet& packet,
            char* buffer
            ) const;

          void serializeFromTo(
            char* buffer,
            Packet& packet
            );
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
