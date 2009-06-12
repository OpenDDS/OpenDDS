// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_DEFINITION_H
#define TAO_DDS_DCPS_DEFINITION_H


#include "Cached_Allocator_With_Overflow_T.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "ace/Message_Block.h"

#include <functional>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace OpenDDS
{
  namespace DCPS
  {
    typedef ACE_UINT16 CoherencyGroup ;
    typedef RepoId PublicationId;

    const ::CORBA::ULong DEFAULT_STATUS_KIND_MASK = 0xFFFF;

    /// Lolipop sequencing (never wrap to negative).
    /// This helps distinguish new and old sequence numbers. (?)
    struct OpenDDS_Dcps_Export SequenceNumber 
    {
      /// Construct with a value, default to negative starting point.
      SequenceNumber( ACE_INT16 value = SHRT_MIN) : value_( value) { }

      // N.B: Default copy constructor is sufficient.

      /// Allow assignments.
      SequenceNumber& operator=( const SequenceNumber& rhs) {
        value_ = rhs.value_;
        return *this; 
      }

      /// Pre-increment.
      SequenceNumber& operator++() {
        this->increment() ;
        return *this ;
      }

      /// Post-increment.
      SequenceNumber operator++(int) {
        SequenceNumber value (*this);
        ++*this;
        return value ;
      }

      /// Convert to integer type.
      operator ACE_INT16() { return this->value_; }

      /// This is the magic of the lollipop.
      /// N.B. This comparison assumes that the shortest distance between
      ///      the values being compared is the positive counting
      ///      sequence between them.  This means that MAX-2 is less
      ///      than 2 since they are separated by only four counts from
      ///      MAX-2 to 2.  But that 2 is less than MAX/2 since the
      ///      shortest distance is from 2 to MAX/2.
      bool operator<(  const SequenceNumber& rvalue) const {
             ACE_INT16 distance = rvalue.value_ - value_;
             return (distance == 0)? false:                    // Equal is not less than.
                    (value_ < 0)?    (value_ < rvalue.value_): // Stem of lollipop.
                    (distance <  0)? (SHRT_MAX/2 < -distance): // Closest distance dominates.
                                     (distance < (SHRT_MAX/2));
           }

      /// Derive a full suite of logical operations.
      bool operator==( const SequenceNumber& rvalue) const { return value_ == rvalue.value_ ; }
      bool operator!=( const SequenceNumber& rvalue) const { return value_ != rvalue.value_ ; }
      bool operator>=( const SequenceNumber& rvalue) const { return !(*this  < rvalue);  }
      bool operator<=( const SequenceNumber& rvalue) const { return !(rvalue < *this); }
      bool operator>(  const SequenceNumber& rvalue) const { return  (rvalue < *this)
                                                                  && (*this != rvalue);  }

      ACE_INT16 value_ ;

      /// Increment operation itself.
      void increment() {
        /// Lolipop sequencing (never wrap to negative).
        if( this->value_ == 0x7fff) this->value_ = 0x0 ;
        else                        this->value_++ ;
      }
    } ;


    typedef Cached_Allocator_With_Overflow<ACE_Message_Block, ACE_Thread_Mutex>  
      MessageBlockAllocator;
    typedef Cached_Allocator_With_Overflow<ACE_Data_Block, ACE_Null_Mutex>  
      DataBlockAllocator;
    
#define DUP true
#define NO_DUP false


    /// This struct holds both object reference and the corresponding servant.
    template < typename T_impl, typename T, typename T_ptr, typename T_var >
    struct Objref_Servant_Pair 
    {
      Objref_Servant_Pair ()
        : svt_ (0)
      {
      };

      Objref_Servant_Pair (T_impl* svt, T_ptr obj, bool dup)
        : svt_ (svt)
      {
        if (dup)
          {
            obj_ = T::_duplicate (obj);
          }
        else
          {
            obj_ = obj;
          }
      };

      ~Objref_Servant_Pair ()
      {
      };

      bool operator== (const Objref_Servant_Pair & pair) const
      {
        return pair.svt_ == this->svt_;
      };

      bool operator< (const Objref_Servant_Pair & pair) const
      {
        return this->svt_ < pair.svt_;
      };

      T_impl* svt_;
      T_var   obj_;
    };

    /// Use a Foo_var in a std::set or std::map with this comparison function,
    /// for example std::set<Foo_var, VarLess<Foo> >
    template <class T, class V = typename T::_var_type>
    struct VarLess : public std::binary_function<V, V, bool>
    {
      bool operator() (const V& x, const V& y) const
      {
        return x.in() < y.in();
      }
    };

  } // namespace OpenDDS
} // namespace DCPS

#endif /* TAO_DDS_DCPS_DEFINITION_H */

