// -*- C++ -*-
//

#ifndef TAO_DCPS_NACKGENERATOR_H
#define TAO_DCPS_NACKGENERATOR_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include "Packet.h"
#include <set>
#include <vector>

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        class ReliableMulticast_Export NackGenerator
        {
        public:
          typedef std::set<
            TAO::DCPS::ReliableMulticast::detail::Packet
            > PacketSet;

          bool cancel(
            TAO::DCPS::ReliableMulticast::detail::Packet::id_type id
            );

          void cancel_all();

          void nack_range(
            TAO::DCPS::ReliableMulticast::detail::Packet::id_type begin,
            TAO::DCPS::ReliableMulticast::detail::Packet::id_type end
            );

          void get_nacks(
            std::vector<TAO::DCPS::ReliableMulticast::detail::Packet>& nacks
            );

        private:
          PacketSet::iterator find_nack_containing(
            const TAO::DCPS::ReliableMulticast::detail::Packet& packet
            );

          PacketSet::iterator join_nacks(
            PacketSet::iterator first,
            PacketSet::iterator second
            );

          PacketSet nacks_;
        };

      } /* namespace detail */

    } /* namespace ReliableMulticast */

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "NackGenerator.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_NACKGENERATOR_H */
