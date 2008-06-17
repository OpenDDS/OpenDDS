// -*- C++ -*-
//
// $Id$

namespace Test
{
  struct PartitionConfig
  {
    PartitionConfig (char const * const * p,
                     short m)
      : partitions (p)
      , expected_matches (m)
    {
    }

    PartitionConfig (PartitionConfig const & rhs)
      : partitions (rhs.partitions)
      , expected_matches (rhs.expected_matches)
    {
    }

    char const * const * const partitions;
    short const expected_matches;
    
  };

  namespace Offered
  {
    /// Partition One
    char const * One[] =
      {
        "",
        0
      };

    /// Partition Two
    char const * Two[] =
      {
        "Loner",
        0
      };

    /// Partition Three
    char const * Three[] = 
      {
        "Amadeus",
        "Ziggie*dust",   // A wildcard
        0
      };

    /// Partition Four
    char const * Four[] =
      {
        "Wun?Two*Tree",  // A wildcard
        0
      };

    /// Partition Five
    char const * Five[] =
      {
        "Not*A Match",
        0
      };

    PartitionConfig const PartitionConfigs[] =
      {
        PartitionConfig (One,   1),
        PartitionConfig (Two,   0),
        PartitionConfig (Three, 2),
        PartitionConfig (Four,  1),
        PartitionConfig (Five,  0)
      };
  }

  namespace Requested
  {
    /// Partition One
    char const * const * const One = 0;

    /// Partition Two
    char const * Two[] =
      {
        "Amadeus",
        "Wun1Two23Tree",
        0
      };

    /// Partition Three
    char const * Three[] = 
      {
        "ZiggieStardust",
        0
      };

    /// Partition Four
    char const * Four[] =
      {
        "Not?A*Match",  // A wildcard
        0
      };

    PartitionConfig const PartitionConfigs[] =
      {
        PartitionConfig (One,   1),
        PartitionConfig (Two,   2),
        PartitionConfig (Three, 1),
        PartitionConfig (Four,  0)
      };
  }
}
