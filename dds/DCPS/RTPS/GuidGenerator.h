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
