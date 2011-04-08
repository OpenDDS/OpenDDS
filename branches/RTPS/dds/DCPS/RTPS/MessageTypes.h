/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef _RTPS_MESSAGETYPES_H_
#define _RTPS_MESSAGETYPES_H_

#include "RtpsMessageTypesC.h"

namespace OpenDDS {
  namespace RTPS {

    enum SubmessageKind {
      PAD            = 0x01,    /* Pad */
      ACKNACK        = 0x06,    /* AckNak */
      HEARTBEAT      = 0x07,    /* Heartbeat */
      GAP            = 0x08,    /* Gap  */
      INFO_TS        = 0x09,    /* InfoTimestamp */
      INFO_SRC       = 0x0c,    /* InfoSource */
      INFO_REPLY_IP4 = 0x0d,    /* InfoReplyIp4 */
      INFO_DST       = 0x0e,    /* InfooDestination */
      INFO_REPLY     = 0x0f,    /* InfoReply */
      NACK_FRAG      = 0x12,    /* NackFrag */
      HEARTBEAT_FRAG = 0x13,    /* HeartbeatFrag */
      DATA           = 0x15,    /* Data */
      DATA_FRAG      = 0x16     /* DataGrag */
    };

  }
}

#endif /* _RTPS_MESSAGETYPES_H_ */
