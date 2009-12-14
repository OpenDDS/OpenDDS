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

#include "dcps_export.h"
#include "Definitions.h"

#include "ace/Basic_Types.h"

#include <climits>
#include <cstdlib>
#include <iterator>
#include <set>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export DisjointSequence {
public:
  typedef std::set<SequenceNumber> SequenceSet;

  class OpenDDS_Dcps_Export range_iterator
    : public std::iterator<std::input_iterator_tag, SequenceRange> {
  public:
    range_iterator();
    range_iterator(SequenceSet::iterator begin,
                   SequenceSet::iterator end);

    range_iterator& operator++();
    range_iterator  operator++(int);

    bool operator==(const range_iterator& rhs) const;
    bool operator!=(const range_iterator& rhs) const;

    SequenceRange&  operator*();
    SequenceRange*  operator->();

  private:
    SequenceSet::iterator pos_;
    SequenceSet::iterator end_;

    SequenceRange value_;
  };

  static const ACE_INT16 MAX_DEPTH;

  explicit DisjointSequence(SequenceNumber value = SequenceNumber());

  SequenceNumber low() const;
  SequenceNumber high() const;

  size_t depth() const;
  bool disjoint() const;

  range_iterator range_begin();
  range_iterator range_end();

  void shift(SequenceNumber value);
  void skip(SequenceNumber value);

  bool update(SequenceNumber value);
  bool update(const SequenceRange& range);

  operator SequenceNumber() const;

private:
  SequenceSet sequences_;

  void normalize();
};


} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#endif  /* DCPS_DISJOINTSEQUENCE_H */
