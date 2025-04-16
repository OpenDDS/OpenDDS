#ifndef PARTITION_TABLE_H
#define PARTITION_TABLE_H

// -*- C++ -*-
//

#include "dds/DCPS/PoolAllocator.h"
#include "Partition_export.h"

namespace Test
{
  struct PartitionConfig
  {
    PartitionConfig (char const * const * p,
                     short m,
                     const OpenDDS::DCPS::String& a)
      : partitions (p)
      , expected_matches (m)
      , actor (a)
    {
    }

    PartitionConfig (PartitionConfig const & rhs)
      : partitions (rhs.partitions)
      , expected_matches (rhs.expected_matches)
      , actor (rhs.actor)
    {
    }

    char const * const * const partitions;
    short const expected_matches;
    const OpenDDS::DCPS::String actor;

  };

  namespace Offered
  {
    Partition_Export extern PartitionConfig const PartitionConfigs[6];
  }

  namespace Requested
  {
    Partition_Export extern PartitionConfig const PartitionConfigs[6];
  }
}

#endif
