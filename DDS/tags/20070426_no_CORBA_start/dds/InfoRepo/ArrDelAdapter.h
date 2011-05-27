#ifndef _ARR_DEL_ADAPTER_
#define _ARR_DEL_ADAPTER_

#include <memory>

/// A auto_ptr implementation to handle
///  array memory management.

template <typename T>
class ArrDelAdapter
{
 public:
  ArrDelAdapter(T *p) : p_(p) { }
  ArrDelAdapter (const ArrDelAdapter<T>& var)
    : p_ (var.p_)
    {
      // copier gets sole handle to ptr
      const_cast<ArrDelAdapter<T>&>(var).p_ = 0;
    }
  ~ArrDelAdapter() { delete [] p_; }
  // operators like ->, *, etc...
 private:
  T* p_;
};

#endif // _ARR_DEL_ADAPTER_
