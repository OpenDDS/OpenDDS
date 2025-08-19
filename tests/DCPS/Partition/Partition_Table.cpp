#include "Partition_Table.h"

// -*- C++ -*-
//

namespace Test
{
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
        PartitionConfig (One,   3, "PubOne"),
        PartitionConfig (Two,   1, "PubTwo"),
        PartitionConfig (Three, 3, "PubThree"),
        PartitionConfig (Four,  1, "PubFour"),
        PartitionConfig (Five,  0, "PubFive"),
        PartitionConfig (Six,   3, "PubSix")
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
        PartitionConfig (One,   2, "SubOne"),
        PartitionConfig (Two,   2, "SubTwo"),
        PartitionConfig (Three, 1, "SubThree"),
        PartitionConfig (Four,  0, "SubFour"),
        PartitionConfig (Five,  2, "SubFive"),
        PartitionConfig (Six,   4, "SubSix")
      };
  }
}
