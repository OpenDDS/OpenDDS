/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RTPS_MESSAGETYPES_H
#define RTPS_MESSAGETYPES_H

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

    const PMDOctetArray PARTICIPANT_MESSAGE_DATA_KIND_UNKNOWN = { 0x00, 0x00, 0x00, 0x00 };
    const PMDOctetArray PARTICIPANT_MESSAGE_DATA_KIND_AUTOMATIC_LIVELINESS_UPDATE = { 0x00, 0x00, 0x00, 0x01 };
    const PMDOctetArray PARTICIPANT_MESSAGE_DATA_KIND_MANUAL_LIVELINESS_UPDATE = { 0x00, 0x00, 0x00, 0x02 };

  }
}

#endif /* RTPS_MESSAGETYPES_H */
