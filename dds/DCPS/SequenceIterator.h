/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SEQUENCEITERATOR_H
#define OPENDDS_DCPS_SEQUENCEITERATOR_H

#include "Util.h"
#include "dds/Versioned_Namespace.h"

#include "tao/Unbounded_Value_Sequence_T.h"
#include <iterator>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename Sequence>
class SequenceBackInsertIterator {
public:

  typedef std::output_iterator_tag iterator_category;
  typedef void value_type;
  typedef void difference_type;
  typedef void pointer;
  typedef void reference;

  explicit SequenceBackInsertIterator(Sequence& seq) : seq_(&seq) {}

  SequenceBackInsertIterator& operator=(const typename Sequence::value_type& value)
  {
    push_back(*seq_, value);
    return *this;
  }

  SequenceBackInsertIterator& operator*() { return *this; }

  SequenceBackInsertIterator& operator++() { return *this; }

  SequenceBackInsertIterator operator++(int) { return *this; }

private:
  Sequence* seq_;
};

template <typename Sequence>
inline SequenceBackInsertIterator<Sequence> back_inserter(Sequence& seq)
{
  return SequenceBackInsertIterator<Sequence>(seq);
}

template <typename Sequence_>
struct SequenceIterTraits
{
  typedef Sequence_ Sequence;
  typedef typename Sequence::value_type value_type;
  typedef typename Sequence::value_type* pointer;
  typedef typename Sequence::value_type& reference;
};

template <typename Sequence_>
struct ConstSequenceIterTraits
{
  typedef const Sequence_ Sequence;
  typedef const typename Sequence::value_type value_type;
  typedef const typename Sequence::value_type* pointer;
  typedef const typename Sequence::value_type& reference;
};

template <typename Derived, typename IterTraits>
struct SequenceIteratorBase
{
  typedef std::random_access_iterator_tag iterator_category;
  typedef int difference_type;
  typedef typename IterTraits::value_type value_type;
  typedef typename IterTraits::pointer pointer;
  typedef typename IterTraits::reference reference;

  Derived& as_derived() { return *static_cast<Derived*>(this); }

  const Derived& as_derived() const { return *static_cast<const Derived*>(this); }

  SequenceIteratorBase() : seq_(), current_(0) {}

  explicit SequenceIteratorBase(typename IterTraits::Sequence& sequence) : seq_(&sequence), current_(0) {}

  operator difference_type() const { return current_; }

  // Forward iterator requirements

  reference operator*() const { return (*seq_)[current_]; }

  pointer operator->() const { return &(*seq_)[current_]; }

  Derived& operator++()
  {
    ++current_;
    return as_derived();
  }

  Derived operator++(int)
  {
    Derived iter(as_derived());
    ++current_;
    return iter;
  }

  bool operator==(const Derived& rhs) const
  {
    return (seq_ == rhs.seq_) && (current_ == rhs.current_);
  }

  bool operator!=(const Derived& rhs) const { return !(*this == rhs); }

  // Bidirectional iterator requirements

  Derived& operator--()
  {
    --current_;
    return as_derived();
  }

  Derived operator--(int)
  {
    Derived iter(as_derived());
    --current_;
    return iter;
  }

  // Random-access iterator requirements

  reference operator[](difference_type n) const { return (*seq_)[n]; }

  Derived& operator+=(difference_type n)
  {
    current_ += n;
    return as_derived();
  }

  Derived operator+(difference_type n) const
  {
    Derived iter(as_derived());
    iter.current_ += n;
    return iter;
  }

  Derived& operator-=(difference_type n)
  {
    current_ -= n;
    return as_derived();
  }

  Derived operator-(difference_type n) const
  {
    Derived iter(as_derived());
    iter.current_ -= n;
    return iter;
  }

  Derived& operator+=(const Derived& rhs)
  {
    current_ += rhs.current_;
    return *this;
  }

  Derived operator+(const Derived& rhs) const
  {
    Derived iter(*this);
    iter.current_ += rhs.current_;
    return iter;
  }

  Derived& operator-=(const Derived& rhs)
  {
    current_ -= rhs.current_;
    return as_derived();
  }

  Derived operator-(const Derived& rhs) const
  {
    Derived iter(as_derived());
    iter.current_ -= rhs.current_;
    return iter;
  }

  bool operator<(const Derived& rhs) const
  {
    return current_ < rhs.current_;
  }

  bool operator>(const Derived& rhs) const
  {
    return current_ > rhs.current_;
  }

  bool operator<=(const Derived& rhs) const
  {
    return current_ <= rhs.current_;
  }

  bool operator>=(const Derived& rhs) const
  {
    return current_ >= rhs.current_;
  }

  static Derived begin(typename IterTraits::Sequence& sequence)
  {
    Derived iter(sequence);
    iter.current_ = 0;
    return iter;
  }

  static Derived end(typename IterTraits::Sequence& sequence)
  {
    Derived iter(sequence);
    iter.current_ = sequence.length();
    return iter;
  }

protected:
  typename IterTraits::Sequence* seq_;
  difference_type current_;
};

template <typename Sequence>
struct SequenceIterator : public SequenceIteratorBase<SequenceIterator<Sequence>,
                                                      SequenceIterTraits<Sequence> >
{
  typedef SequenceIteratorBase<SequenceIterator<Sequence>,
                               SequenceIterTraits<Sequence> > Base;

  SequenceIterator() : Base() {}
  explicit SequenceIterator(typename SequenceIterTraits<Sequence>::Sequence& seq) : Base(seq) {}
};

template <typename Sequence>
struct ConstSequenceIterator : public SequenceIteratorBase<ConstSequenceIterator<Sequence>,
                                                           ConstSequenceIterTraits<Sequence> >
{
  typedef SequenceIteratorBase<ConstSequenceIterator<Sequence>,
                               ConstSequenceIterTraits<Sequence> > Base;

  ConstSequenceIterator() : Base() {}
  explicit ConstSequenceIterator(typename ConstSequenceIterTraits<Sequence>::Sequence& seq) : Base(seq) {}
};

template <typename Sequence>
inline SequenceIterator<Sequence> sequence_begin(Sequence& seq)
{
  return SequenceIterator<Sequence>::Base::begin(seq);
}

template <typename Sequence>
inline SequenceIterator<Sequence> sequence_end(Sequence& seq)
{
  return SequenceIterator<Sequence>::Base::end(seq);
}

template <typename Sequence>
inline ConstSequenceIterator<Sequence> const_sequence_begin(Sequence& seq)
{
  return ConstSequenceIterator<Sequence>::Base::begin(seq);
}

template <typename Sequence>
inline ConstSequenceIterator<Sequence> const_sequence_end(Sequence& seq)
{
  return ConstSequenceIterator<Sequence>::Base::end(seq);
}

}  // namespace DCPS
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
