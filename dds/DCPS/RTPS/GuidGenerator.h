/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DdsDcpsGuidC.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/RTPS/rtps_export.h"

#include "ace/Basic_Types.h"
#include "ace/Thread_Mutex.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
  GuidGenerator();

  /// override the MAC address to use a specific network interface
  /// instead of just the first (non-loopback) interface
  int interfaceName(const char* interface);

  /// populate a GUID container with a unique ID. This will increment
  /// the counter, and use a lock (if compiled with MT ACE) while
  /// doing so.
  void populate(DCPS::GUID_t& container);

private:
  enum {NODE_ID_SIZE = 6};

  /// Borrowed from ACE::UUID_Node, definition of the
  /// MAC address holder type
  typedef unsigned char Node_ID[NODE_ID_SIZE];

  ACE_UINT16 getCount();

  Node_ID node_id_;
  pid_t pid_;
  ACE_Thread_Mutex counter_lock_;
  ACE_UINT16 counter_;
  OPENDDS_STRING interface_name_;
};

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
