// -*- C++ -*-
/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DdsDcpsGuidC.h"
#include "dds/DCPS/RTPS/rtps_export.h"

namespace OpenDDS {
  namespace RTPS {

/**
 *   Generate GuidPrefix_t values for use with RTPS
 *   Also see GuidConverter.h in dds/DCPS
 *   0  GUID_t.guidPrefix[ 0] == VendorId_t == 0x01 for OCI (used for OpenDDS)
 *   1  GUID_t.guidPrefix[ 1] == VendorId_t == 0x03 for OCI (used for OpenDDS)
 *   2  GUID_t.guidPrefix[ 2] == MAC Address
 *   3  GUID_t.guidPrefix[ 3] == MAC Address
 *   4  GUID_t.guidPrefix[ 4] == MAC Address
 *   5  GUID_t.guidPrefix[ 5] == MAC Address
 *   6  GUID_t.guidPrefix[ 6] == MAC Address
 *   7  GUID_t.guidPrefix[ 7] == MAC Address
 *   8  GUID_t.guidPrefix[ 8] == Process ID (MS byte)
 *   9  GUID_t.guidPrefix[ 9] == Process ID (LS byte)
 *  10  GUID_t.guidPrefix[10] == Counter (MS byte)
 *  11  GUID_t.guidPrefix[11] == Counter (LS byte)
 *
 */
class OpenDDS_Rtps_Export GuidGenerator {
 public:
  /// Borrowed from ACE::UUID_Node, definition of the
  /// MAC address holder type

  enum {NODE_ID_SIZE = 6};
  typedef u_char Node_ID[NODE_ID_SIZE];

  /// Default constructor - initializes pid and MAC address values
  GuidGenerator (void);

  /// populate a GUID container with a unique ID. This will increment
  /// the counter, and use a lock (if compiled with MT ACE) while
  /// doing so.
  void populate (DCPS::GUID_t &container);

 private:
  uint16_t getCount(void);

  Node_ID node_id_;
  pid_t pid_;
  ACE_SYNCH_MUTEX counter_lock_;
  uint16_t counter_;
};

} // namespace RTPS
} // namespace OpenDDS
