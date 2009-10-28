/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_DISJOINTSEQUENCE_H
#define DCPS_DISJOINTSEQUENCE_H

#include <set>

#include "Definitions.h"

#include "dcps_export.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export DisjointSequence {
public:
  DisjointSequence();
  ~DisjointSequence();

  SequenceNumber low();
  SequenceNumber high();

  bool disjoint();

  bool update(SequenceNumber value);
  void skip(SequenceNumber value);

  operator SequenceNumber();

private:
  SequenceNumber low_;
  SequenceNumber high_;

  typedef std::set<SequenceNumber> values_type;
  values_type values_;

  void normalize();
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#endif  /* DCPS_DISJOINTSEQUENCE_H */
