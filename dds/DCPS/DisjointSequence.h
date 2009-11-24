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

#include <iterator>
#include <set>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export DisjointSequence {
public:
  typedef std::pair<SequenceNumber, SequenceNumber> range_type;
  typedef std::set<SequenceNumber> set_type;

  class range_iterator
    : public std::iterator<std::input_iterator_tag, range_type> {
  public:
    explicit range_iterator(set_type::iterator pos);
    range_iterator(const range_iterator& it);

    range_iterator& operator++();
    range_iterator  operator++(int);

    bool operator==(const range_iterator& rhs);
    bool operator!=(const range_iterator& rhs);

    range_type& operator*();
    range_type* operator->(); 

  private:
    set_type::iterator pos_;
    range_type value_;
  };

  explicit DisjointSequence(SequenceNumber value = SequenceNumber());

  SequenceNumber low() const;
  SequenceNumber high() const;

  size_t depth() const;
  bool disjoint() const;

  range_iterator range_begin();
  range_iterator range_end();

  bool update(SequenceNumber value);
  void skip(SequenceNumber value);

  operator SequenceNumber() const;

private:
  set_type values_;

  void normalize();
};
  

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#endif  /* DCPS_DISJOINTSEQUENCE_H */
