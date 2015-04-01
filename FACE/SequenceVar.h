#ifndef OPENDDS_FACE_SEQUENCE_VAR_HEADER
#define OPENDDS_FACE_SEQUENCE_VAR_HEADER


namespace OpenDDS {
namespace FaceTypes {

  template <typename T>
  class SequenceVar {
  public:
    typedef typename T::subscript_type T_elem;
    typedef typename T::const_subscript_type T_const_elem;

    SequenceVar ();
    SequenceVar (T* p);
    SequenceVar (const SequenceVar<T>& p);

    ~SequenceVar();

    SequenceVar& operator= (T* );
    SequenceVar& operator= (const SequenceVar<T>& );

    T* operator-> ();
    const T* operator-> () const;

    typedef const T&   _in_type;
    typedef       T&   _inout_type;
    typedef       T*&  _out_type;
    typedef       T*   _retn_type;

    // in, inout, out, _retn
    _in_type      in () const;
    _inout_type   inout ();
    _out_type     out ();
    _retn_type    _retn ();

    _retn_type    ptr () const;

    T_elem operator[] (FACE::UnsignedLong index);
    T_const_elem operator[] (FACE::UnsignedLong index) const;

  protected:
  private:
    T * ptr_;
  };

  template <typename T>
  inline SequenceVar<T>::SequenceVar ()
    : ptr_(0)
  {
  }

  template <typename T>
  inline SequenceVar<T>::SequenceVar (T* p)
    : ptr_(p)
  {
  }

  template <typename T>
  inline SequenceVar<T>::SequenceVar (const SequenceVar<T>& p)
    : ptr_ (p.ptr_ ? new T (*p.ptr_) : 0)
  {
  }

  template<typename T>
  inline
  SequenceVar<T>::~SequenceVar ()
  {
    delete this->ptr_;
  }

  template<typename T>
  inline
  SequenceVar<T>&
  SequenceVar<T>::operator= (T* p)
  {
    delete this->ptr_;
    this->ptr_ = p;
    return *this;
  }

  template<typename T>
  inline
  SequenceVar<T>&
  SequenceVar<T>::operator= (
      const SequenceVar<T>& p)
  {
    SequenceVar<T> tmp(p);

    T* old_ptr = this->ptr_;
    this->ptr_ = tmp.ptr_;
    tmp.ptr_ = old_ptr;

    return *this;
  }

  template<typename T>
  inline
  const T*
  SequenceVar<T>::operator-> () const
  {
    return this->ptr_;
  }

  template<typename T>
  inline
  T*
  SequenceVar<T>::operator-> ()
  {
    return this->ptr_;
  }

  template<typename T>
  inline
  const T&
  SequenceVar<T>::in () const
  {
    return *this->ptr_;
  }

  template<typename T>
  inline
  T&
  SequenceVar<T>::inout ()
  {
    return *this->ptr_;
  }

  template<typename T>
  inline
  T*&
  SequenceVar<T>::out ()
  {
    delete this->ptr_;
    this->ptr_ = 0;
    return this->ptr_;
  }

  template<typename T>
  inline
  T*
  SequenceVar<T>::_retn ()
  {
    T * tmp = this->ptr_;
    this->ptr_ = 0;
    return tmp;
  }

  template<typename T>
  inline
  T*
  SequenceVar<T>::ptr () const
  {
    return this->ptr_;
  }

  template<typename T>
  inline
  typename SequenceVar<T>::T_elem
  SequenceVar<T>::operator[] (FACE::UnsignedLong index)
  {
    return this->ptr_->operator[] (index);
  }

  template<typename T>
  inline
  typename SequenceVar<T>::T_const_elem
  SequenceVar<T>::operator[] (FACE::UnsignedLong index) const
  {
    return this->ptr_->operator[] (index);
  }
}
}

#endif
