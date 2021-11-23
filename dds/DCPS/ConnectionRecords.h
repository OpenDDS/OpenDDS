/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CONNECTION_RECORDS_H
#define OPENDDS_DCPS_CONNECTION_RECORDS_H

#ifndef DDS_HAS_MINIMUM_BIT

#include "JobQueue.h"
#include "dcps_export.h"

#include <dds/OpenddsDcpsExtTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

typedef std::pair<bool, ConnectionRecord> ActionConnectionRecord;
typedef OPENDDS_VECTOR(ActionConnectionRecord) ConnectionRecords;

class OpenDDS_Dcps_Export WriteConnectionRecords : public DCPS::JobQueue::Job {
 public:
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

 private:
  DDS::Subscriber_var bit_sub_;
  ConnectionRecords records_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DDS_HAS_MINIMUM_BIT */

#endif  /* OPENDDS_DCPS_CONNECTION_RECORDS_H */
