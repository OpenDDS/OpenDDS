/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ARR_DEL_ADAPTER
#define ARR_DEL_ADAPTER

/// A auto_ptr implementation to handle
///  array memory management.

template <typename T>
class ArrDelAdapter {
public:
  ArrDelAdapter(T *p) : p_(p) { }
  ArrDelAdapter(const ArrDelAdapter<T>& var)
  : p_(var.p_) {
    // copier gets sole handle to ptr
    const_cast<ArrDelAdapter<T>&>(var).p_ = 0;
  }
  ~ArrDelAdapter() {
    delete [] p_;
  }
  // operators like ->, *, etc...
private:
  T* p_;
};

#endif /* ARR_DEL_ADAPTER */
