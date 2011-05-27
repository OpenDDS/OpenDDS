// -*- C++ -*-
//

#ifndef TAO_DCPS_PACKETRECEIVERCALLBACK_H
#define TAO_DCPS_PACKETRECEIVERCALLBACK_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "Packet.h"
#include <vector>

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        /// Specifies an interface only!
        class ReliableMulticast_Export PacketReceiverCallback
        {
        public:
          virtual ~PacketReceiverCallback() {}

          virtual void received_packets(
            const std::vector<Packet>& packets
            ) = 0;

          virtual void reliability_compromised() = 0;
        };

      } /* namespace detail */

    } /* namespace ReliableMulticast */

  } /* namespace DCPS */

} /* namespace TAO */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_PACKETRECEIVERCALLBACK_H */
