// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_DEFINITION_H
#define TAO_DDS_DCPS_DEFINITION_H


#include "Cached_Allocator_With_Overflow_T.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "ace/Message_Block.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace TAO
{
  namespace DCPS
  {
    typedef ACE_UINT16 CoherencyGroup ;
    typedef RepoId PublicationId;
   
    const ::DDS::InstanceHandle_t HANDLE_NIL = 0;
    const ::CORBA::ULong DEFAULT_STATUS_KIND_MASK = 0xFFFF;

    /// Lolipop sequencing (never wrap to negative).
    /// This helps distinguish new and old sequence numbers. (?)
    struct TAO_DdsDcps_Export SequenceNumber 
    {
      /// Default constructor starts negative.
      SequenceNumber() { value_ = SHRT_MIN ; }

      /// Pre-increment.
      SequenceNumber operator++() {
        this->increment() ;
        return *this ;
      }

      /// Post-increment.
      SequenceNumber operator++(int) {
        SequenceNumber value = *this ;
        this->increment() ;
        return value ;
      }

      bool operator==( const SequenceNumber& rvalue) { return value_ == rvalue.value_ ; }
      bool operator!=( const SequenceNumber& rvalue) { return value_ != rvalue.value_ ; }
      bool operator<(  const SequenceNumber& rvalue) { return value_  < rvalue.value_ ; }
      bool operator>(  const SequenceNumber& rvalue) { return value_  > rvalue.value_ ; }
      bool operator>=( const SequenceNumber& rvalue) { return value_ >= rvalue.value_ ; }
      bool operator<=( const SequenceNumber& rvalue) { return value_ <= rvalue.value_ ; }

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

      T_impl* svt_;
      T_var   obj_;
    };

  } // namespace TAO
} // namespace DCPS

#endif /* TAO_DDS_DCPS_DEFINITION_H */

