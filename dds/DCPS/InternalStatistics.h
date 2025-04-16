/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INTERNAL_STATISTICS_H
#define OPENDDS_DCPS_INTERNAL_STATISTICS_H

#include "dcps_export.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "InternalDataReader.h"
#include "InternalDataWriter.h"
#include "InternalTopic.h"

#include <dds/OpenddsDcpsExtC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// The InternalStatistics struct is defined in dds/OpenddsDcpsExt.idl

template <>
struct InternalTraits<InternalStatistics> {
  struct KeyCompare {
    bool operator()(const InternalStatistics& lhs, const InternalStatistics& rhs) const
    {
      using ::operator<; // TAO::String_Manager's operator< is in the global namespace
      return lhs.id < rhs.id;
    }
  };
};

typedef InternalDataWriter<InternalStatistics> InternalStatisticsDataWriter;
typedef RcHandle<InternalStatisticsDataWriter> InternalStatisticsDataWriter_rch;

typedef InternalDataReader<InternalStatistics> InternalStatisticsDataReader;
typedef RcHandle<InternalStatisticsDataReader> InternalStatisticsDataReader_rch;

typedef InternalTopic<InternalStatistics> InternalStatisticsTopic;
typedef RcHandle<InternalStatisticsTopic> InternalStatisticsTopic_rch;

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
