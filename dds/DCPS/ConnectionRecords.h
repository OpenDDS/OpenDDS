/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CONNECTION_RECORDS_H
#define OPENDDS_DCPS_CONNECTION_RECORDS_H

#include "Definitions.h"

#if OPENDDS_CONFIG_BUILT_IN_TOPICS

#include "JobQueue.h"
#include "dcps_export.h"

#include "BuiltInTopicUtils.h"
#include <dds/OpenddsDcpsExtTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

typedef std::pair<bool, ConnectionRecord> ActionConnectionRecord;
typedef OPENDDS_VECTOR(ActionConnectionRecord) ConnectionRecords;

class OpenDDS_Dcps_Export WriteConnectionRecords : public DCPS::Job {
 public:
  WriteConnectionRecords(WeakRcHandle<BitSubscriber> bit_sub,
                         const ConnectionRecords& records)
    : bit_sub_(bit_sub)
    , records_(records)
  {}

  WriteConnectionRecords(WeakRcHandle<BitSubscriber> bit_sub,
                         bool action,
                         const ConnectionRecord& record)
    : bit_sub_(bit_sub)
  {
    records_.push_back(std::make_pair(action, record));
  }

  void execute();

 private:
  WeakRcHandle<BitSubscriber> bit_sub_;
  ConnectionRecords records_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

#endif  /* OPENDDS_DCPS_CONNECTION_RECORDS_H */
