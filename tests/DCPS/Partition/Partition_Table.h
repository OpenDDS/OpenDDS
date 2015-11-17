// -*- C++ -*-
//

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
        "Wun?Two[0-9][!a-z]Tree",  // A wildcard
        0
      };

    /// Partition Five
    char const * Five[] =
      {
        "Not*A Match",
        0
      };

    /// Partition Six (equivalent to One)
    char const * Six[] = { 0 };

    PartitionConfig const PartitionConfigs[] =
      {
        PartitionConfig (One,   3),
        PartitionConfig (Two,   1),
        PartitionConfig (Three, 3),
        PartitionConfig (Four,  1),
        PartitionConfig (Five,  0),
        PartitionConfig (Six,   3)
      };
  }

  namespace Requested
  {
    /// Partition One
    char const * One[] = { 0 };

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

    /// Partition Five (equivalent to One)
    char const * Five[] =
      {
        "",
        0
      };

    /// Partition Six
    char const * Six[] =
      {
        "*",
        0
      };

    PartitionConfig const PartitionConfigs[] =
      {
        PartitionConfig (One,   2),
        PartitionConfig (Two,   2),
        PartitionConfig (Three, 1),
        PartitionConfig (Four,  0),
        PartitionConfig (Five,  2),
        PartitionConfig (Six,   4)
      };
  }
}
