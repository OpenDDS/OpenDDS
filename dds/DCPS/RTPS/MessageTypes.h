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

    // For messages we create, the "octetsToInlineQoS" value will be constant.
    const ACE_CDR::UShort DATA_OCTETS_TO_IQOS = 16;
    const ACE_CDR::UShort DATA_FRAG_OCTETS_TO_IQOS = 28;

    const ACE_CDR::UShort SMHDR_SZ = 4; // size of SubmessageHeader

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
