/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef COMPARATOR_H
#define COMPARATOR_H

#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/OS_NS_string.h"

#include "RcHandle_T.h"
#include "RcObject_T.h"
#include "RakeData.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ComparatorBase : public RcObject<ACE_SYNCH_MUTEX> {
public:
  typedef RcHandle<ComparatorBase> Ptr;

  ComparatorBase(){}
  explicit ComparatorBase(Ptr next) : next_(next) {}

  virtual ~ComparatorBase() {}

  virtual bool less(void* lhs, void* rhs) const = 0;
  virtual bool equal(void* lhs, void* rhs) const = 0;

  bool compare(void* lhs, void* rhs) const {
    if (next_.in() && equal(lhs, rhs)) return next_->compare(lhs, rhs);

    return less(lhs, rhs);
  }

private:
  Ptr next_;
};

template <class Sample, class Field>
class FieldComparator : public ComparatorBase {
public:
  typedef Field Sample::* MemberPtr;
  FieldComparator(MemberPtr mp, ComparatorBase::Ptr next)
  : ComparatorBase(next)
  , mp_(mp) {}

  bool less(void* lhs_void, void* rhs_void) const {
    Sample* lhs = static_cast<Sample*>(lhs_void);
    Sample* rhs = static_cast<Sample*>(rhs_void);
    using ::operator<; // TAO::String_Manager's operator< is in global NS
    return lhs->*mp_ < rhs->*mp_;
  }

  bool equal(void* lhs_void, void* rhs_void) const {
    Sample* lhs = static_cast<Sample*>(lhs_void);
    Sample* rhs = static_cast<Sample*>(rhs_void);
    const Field& field_l = lhs->*mp_;
    const Field& field_r = rhs->*mp_;
    using ::operator<; // TAO::String_Manager's operator< is in global NS
    return !(field_l < field_r) && !(field_r < field_l);
  }

private:
  MemberPtr mp_;
};

template <class Sample, class Field>
ComparatorBase::Ptr make_field_cmp(Field Sample::* mp,
                                   ComparatorBase::Ptr next)
{
  return make_rch<FieldComparator<Sample, Field> >(mp, next);
}

/** deal with nested structs, for example:
 *  struct A { long x; };
 *  struct B { A theA; };
 *  B's query string has "ORDER BY theA.x"
 *  The generated code will create a StructComparator with
 *  Sample = B and Field = A which in turn has a "delegate" which is
 *  a FieldComparator (see above) with Sample = A and Field = CORBA::Long
 */
template <class Sample, class Field>
class StructComparator : public ComparatorBase {
public:
  typedef Field Sample::* MemberPtr;
  StructComparator(MemberPtr mp, ComparatorBase::Ptr delegate,
                   ComparatorBase::Ptr next)
  : ComparatorBase(next)
  , mp_(mp)
  , delegate_(delegate) {}

  bool less(void* lhs_void, void* rhs_void) const {
    Sample* lhs = static_cast<Sample*>(lhs_void);
    Sample* rhs = static_cast<Sample*>(rhs_void);
    return delegate_->less(&(lhs->*mp_), &(rhs->*mp_));
  }

  bool equal(void* lhs_void, void* rhs_void) const {
    Sample* lhs = static_cast<Sample*>(lhs_void);
    Sample* rhs = static_cast<Sample*>(rhs_void);
    return delegate_->equal(&(lhs->*mp_), &(rhs->*mp_));
  }

private:
  MemberPtr mp_;
  ComparatorBase::Ptr delegate_;
};

template <class Sample, class Field>
ComparatorBase::Ptr make_struct_cmp(Field Sample::* mp,
                                    ComparatorBase::Ptr delegate,
                                    ComparatorBase::Ptr next)
{
  return make_rch<StructComparator<Sample, Field> >(mp, delegate, next);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
