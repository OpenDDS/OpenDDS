/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATA_COLLECTOR_H
#define DATA_COLLECTOR_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/SafetyProfileStreams.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class DataCollector<DatumType>
 *
 * @brief Collect data in a buffer.
 *
 * This class implements a rudimentary data collection mechanism.  It is
 * extendable to allow data to be collected as a feature of a more
 * complex class.
 */
template<typename DatumType>
class DataCollector {
public:
  /// Selectors for behavior when buffer fills.
  enum OnFull { KeepOldest, KeepNewest, Unbounded};

  /**
   * Construct with optional buffer size and full behavior.
   *
   * @param bound  - amount of data to store or reserve as buffer.
   * @param onFull - behavior of collector when bound is reached.
   *
   * OnFull == KeepOldest: The buffer is limited to the amount of data
   *                       specified by the bound parameter and only
   *                       the data collected first is retained.
   * OnFull == KeepNewest: The buffer is limited to the amount of data
   *                       specified by the bound parameter and only
   *                       the most recently collected data is
   *                       retained.
   * OnFull == Unbounded:  The buffer contains all collected data and
   *                       the bound parameter is used as an initial
   *                       reservation amount.
   *
   * Collection is either bounded or unbounded.  In the case of a
   * bounded collection, either the first data collected or the most
   * recent data collected is retained.  When an unbounded collection
   * is specified, then the bound parameter is used as a capacity hint
   * and that amount of data is reserved initially.
   */
  DataCollector(unsigned int bound = 0, OnFull onFull = KeepOldest);

  /// Allow the class to be extended.
  virtual ~DataCollector();

  /// Implement data collection.
  void collect(const DatumType& datum);

  /// Amount of data actually stored.
  unsigned int size() const;

#ifndef OPENDDS_SAFETY_PROFILE
  /// Convenience operator for collecting data by inserting it into the
  /// collector.
  DataCollector<DatumType>& operator<<(DatumType datum);

  /// Implement insertion of collected data onto an ostream.
  std::ostream& insert(std::ostream& str) const;
#endif //OPENDDS_SAFETY_PROFILE

private:
  /// The collected data goes here.
  OPENDDS_VECTOR(DatumType) buffer_;

  /// Where to write the next datum collected.
  unsigned int writeAt_;

  /// Total or initial capacity of buffer.
  unsigned int bound_;

  /// Flag indicating that we have collected as much or more data than
  /// we can store.
  bool full_;

  /// Selector for behavior when buffer fills.
  OnFull onFull_;
};

#ifndef OPENDDS_SAFETY_PROFILE
/// Insert collected data onto an ostream.
template<typename DatumType>
std::ostream& operator<<(
  std::ostream& str,
  const DataCollector<DatumType>& value);
#endif //OPENDDS_SAFETY_PROFILE

} // namespace DCPS
} // namespace OpenDDS


OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "DataCollector_T.inl"
#endif /* __ACE_INLINE__ */

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "DataCollector_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("DataCollector_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#include /**/ "ace/post.h"

#endif /* DATA_COLLECTOR_H */
