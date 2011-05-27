// -*- C++ -*-
//

#ifndef TAO_DCPS_REACTIVEPACKETSENDER_H
#define TAO_DCPS_REACTIVEPACKETSENDER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "Packet.h"
#include "PacketHandler.h"
#include "SenderLogic.h"

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        class ReliableMulticast_Export ReactivePacketSender
          : public PacketHandler
        {
        public:
          ReactivePacketSender(
            const ACE_INET_Addr& local_address,
            const ACE_INET_Addr& multicast_group_address,
            size_t sender_history_size
            );
          virtual ~ReactivePacketSender();

          bool open();

          virtual void close();

          virtual void send(const Packet& p);

          virtual void receive(
            const Packet& packet,
            const ACE_INET_Addr& peer
            );

          int handle_timeout(
            const ACE_Time_Value& current_time,
            const void* = 0
            );

        private:
          ACE_Thread_Mutex heartbeat_mutex_;
          SenderLogic sender_logic_;
          ACE_INET_Addr local_address_;
          ACE_INET_Addr multicast_group_address_;
        };

      } /* namespace detail */

    } /* namespace ReliableMulticast */

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReactivePacketSender.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_REACTIVEPACKETSENDER_H */
