/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef _RTPS_BASEMESSAGETYPES_H_
#define _RTPS_BASEMESSAGETYPES_H_

#include "RtpsBaseMessageTypesC.h"

namespace OpenDDS {
  namespace RTPS {

    const Time_t TIME_ZERO     = { 0, 0 };
    const Time_t TIME_INVALID  = { -1, 0xffffffff };
    const Time_t TIME_INFINITE = { 0x7fffffff, 0xffffffff };

    const VendorId_t VENDORID_UNKNOWN = { { 0 } };

    const SequenceNumber_t SEQUENCENUMBER_UNKNOWN = { -1, 0 };

    const Locator_address_t LOCATOR_ADDRESS_INVALID = { 0 };
    const Locator_t LOCATOR_INVALID = { -1, 0, { 0 } };

    const LocatorUDPv4_t LOCATORUDPv4_INVALID = {0, 0};

    const ProtocolVersion_t PROTOCOLVERSION_1_0 = { 1, 0 };
    const ProtocolVersion_t PROTOCOLVERSION_1_1 = { 1, 1 };
    const ProtocolVersion_t PROTOCOLVERSION_2_0 = { 2, 0 };
    const ProtocolVersion_t PROTOCOLVERSION_2_1 = { 2, 1 };
    const ProtocolVersion_t PROTOCOLVERSION = PROTOCOLVERSION_2_1;

    const ParameterId_t PID_PAD = { 0 };
    const ParameterId_t PID_SENTINEL = { 1 };

  }
}

#endif /* _RTPS_BASEMESSAGETYPES_H_ */
