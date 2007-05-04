// -*- C++ -*-
//

#ifndef TAO_DCPS_REACTIVEPACKETRECEIVER_H
#define TAO_DCPS_REACTIVEPACKETRECEIVER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "PacketHandler.h"
#include "Packet.h"
#include "ReceiverLogic.h"
#include "ace/Bound_Ptr.h"
#include "ace/Null_Mutex.h"
#include <map>

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        class PacketReceiverCallback;

        class ReliableMulticast_Export ReactivePacketReceiver
          : public PacketHandler
        {
        public:
          ReactivePacketReceiver(
            const ACE_INET_Addr& multicast_group_address,
            PacketReceiverCallback& callback,
            size_t receiver_buffer_size
            );

          virtual ~ReactivePacketReceiver();

          bool open();

          virtual void close();

          virtual void receive_packet_from(
            const Packet& packet,
            const ACE_INET_Addr& peer
            );

          int handle_timeout(
            const ACE_Time_Value& current_time,
            const void* = 0
            );

        private:
          typedef ACE_Strong_Bound_Ptr<ReceiverLogic, ACE_Null_Mutex> ReceiverLogicPtr;

          PacketReceiverCallback& callback_;
          ACE_INET_Addr multicast_group_address_;
          size_t receiver_buffer_size_;
          ACE_Thread_Mutex nack_mutex_;
          std::map<ACE_INET_Addr, ReceiverLogicPtr> receiver_logics_;
          typedef std::map<ACE_INET_Addr, std::vector<Packet> > PeerToPacketVectorMap;
          PeerToPacketVectorMap nacks_;
        };

      } /* namespace detail */

    } /* namespace ReliableMulticast */

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReactivePacketReceiver.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_REACTIVEPACKETRECEIVER_H */
