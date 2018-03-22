/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef DDS_DCPS_ITERATOR_ADAPTOR_H
#define DDS_DCPS_ITERATOR_ADAPTOR_H

#include "tao/Unbounded_Value_Sequence_T.h"
#include <iterator>

namespace OpenDDS {
  namespace DCPS {

    template <typename T>
    class unbounded_value_sequence_iterator;

    template <typename T>
    inline unbounded_value_sequence_iterator<T> sequence_begin(T& sequence)
    {
      unbounded_value_sequence_iterator<T> iter(sequence);
      iter.current_ = 0;
      return iter;
    }

    template <typename T>
    inline unbounded_value_sequence_iterator<T> sequence_end(T& sequence)
    {
      unbounded_value_sequence_iterator<T> iter(sequence);
      iter.current_ = sequence.length();
      return iter;
    }

    template <typename T>
    class unbounded_value_sequence_back_insert_iterator
    {
    public:
      typedef unbounded_value_sequence_back_insert_iterator Iter_Type;

      typedef std::output_iterator_tag iterator_category;
      typedef typename T::value_type value_type;
      typedef int difference_type;
      typedef typename T::value_type* pointer;
      typedef typename T::value_type& reference;

      unbounded_value_sequence_back_insert_iterator(T& sequence) :
        seq_(sequence) { }

      unbounded_value_sequence_back_insert_iterator(const Iter_Type& other) :
        seq_(other.seq_) { }

      void push_back(const typename T::value_type& value)
      {
        size_t len = seq_.length();

        if (len == seq_.maximum()) {
          // Force a resize and copy that won't occur every invocation
          size_t newmax = len + len/2;
          seq_.length(newmax);
        }

        seq_.length(len + 1);
        seq_[len] = value;
      }

      Iter_Type& operator= (const typename T::value_type& value)
      {
        push_back(value);
        return *this;
      }

      Iter_Type& operator* ()
      {
        return *this;
      }

      Iter_Type& operator++ ()
      {
        return *this;
      }

      Iter_Type operator++ (int)
      {
          return *this;
      }

    protected:
      T& seq_;
    };

    template<typename T>
      inline unbounded_value_sequence_back_insert_iterator<T>
      back_inserter(T& sequence)
      {
        return unbounded_value_sequence_back_insert_iterator<T>(sequence);
      }

    template <typename T>
    class unbounded_value_sequence_iterator
    {
    public:
      typedef unbounded_value_sequence_iterator Iter_Type;

      template <typename U>
      friend unbounded_value_sequence_iterator<U> sequence_begin(U&);

      template <typename U>
      friend unbounded_value_sequence_iterator<U> sequence_end(U&);

      typedef std::random_access_iterator_tag iterator_category;
      typedef typename T::value_type value_type;
      typedef int difference_type;
      typedef typename T::value_type* pointer;
      typedef typename T::value_type& reference;

      typedef std::iterator_traits<Iter_Type> Traits_Type;
      typedef typename Traits_Type::iterator_category Category;
      typedef typename Traits_Type::value_type Value_Type;
      typedef typename Traits_Type::difference_type Difference_Type;
      typedef typename Traits_Type::pointer Pointer;
      typedef typename Traits_Type::reference Reference;

      unbounded_value_sequence_iterator() :
        seq_(), current_(0) { }

      unbounded_value_sequence_iterator(T& sequence) :
        seq_(sequence), current_(0) { }

      unbounded_value_sequence_iterator(Iter_Type& from) :
        seq_(from.seq_), current_(from.current_) { }

      unbounded_value_sequence_iterator(const Iter_Type& from) :
        seq_(from.seq_), current_(from.current_) { }

      operator Difference_Type ()
      {
        return current_;
      }

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

      Reference operator[] (Difference_Type n) const
      {
        return seq_[n];
      }

      Iter_Type& operator+= (Difference_Type n)
      {
        current_ += n;
        return *this;
      }

      Iter_Type operator+ (Difference_Type n)
      {
        Iter_Type iter(*this);
        iter.current_ += n;
        return iter;
      }

      Iter_Type& operator-= (Difference_Type n)
      {
        current_ -= n;
        return *this;
      }

      Iter_Type operator- (Difference_Type n)
      {
        Iter_Type iter(*this);
        iter.current_ -= n;
        return iter;
      }

      Iter_Type& operator+= (const Iter_Type& rhs)
      {
        current_ += rhs.current_;
        return *this;
      }

      Iter_Type operator+ (const Iter_Type& rhs) const
      {
        Iter_Type iter(*this);
        iter.current_ += rhs.current_;
        return iter;
      }

      Iter_Type& operator-= (const Iter_Type& rhs)
      {
        current_ -= rhs.current_;
        return *this;
      }

      Iter_Type operator- (const Iter_Type& rhs)
      {
        Iter_Type iter(*this);
        iter.current_ -= rhs.current_;
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
      T& seq_;
      Difference_Type current_;
    };

  }
}

#endif
