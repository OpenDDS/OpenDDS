/* -*- C++ -*- */

#ifndef _REFERENCE_COUNTER_H_
#define _REFERENCE_COUNTER_H_

/**
 * @class Reference_Counter_T
 * @brief This class does reference counting in its constructor and
 *        destructor. The inherited class must have a member element
 *        "int use_"
 */
template <class T>
class Reference_Counter_T
{
public:

  /// default Ctor
  Reference_Counter_T ();

  /// Ctor with refcounting functionality
  /// @param ptr of the object that should be managed
  /// @param own indicates if Reference_Counter_T should take ownership
  Reference_Counter_T (T* ptr, bool increase_count = false);

  /// copy Ctor
  Reference_Counter_T (const Reference_Counter_T& rhs);

  /// Dtor will delete pointer if refcount becomes 0
  virtual ~Reference_Counter_T ();

  /// assignment operator
  void operator= (const Reference_Counter_T& rhs);

  /// dereference operator
  T *operator-> () const;

  /// returns the underlying pointer
  T *ptr () const;
  
  /// returns true if the internal pointer is 0
  bool is_null () const;

 private:
  /// implementation of the increment operation
  void increment ();

  /// implementation of the decrement operation
  void decrement ();

 private:
  /// reference to the refcounted object
  T* ptr_;
};

#include "Reference_Counter_T.cpp"

#endif /* _REFERENCE_COUNTER_H_ */
