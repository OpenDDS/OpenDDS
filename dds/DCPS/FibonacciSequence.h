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
  explicit FibonacciSequence(const T& f1, const T& f1_minus_1 = T(0))
    : f_n_(f1)
    , f_n_minus_1_(f1_minus_1)
  {}

  T get() const
  {
    return f_n_;
  }

  void advance(const T& fmax = T(0))
  {
    const T f_n_plus_1 = f_n_minus_1_ + f_n_;
    f_n_minus_1_ = f_n_;
    f_n_ = f_n_plus_1;
    if (fmax != T(0)) {
      f_n_ = (std::min)(f_n_, fmax);
    }
  }

  void set(const T& f1, const T& f1_minus_1 = T(0))
  {
    f_n_ = f1;
    f_n_minus_1_ = f1_minus_1;
  }

private:
  T f_n_;
  T f_n_minus_1_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
