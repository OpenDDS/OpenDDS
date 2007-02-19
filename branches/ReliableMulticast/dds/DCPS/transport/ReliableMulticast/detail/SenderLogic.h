// -*- C++ -*-
//

#ifndef TAO_DCPS_SENDERLOGIC_H
#define TAO_DCPS_SENDERLOGIC_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include "Packet.h"
#include <vector>
#include <map>

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        class ReliableMulticast_Export SenderLogic
        {
        public:
          typedef std::vector<
            TAO::DCPS::ReliableMulticast::detail::Packet
            > PacketVector;

	  SenderLogic(size_t sender_history_size);

          void receive(
            const TAO::DCPS::ReliableMulticast::detail::Packet& p,
            PacketVector& redelivered
            ) const;

          void send(
            const TAO::DCPS::ReliableMulticast::detail::Packet& p,
            PacketVector& delivered
            );

          void make_heartbeat(TAO::DCPS::ReliableMulticast::detail::Packet& p);

        private:
          void buffer_packet(
            const TAO::DCPS::ReliableMulticast::detail::Packet& p,
            PacketVector& delivered
            );

          bool is_buffered(
            const TAO::DCPS::ReliableMulticast::detail::Packet& p
            ) const;

          size_t buffersize(
            ) const;

          typedef std::map<
            TAO::DCPS::ReliableMulticast::detail::Packet::id_type,
            TAO::DCPS::ReliableMulticast::detail::Packet
            > BufferType;

          size_t sender_history_size_;
          TAO::DCPS::ReliableMulticast::detail::Packet::id_type current_id_;
          BufferType buffer_;
        };

      } /* namespace detail */

    } /* namespace ReliableMulticast */

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SenderLogic.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_SENDERLOGIC_H */
