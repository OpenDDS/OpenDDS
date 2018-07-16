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
  class sequence_iterator;

  template <typename T>
  inline sequence_iterator<T> sequence_begin(T& sequence)
  {
    sequence_iterator<T> iter(sequence);
    iter.current_ = 0;
    return iter;
  }

  template <typename T>
  inline sequence_iterator<T> sequence_end(T& sequence)
  {
    sequence_iterator<T> iter(sequence);
    iter.current_ = sequence.length();
    return iter;
  }

  template <typename T>
  class sequence_back_insert_iterator
  {
   public:
    typedef sequence_back_insert_iterator Iter_Type;

    typedef std::output_iterator_tag iterator_category;
    typedef typename T::value_type value_type;
    typedef int difference_type;
    typedef typename T::value_type* pointer;
    typedef typename T::value_type& reference;

    sequence_back_insert_iterator(T& sequence) : seq_(&sequence) {}

    Iter_Type& operator=(const typename T::value_type& value)
    {
      const size_t Default = 8;

      size_t len = seq_->length();

      if (len == seq_->maximum()) {
        const size_t newmax = (0 == len ? Default : len + len / 2);
        seq_->length(newmax);
      }

      seq_->length(len + 1);
      (*seq_)[len] = value;

      return *this;
    }

    Iter_Type& operator*() { return *this; }

    Iter_Type& operator++() { return *this; }

    Iter_Type operator++(int) { return *this; }

   private:
    T* seq_;
  };

  template <typename T>
  inline sequence_back_insert_iterator<T> back_inserter(T& sequence)
  {
    return sequence_back_insert_iterator<T>(sequence);
  }

  template <typename T>
  class sequence_iterator
  {
   public:
    typedef sequence_iterator Iter_Type;

    template <typename U>
    friend sequence_iterator<U> sequence_begin(U&);

    template <typename U>
    friend sequence_iterator<U> sequence_end(U&);

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

    sequence_iterator() : seq_(), current_(0) {}

    sequence_iterator(T& sequence) : seq_(&sequence), current_(0) {}

    operator Difference_Type() const { return current_; }

    // Forward iterator requirements

    Reference operator*() const { return (*seq_)[current_]; }

    Pointer operator->() const { return &(*seq_)[current_]; }

    Iter_Type& operator++()
    {
      ++current_;
      return *this;
    }

    Iter_Type operator++(int)
    {
      Iter_Type iter(*this);
      ++current_;
      return iter;
    }

    bool operator==(const Iter_Type& rhs) const
    {
      return (seq_ == rhs.seq_) && (current_ == rhs.current_);
    }

    bool operator!=(const Iter_Type& rhs) const { return !(*this == rhs); }

    // Bidirectional iterator requirements

    Iter_Type& operator--()
    {
      --current_;
      return *this;
    }

    Iter_Type operator--(int)
    {
      Iter_Type iter(*this);
      --current_;
      return iter;
    }

    // Random-access iterator requirements

    Reference operator[](Difference_Type n) const { return (*seq_)[n]; }

    Iter_Type& operator+=(Difference_Type n)
    {
      current_ += n;
      return *this;
    }

    Iter_Type operator+(Difference_Type n) const
    {
      Iter_Type iter(*this);
      iter.current_ += n;
      return iter;
    }

    Iter_Type& operator-=(Difference_Type n)
    {
      current_ -= n;
      return *this;
    }

    Iter_Type operator-(Difference_Type n) const
    {
      Iter_Type iter(*this);
      iter.current_ -= n;
      return iter;
    }

    Iter_Type& operator+=(const Iter_Type& rhs)
    {
      current_ += rhs.current_;
      return *this;
    }

    Iter_Type operator+(const Iter_Type& rhs) const
    {
      Iter_Type iter(*this);
      iter.current_ += rhs.current_;
      return iter;
    }

    Iter_Type& operator-=(const Iter_Type& rhs)
    {
      current_ -= rhs.current_;
      return *this;
    }

    Iter_Type operator-(const Iter_Type& rhs) const
    {
      Iter_Type iter(*this);
      iter.current_ -= rhs.current_;
      return iter;
    }

    bool operator<(const Iter_Type& rhs) const
    {
      return current_ < rhs.current_;
    }

    bool operator>(const Iter_Type& rhs) const
    {
      return current_ > rhs.current_;
    }

    bool operator<=(const Iter_Type& rhs) const
    {
      return current_ <= rhs.current_;
    }

    bool operator>=(const Iter_Type& rhs) const
    {
      return current_ >= rhs.current_;
    }

   private:
    T* seq_;
    Difference_Type current_;
  };

}  // namespace DCPS
}  // namespace OpenDDS

#endif
