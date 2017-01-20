#ifndef SCOPED_PTR_H_18C6F30C
#define SCOPED_PTR_H_18C6F30C

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


#include "dds/Versioned_Namespace.h"


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<class T>
class scoped_ptr {
   public:
     typedef T element_type;

     explicit scoped_ptr(T * p = 0) // never throws
       : ptr_(p)
     {}

     ~scoped_ptr() // never throws
     {
       delete ptr_;
     }

     void reset(T * p = 0) // never throws
     {
       delete ptr_;
       ptr_ = p;
     }

     T * release()
     {
       T* p = ptr_;
       ptr_ = 0;
       return p;
     }

     T & operator*() const // never throws
     {
       return *get();
     }

     T * operator->() const // never throws
     {
       return get();
     }

     T * get() const // never throws
     {
       return ptr_;
     }

     operator bool() const // never throws
     {
       return get() != 0;
     }

     void swap(scoped_ptr & b) // never throws
     {
       std::swap(ptr_, b.ptr_);
     }
   private:
     scoped_ptr(const scoped_ptr<T>&);
     scoped_ptr<T>& operator = (const scoped_ptr<T>&);

     T* ptr_;
  };

  template<class T>
  void swap(scoped_ptr<T> & a, scoped_ptr<T> & b) // never throws
  {
    return a.swap(b);
  }

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* end of include guard: SCOPED_PTR_H_18C6F30C */
