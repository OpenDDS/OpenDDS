/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef DDS_DCPS_ITERATOR_ADAPTORS_H
#define DDS_DCPS_ITERATOR_ADAPTORS_H

#include "tao/Unbounded_Value_Sequence_T.h"
#include <iterator>

namespace OpenDDS {
  namespace DCPS {

    template <typename T>
    class ace_unbounded_value_sequence_iterator;

    template <typename T>
    ace_unbounded_value_sequence_iterator<T> sequence_begin(TAO::unbounded_value_sequence<T>& sequence)
    {
      ace_unbounded_value_sequence_iterator<T> iter(sequence);
      iter.current_ = 0;
      return iter;
    }

    template <typename T>
    ace_unbounded_value_sequence_iterator<T> sequence_end(TAO::unbounded_value_sequence<T>& sequence)
    {
      ace_unbounded_value_sequence_iterator<T> iter(sequence);
      iter.current_ = sequence.length();
      return iter;
    }

    template <typename T>
    class ace_unbounded_value_sequence_iterator
    {
    public:
      typedef ace_unbounded_value_sequence_iterator Iter_Type;
      typedef TAO::unbounded_value_sequence<T> Ace_Sequence_Type;
      typedef typename Ace_Sequence_Type::value_type Ace_Value_Type;

      friend Iter_Type sequence_begin<>(Ace_Sequence_Type&);
      friend Iter_Type sequence_end<>(Ace_Sequence_Type&);

      typedef std::random_access_iterator_tag iterator_category;
      typedef Ace_Value_Type value_type;
      typedef CORBA::ULong difference_type;
      typedef Ace_Value_Type* pointer;
      typedef Ace_Value_Type& reference;

      typedef std::iterator_traits<Iter_Type> Traits_Type;
      typedef typename Traits_Type::iterator_category Category;
      typedef typename Traits_Type::value_type Value_Type;
      typedef typename Traits_Type::difference_type Difference_Type;
      typedef typename Traits_Type::pointer Pointer;
      typedef typename Traits_Type::reference Reference;

      ace_unbounded_value_sequence_iterator() :
        seq_(), current_(0) { }

      explicit
      ace_unbounded_value_sequence_iterator(Ace_Sequence_Type& sequence) :
        seq_(sequence), current_(0) { }

      explicit
      ace_unbounded_value_sequence_iterator(Iter_Type& from) :
        seq_(from.seq_), current_(from.current_) { }

      // Forward iterator requirements

      Reference operator* () const
      {
        return seq_[current_];
      }

      Pointer operator-> () const
      {
        return &seq_[current_];
      }

      Iter_Type& operator++ ()
      {
        ++current_;
        return *this;
      }

      Iter_Type operator++ (int)
      {
        Iter_Type iter(*this);
        ++current_;
        return iter;
      }

      bool operator== (const Iter_Type& rhs) const
      {
        return (seq_ == rhs.seq_) && (current_ == rhs.current_);
      }

      bool operator!= (const Iter_Type& rhs) const
      {
        return (seq_ != rhs.seq_) && (current_ != rhs.current_);
      }

      // Bidirectional iterator requirements

      Iter_Type& operator-- ()
      {
        --current_;
        return *this;
      }

      Iter_Type operator-- (int)
      {
        Iter_Type iter(*this);
        --current_;
        return iter;
      }

      // Random-access iterator requirements

      Reference operator[] (difference_type n) const
      {
        return seq_[n];
      }

      Iter_Type& operator+= (difference_type n)
      {
        current_ += n;
        return *this;
      }

      Iter_Type operator+ (difference_type n)
      {
        Iter_Type iter(*this);
        iter.current_ += n;
        return iter;
      }

      Iter_Type& operator-= (difference_type n)
      {
        current_ -= n;
        return *this;
      }

      Iter_Type operator- (difference_type n)
      {
        Iter_Type iter(*this);
        iter.current_ -= n;
        return iter;
      }

      bool operator< (const Iter_Type& rhs)
      {
        return current_ < rhs.current_;
      }

      bool operator> (const Iter_Type& rhs)
      {
        return current_ > rhs.current_;
      }

      bool operator<= (const Iter_Type& rhs)
      {
        return current_ <= rhs.current_;
      }

      bool operator>= (const Iter_Type& rhs)
      {
        return current_ >= rhs.current_;
      }

    protected:
      Ace_Sequence_Type& seq_;
      difference_type current_;
    };

  }
}

#endif
