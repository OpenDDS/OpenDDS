// -*- C++ -*-
//

#ifndef OPENDDS_DCPS_PACKETSERIALIZER_H
#define OPENDDS_DCPS_PACKETSERIALIZER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include <cstring>

namespace OpenDDS
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

          /// Returns a pointer to the first byte in the buffer that is
          /// used - this is due to alignment in ACE_OutputCDR.
          char* serializeFromTo(
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

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "PacketSerializer.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_PACKETSERIALIZER_H */
