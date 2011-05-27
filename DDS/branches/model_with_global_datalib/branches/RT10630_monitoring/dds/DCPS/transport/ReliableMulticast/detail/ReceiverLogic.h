// -*- C++ -*-
//

#ifndef TAO_DCPS_RECEIVERLOGIC_H
#define TAO_DCPS_RECEIVERLOGIC_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include "Packet.h"
#include "NackGenerator.h"
#include <map>
#include <stdexcept>
#include <vector>

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        class ReliableMulticast_Export ReceiverLogic
        {
        public:
          enum ReliabilityMode
          {
            HARD_RELIABILITY,
            SOFT_RELIABILITY
          };

          typedef std::vector<
            TAO::DCPS::ReliableMulticast::detail::Packet
            > PacketVector;

          ReceiverLogic(
            size_t receiver_buffer_size,
            const ReliabilityMode& reliability = HARD_RELIABILITY
            );

          void receive(
            const TAO::DCPS::ReliableMulticast::detail::Packet& p,
            PacketVector& nacks,
            PacketVector& delivered
            );

        private:
          bool in_range(
            const TAO::DCPS::ReliableMulticast::detail::Packet::id_type& id,
            int minadd,
            int maxadd
            );

          bool get_and_remove_buffered_packet(
            const TAO::DCPS::ReliableMulticast::detail::Packet::id_type& id,
            TAO::DCPS::ReliableMulticast::detail::Packet& p
            );

          void deliver(
            PacketVector& delivered,
            const TAO::DCPS::ReliableMulticast::detail::Packet& p
            );

          void buffer_packet(
            const TAO::DCPS::ReliableMulticast::detail::Packet& p,
            PacketVector& delivered
            );

          bool is_buffered(
            const TAO::DCPS::ReliableMulticast::detail::Packet& p
            ) const;

          TAO::DCPS::ReliableMulticast::detail::Packet::id_type find_previous_received(
            const TAO::DCPS::ReliableMulticast::detail::Packet::id_type& id
            ) const;

          size_t buffersize(
            ) const;

          TAO::DCPS::ReliableMulticast::detail::Packet::id_type find_beginning_of_consecutive_range(
            const TAO::DCPS::ReliableMulticast::detail::Packet::id_type& end
            ) const;

          void handle_unreliable_operation(
            PacketVector& delivered
            );

          typedef std::map<
            TAO::DCPS::ReliableMulticast::detail::Packet::id_type,
            TAO::DCPS::ReliableMulticast::detail::Packet
            > BufferType;

          size_t receiver_buffer_size_;
          ReliabilityMode reliability_;
          bool seen_last_delivered_;
          TAO::DCPS::ReliableMulticast::detail::Packet::id_type last_delivered_id_;
          TAO::DCPS::ReliableMulticast::detail::NackGenerator nacker_;
          BufferType buffer_;
        };

      } /* namespace detail */

    } /* namespace ReliableMulticast */

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReceiverLogic.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RECEIVERLOGIC_H */
