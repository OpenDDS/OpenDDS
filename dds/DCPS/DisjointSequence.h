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

  friend class const_iterator
    : public std::iterator<std::input_iterator_tag, range_type> {
  public:
    const_iterator(const set_type& set, set_type::const_iterator pos);
    const_iterator(const const_iterator& it);

    const_iterator& operator++();
    const_iterator& operator++(int);

    bool operator==(const const_iterator& rhs);
    bool operator!=(const const_iterator& rhs);

    range_type operator*();

  private:
    const set_type& values_;
    set_type::const_iterator pos_;
  };
  typedef const_iterator iterator;

  explicit DisjointSequence(SequenceNumber value = SequenceNumber());

  SequenceNumber low() const;
  SequenceNumber high() const;

  size_t depth() const;
  bool disjoint() const;

  iterator begin();
  const_iterator begin() const;

  iterator end();
  const_iterator end() const;

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
