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
      PAD               = 0x01, /* Pad */
      ACKNACK           = 0x06, /* AckNack */
      HEARTBEAT         = 0x07, /* Heartbeat */
      GAP               = 0x08, /* Gap */
      INFO_TS           = 0x09, /* InfoTimestamp */
      INFO_SRC          = 0x0c, /* InfoSource */
      INFO_REPLY_IP4    = 0x0d, /* InfoReplyIp4 */
      INFO_DST          = 0x0e, /* InfoDestination */
      INFO_REPLY        = 0x0f, /* InfoReply */
      NACK_FRAG         = 0x12, /* NackFrag */
      HEARTBEAT_FRAG    = 0x13, /* HeartbeatFrag */
      DATA              = 0x15, /* Data */
      DATA_FRAG         = 0x16, /* DataFrag */
      SMKIND_SPEC_MAX   = 0x7f, /* maximum that can be defined by spec */
      SMKIND_VENDOR_MIN = 0x80, /* minimum that can be implementation-defined */
      SMKIND_MAX        = 0xff  /* must fit in an IDL octet */
    };

    const OctetArray4
      PARTICIPANT_MESSAGE_DATA_KIND_UNKNOWN =
        { 0x00, 0x00, 0x00, 0x00 },
      PARTICIPANT_MESSAGE_DATA_KIND_AUTOMATIC_LIVELINESS_UPDATE =
        { 0x00, 0x00, 0x00, 0x01 },
      PARTICIPANT_MESSAGE_DATA_KIND_MANUAL_LIVELINESS_UPDATE =
        { 0x00, 0x00, 0x00, 0x02 };

  }
}

#endif /* RTPS_MESSAGETYPES_H */
