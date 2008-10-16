/* -*- C++ -*- */
#ifndef _REFERENCE_COUNTER_CPP_
#define _REFERENCE_COUNTER_CPP_

template <class T>
Reference_Counter_T<T>::Reference_Counter_T ()
  : ptr_ (0)
{
}

template <class T>
Reference_Counter_T<T>::Reference_Counter_T (T* ptr, bool increase_count)
  : ptr_ (ptr)
{
  // by default we assume, the ptr will be created with use_ = 1
  // and that the Reference_Counter_T is taking over ownership, meaning,
  // that is responsible for deletion of the ptr.
  // If this is not the case, the user can set increase count to
  // true, which results in incrementing the reference count for a
  // passed in object.
  if (increase_count)
    increment ();
}

template <class T>
Reference_Counter_T<T>::Reference_Counter_T (const Reference_Counter_T& rhs)
  : ptr_ (rhs.ptr_)
{
  increment ();
}

template <class T>
Reference_Counter_T<T>::~Reference_Counter_T ()
{
  decrement ();
}

template <class T>
void
Reference_Counter_T<T>::operator= (const Reference_Counter_T& rhs)
{
  if (this != &rhs)
    {
      decrement ();
      ptr_ = rhs.ptr_;
      increment ();
    }
}

template <class T>
T* 
Reference_Counter_T<T>::operator-> () const
{
  return ptr_;
}

template <class T>
T*
Reference_Counter_T<T>::ptr () const
{
  return ptr_;
}

template <class T>
bool
Reference_Counter_T<T>::is_null () const
{
  return (ptr_ == 0);
}  

template <class T>
void
Reference_Counter_T<T>::increment ()
{
  // increment reference count
  if (ptr_ != 0)
    ++ptr_->use_;    
}

template <class T>
void
Reference_Counter_T<T>::decrement ()
{
  // decrement reference count
  if (ptr_ != 0)
    {
      --ptr_->use_;
      
      // delete object if necessary
      if (ptr_->use_ == 0)
        delete ptr_;
    }
}

#endif /* _REFERENCE_COUNTER_CPP_ */
