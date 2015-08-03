#ifndef dds_DCPS_SafetyProfileSequencVar_h
#define dds_DCPS_SafetyProfileSequencVar_h


namespace OpenDDS {
namespace SafetyProfile {

  /**
   * @class SequenceVar
   *
   * @brief Parametrized implementation of var class for sequences.
   *
   */
  template <typename T>
  class SequenceVar {
  public:
    typedef typename T::subscript_type T_elem;
    typedef typename T::const_subscript_type T_const_elem;

    SequenceVar()
      : ptr_(0)
    {
    }

    SequenceVar(T* p)
      : ptr_(p)
    {
    }

    SequenceVar(const SequenceVar<T>& p)
      : ptr_(p.ptr_ ? new T(*p.ptr_) : 0)
    {
    }

    ~SequenceVar()
    {
      delete ptr_;
    }


    SequenceVar& operator=(T* p);
    SequenceVar& operator=(const SequenceVar<T>& p);

    T* operator->()
    {
      return ptr_;
    }

    const T* operator->() const
    {
      return ptr_;
    }


    typedef const T& _in_type;
    typedef T& _inout_type;
    typedef T*& _out_type;
    typedef T* _retn_type;

    _in_type in() const
    {
      return *ptr_;
    }

    _inout_type inout()
    {
      return *ptr_;
    }

    _out_type out();
    _retn_type _retn();

    _retn_type ptr() const
    {
      return ptr_;
    }

    operator _in_type() const
    {
      return in();
    }

    operator _inout_type()
    {
      return inout();
    }

    operator _out_type()
    {
      return out();
    }


    T_elem operator[](CORBA::ULong index)
    {
      return (*ptr_)[index];
    }

    T_const_elem operator[](CORBA::ULong index) const
    {
      return (*ptr_)[index];
    }

  private:
    T* ptr_;
  };

  template<typename T>
  inline
  SequenceVar<T>&
  SequenceVar<T>::operator=(T* p)
  {
    delete ptr_;
    ptr_ = p;
    return *this;
  }

  template<typename T>
  inline
  SequenceVar<T>&
  SequenceVar<T>::operator=(
      const SequenceVar<T>& p)
  {
    SequenceVar<T> tmp(p);

    T* old_ptr = ptr_;
    ptr_ = tmp.ptr_;
    tmp.ptr_ = old_ptr;

    return *this;
  }

  template<typename T>
  inline
  T*&
  SequenceVar<T>::out()
  {
    delete ptr_;
    ptr_ = 0;
    return ptr_;
  }

  template<typename T>
  inline
  T*
  SequenceVar<T>::_retn()
  {
    T* tmp = ptr_;
    ptr_ = 0;
    return tmp;
  }
}
}

#endif /* dds_DCPS_SafetyProfileSequencVar_h */
