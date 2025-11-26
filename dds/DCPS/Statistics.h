/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_STATISTICS_H
#define OPENDDS_DCPS_STATISTICS_H

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

// The Statistics struct is defined in dds/OpenddsDcpsExt.idl

template <>
struct InternalTraits<Statistics> {
  struct KeyCompare {
    bool operator()(const Statistics& lhs, const Statistics& rhs) const
    {
      using ::operator<; // TAO::String_Manager's operator< is in the global namespace
      return lhs.id < rhs.id;
    }
  };
};

typedef InternalDataWriter<Statistics> StatisticsDataWriter;
typedef RcHandle<StatisticsDataWriter> StatisticsDataWriter_rch;

typedef InternalDataReader<Statistics> StatisticsDataReader;
typedef RcHandle<StatisticsDataReader> StatisticsDataReader_rch;

typedef InternalTopic<Statistics> StatisticsTopic;
typedef RcHandle<StatisticsTopic> StatisticsTopic_rch;

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
