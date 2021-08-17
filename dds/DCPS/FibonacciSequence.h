#ifndef OPENDDS_DCPS_FIBONACCI_SEQUENCE_H
#define OPENDDS_DCPS_FIBONACCI_SEQUENCE_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
class FibonacciSequence
{
public:
  FibonacciSequence(const T& f1) : f1_(f1), f_n_minus_1_(0), f_n_minus_2_(0)
  {
  }

  T get() const
  {
    if (f_n_minus_1_ == T(0)) {
      return f1_;
    } else {
      T f_n = f_n_minus_1_ + f_n_minus_2_;
      return f_n;
    }
  }

  void advance()
  {
    T f_n = f_n_minus_1_ == T(0) ? f1_ : f_n_minus_1_ + f_n_minus_2_;
    f_n_minus_2_ = f_n_minus_1_;
    f_n_minus_1_ = f_n;
  }

  void reset()
  {
    f_n_minus_1_ = T(0);
    f_n_minus_2_ = T(0);
  }

private:
  const T f1_;
  T f_n_minus_1_;
  T f_n_minus_2_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
