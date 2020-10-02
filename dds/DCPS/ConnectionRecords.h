/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CONNECTION_RECORDS_H
#define OPENDDS_DCPS_CONNECTION_RECORDS_H

#include "dds/DCPS/JobQueue.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

typedef std::pair<bool, ConnectionRecord> ActionConnectionRecord;
typedef OPENDDS_VECTOR(ActionConnectionRecord) ConnectionRecords;

struct WriteConnectionRecords : public DCPS::JobQueue::Job {
  WriteConnectionRecords(DDS::Subscriber_var bit_sub,
                         const ConnectionRecords& records)
    : bit_sub_(bit_sub)
    , records_(records)
  {}

  WriteConnectionRecords(DDS::Subscriber_var bit_sub,
                         bool action,
                         const ConnectionRecord& record)
    : bit_sub_(bit_sub)
  {
    records_.push_back(std::make_pair(action, record));
  }

  void execute();

  DDS::Subscriber_var bit_sub_;
  ConnectionRecords records_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_CONNECTION_RECORDS_H */
