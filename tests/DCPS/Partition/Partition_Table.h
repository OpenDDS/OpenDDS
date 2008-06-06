// -*- C++ -*-
//
// $Id$

namespace Test
{
  struct PartitionConfig
  {
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
        { One,   1 },
        { Two,   0 },
        { Three, 2 },
        { Four,  1 },
        { Five,  0 }
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
        { One,   1 },
        { Two,   2 },
        { Three, 1 },
        { Four,  0 }
      };
  }
}
