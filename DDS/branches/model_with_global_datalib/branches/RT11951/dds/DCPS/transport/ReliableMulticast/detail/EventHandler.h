// -*- C++ -*-
//

#ifndef OPENDDS_DCPS_EVENTHANDLER_H
#define OPENDDS_DCPS_EVENTHANDLER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include "ace/Reactor.h"
#include "ace/Event_Handler.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include <queue>
#include <string>
#include <utility>

namespace OpenDDS
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        class ReliableMulticast_Export EventHandler
          : public ACE_Event_Handler
        {
        public:
          virtual ~EventHandler();

          virtual void close();

          virtual void send(
            char* buffer,
            size_t size,
            const ACE_INET_Addr& dest
            );

          virtual void receive(
            const char* buffer,
            size_t size,
            const ACE_INET_Addr& peer
            ) = 0;

          virtual ACE_HANDLE get_handle() const;

          virtual int handle_input(
            ACE_HANDLE fd = ACE_INVALID_HANDLE
            );

          virtual int handle_output(
            ACE_HANDLE fd = ACE_INVALID_HANDLE
            );

          virtual int handle_close(
            ACE_HANDLE fd,
            ACE_Reactor_Mask mask
            );

        protected:
          typedef std::queue<
            std::pair<std::string, ACE_INET_Addr>
            > Queue;

          ACE_SOCK_Dgram_Mcast socket_;
          ACE_Thread_Mutex input_mutex_;
          ACE_Thread_Mutex output_mutex_;
          Queue output_queue_;
        };

      } /* namespace detail */

    } /* namespace ReliableMulticast */

  } /* namespace DCPS */

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "EventHandler.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_EVENTHANDLER_H */
