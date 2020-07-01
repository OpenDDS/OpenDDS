#ifndef _TYPE_LOOKUP_GET_DEPENDENCIES_IN_H_
#define _TYPE_LOOKUP_GET_DEPENDENCIES_IN_H_


#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


#include "tao/AnyTypeCode/AnyTypeCode_methods.h"
#include "tao/AnyTypeCode/Any.h"
#include "tao/ORB.h"
#include "tao/Basic_Types.h"
#include "tao/String_Manager_T.h"
#include "tao/Sequence_T.h"
#include "tao/Seq_Var_T.h"
#include "tao/Seq_Out_T.h"
#include "tao/VarOut_T.h"
#include "tao/Arg_Traits_T.h"
#include "tao/Basic_Arguments.h"
#include "tao/Special_Basic_Arguments.h"
#include "tao/Any_Insert_Policy_T.h"
#include "tao/Fixed_Size_Argument_T.h"
#include "tao/Var_Size_Argument_T.h"
#include /**/ "tao/Version.h"
#include /**/ "tao/Versioned_Namespace.h"

#include "TypeObject.h"

namespace OpenDDS
{
namespace XTypes
{
#if !defined (_XTYPES_TYPEIDENTIFIERSEQ_CH_)
#define _XTYPES_TYPEIDENTIFIERSEQ_CH_

  //class TypeIdentifierSeq;

  typedef
    ::TAO_VarSeq_Var_T<
        TypeIdentifierSeq
      >
    TypeIdentifierSeq_var;

  typedef
    ::TAO_Seq_Out_T<
        TypeIdentifierSeq
      >
    TypeIdentifierSeq_out;

//  class  TypeIdentifierSeq
//    : public
//        ::TAO::unbounded_value_sequence<
//            TypeIdentifier
//          >
//  {
//  public:
//    TypeIdentifierSeq (void);
//    TypeIdentifierSeq ( ::CORBA::ULong max);
//    TypeIdentifierSeq (
//      ::CORBA::ULong max,
//      ::CORBA::ULong length,
//      TypeIdentifier* buffer,
//      ::CORBA::Boolean release = false);
//#if defined (ACE_HAS_CPP11)
//    TypeIdentifierSeq (const TypeIdentifierSeq &) = default;
//    TypeIdentifierSeq (TypeIdentifierSeq &&) = default;
//    TypeIdentifierSeq& operator= (const TypeIdentifierSeq &) = default;
//    TypeIdentifierSeq& operator= (TypeIdentifierSeq &&) = default;
//#endif /* ACE_HAS_CPP11 */
//    virtual ~TypeIdentifierSeq (void);
//    
//    typedef TypeIdentifierSeq_var _var_type;
//    typedef TypeIdentifierSeq_out _out_type;
//
//    static void _tao_any_destructor (void *);
//  };

#endif /* end #if !defined */

  extern  ::CORBA::TypeCode_ptr const _tc_TypeIdentifierSeq;

#if !defined (_XTYPES_CONTINUATION_POINT_SEQ_CH_)
#define _XTYPES_CONTINUATION_POINT_SEQ_CH_

  class continuation_point_Seq;

  typedef
    ::TAO_FixedSeq_Var_T<
        continuation_point_Seq
      >
    continuation_point_Seq_var;

  typedef
    ::TAO_Seq_Out_T<
        continuation_point_Seq
      >
    continuation_point_Seq_out;

  // TODO: do we need to add 'generated' *C.cpp file with implementation of serialization methods, or remove their declarations from the header?
  class  continuation_point_Seq
    : public
        ::TAO::bounded_value_sequence<
            ::CORBA::Octet,
            32
          >
  {
  public:
    continuation_point_Seq(void) {}
    // TODO: do we need to add 'generated' *C.cpp file with implementation?
    //continuation_point_Seq (
    //  ::CORBA::ULong length,
    //  ::CORBA::Octet* buffer,
    //  ::CORBA::Boolean release = false);
#if defined (ACE_HAS_CPP11)
    continuation_point_Seq (const continuation_point_Seq &) = default;
    continuation_point_Seq (continuation_point_Seq &&) = default;
    continuation_point_Seq& operator= (const continuation_point_Seq &) = default;
    continuation_point_Seq& operator= (continuation_point_Seq &&) = default;
#endif /* ACE_HAS_CPP11 */
    virtual ~continuation_point_Seq(void) {}
    
    typedef continuation_point_Seq_var _var_type;
    typedef continuation_point_Seq_out _out_type;

    // TODO: do we need to add 'generated' *C.cpp file with implementation?
    //static void _tao_any_destructor (void *);
  };

#endif /* end #if !defined */

  extern  ::CORBA::TypeCode_ptr const _tc_continuation_point_Seq;

  struct TypeLookup_getTypeDependencies_In;

  typedef
    ::TAO_Var_Var_T<
        TypeLookup_getTypeDependencies_In
      >
    TypeLookup_getTypeDependencies_In_var;

  typedef
    ::TAO_Out_T<
        TypeLookup_getTypeDependencies_In
      >
    TypeLookup_getTypeDependencies_In_out;

  struct  TypeLookup_getTypeDependencies_In
  {   
    typedef TypeLookup_getTypeDependencies_In_var _var_type;
    typedef TypeLookup_getTypeDependencies_In_out _out_type;

    static void _tao_any_destructor (void *);
    
    XTypes::TypeIdentifierSeq type_ids;
    XTypes::continuation_point_Seq continuation_point;
  };

  extern  ::CORBA::TypeCode_ptr const _tc_TypeLookup_getTypeDependencies_In;

} // module XTypes
}

TAO_BEGIN_VERSIONED_NAMESPACE_DECL


// Arg traits specializations.
namespace TAO
{
  template<>
  class Arg_Traits< OpenDDS::XTypes::TypeIdentifierSeq>
    : public
        Var_Size_Arg_Traits_T<
            OpenDDS::XTypes::TypeIdentifierSeq,
            TAO::Any_Insert_Policy_Stream
          >
  {
  };

  template<>
  class Arg_Traits< OpenDDS::XTypes::continuation_point_Seq>
    : public
        Var_Size_Arg_Traits_T<
            OpenDDS::XTypes::continuation_point_Seq,
            TAO::Any_Insert_Policy_Stream
          >
  {
  };

  template<>
  class Arg_Traits< OpenDDS::XTypes::TypeLookup_getTypeDependencies_In>
    : public
        Var_Size_Arg_Traits_T<
            OpenDDS::XTypes::TypeLookup_getTypeDependencies_In,
            TAO::Any_Insert_Policy_Stream
          >
  {
  };
}

TAO_END_VERSIONED_NAMESPACE_DECL



TAO_BEGIN_VERSIONED_NAMESPACE_DECL

// Traits specializations.
namespace TAO
{
}
TAO_END_VERSIONED_NAMESPACE_DECL


#if defined (ACE_ANY_OPS_USE_NAMESPACE)

namespace XTypes
{
   void operator<<= ( ::CORBA::Any &, const ::XTypes::TypeIdentifierSeq &); // copying version
   void operator<<= ( ::CORBA::Any &, ::XTypes::TypeIdentifierSeq*); // noncopying version
   ::CORBA::Boolean operator>>= (const ::CORBA::Any &, ::XTypes::TypeIdentifierSeq *&); // deprecated
   ::CORBA::Boolean operator>>= (const ::CORBA::Any &, const ::XTypes::TypeIdentifierSeq *&);
}

#else


TAO_BEGIN_VERSIONED_NAMESPACE_DECL

 void operator<<= ( ::CORBA::Any &, const OpenDDS::XTypes::TypeIdentifierSeq &); // copying version
 void operator<<= ( ::CORBA::Any &, OpenDDS::XTypes::TypeIdentifierSeq*); // noncopying version
 ::CORBA::Boolean operator>>= (const ::CORBA::Any &, OpenDDS::XTypes::TypeIdentifierSeq *&); // deprecated
 ::CORBA::Boolean operator>>= (const ::CORBA::Any &, const OpenDDS::XTypes::TypeIdentifierSeq *&);
TAO_END_VERSIONED_NAMESPACE_DECL



#endif


#if defined (ACE_ANY_OPS_USE_NAMESPACE)

namespace XTypes
{
   void operator<<= ( ::CORBA::Any &, const ::XTypes::continuation_point_Seq &); // copying version
   void operator<<= ( ::CORBA::Any &, ::XTypes::continuation_point_Seq*); // noncopying version
   ::CORBA::Boolean operator>>= (const ::CORBA::Any &, ::XTypes::continuation_point_Seq *&); // deprecated
   ::CORBA::Boolean operator>>= (const ::CORBA::Any &, const ::XTypes::continuation_point_Seq *&);
}

#else


TAO_BEGIN_VERSIONED_NAMESPACE_DECL

 void operator<<= ( ::CORBA::Any &, const OpenDDS::XTypes::continuation_point_Seq &); // copying version
 void operator<<= ( ::CORBA::Any &, OpenDDS::XTypes::continuation_point_Seq*); // noncopying version
 ::CORBA::Boolean operator>>= (const ::CORBA::Any &, OpenDDS::XTypes::continuation_point_Seq *&); // deprecated
 ::CORBA::Boolean operator>>= (const ::CORBA::Any &, const OpenDDS::XTypes::continuation_point_Seq *&);
TAO_END_VERSIONED_NAMESPACE_DECL



#endif


#if defined (ACE_ANY_OPS_USE_NAMESPACE)

namespace XTypes
{
   void operator<<= (::CORBA::Any &, const ::XTypes::TypeLookup_getTypeDependencies_In &); // copying version
   void operator<<= (::CORBA::Any &, ::XTypes::TypeLookup_getTypeDependencies_In*); // noncopying version
   ::CORBA::Boolean operator>>= (const ::CORBA::Any &, ::XTypes::TypeLookup_getTypeDependencies_In *&); // deprecated
 ::CORBA::Boolean operator>>= (const ::CORBA::Any &, const ::XTypes::TypeLookup_getTypeDependencies_In *&);
}

#else


TAO_BEGIN_VERSIONED_NAMESPACE_DECL

 void operator<<= (::CORBA::Any &, const OpenDDS::XTypes::TypeLookup_getTypeDependencies_In &); // copying version
 void operator<<= (::CORBA::Any &, OpenDDS::XTypes::TypeLookup_getTypeDependencies_In*); // noncopying version
 ::CORBA::Boolean operator>>= (const ::CORBA::Any &, OpenDDS::XTypes::TypeLookup_getTypeDependencies_In *&); // deprecated
 ::CORBA::Boolean operator>>= (const ::CORBA::Any &, const OpenDDS::XTypes::TypeLookup_getTypeDependencies_In *&);
TAO_END_VERSIONED_NAMESPACE_DECL



#endif


#if !defined _TAO_CDR_OP_XTypes_TypeIdentifierSeq_H_
#define _TAO_CDR_OP_XTypes_TypeIdentifierSeq_H_
TAO_BEGIN_VERSIONED_NAMESPACE_DECL


 ::CORBA::Boolean operator<< (
    TAO_OutputCDR &strm,
    const OpenDDS::XTypes::TypeIdentifierSeq &_tao_sequence);
 ::CORBA::Boolean operator>> (
    TAO_InputCDR &strm,
    OpenDDS::XTypes::TypeIdentifierSeq &_tao_sequence);

TAO_END_VERSIONED_NAMESPACE_DECL



#endif /* _TAO_CDR_OP_XTypes_TypeIdentifierSeq_H_ */


#if !defined _TAO_CDR_OP_XTypes_continuation_point_Seq_H_
#define _TAO_CDR_OP_XTypes_continuation_point_Seq_H_
TAO_BEGIN_VERSIONED_NAMESPACE_DECL


 ::CORBA::Boolean operator<< (
    TAO_OutputCDR &strm,
    const OpenDDS::XTypes::continuation_point_Seq &_tao_sequence);
 ::CORBA::Boolean operator>> (
    TAO_InputCDR &strm,
    OpenDDS::XTypes::continuation_point_Seq &_tao_sequence);

TAO_END_VERSIONED_NAMESPACE_DECL



#endif /* _TAO_CDR_OP_XTypes_continuation_point_Seq_H_ */


TAO_BEGIN_VERSIONED_NAMESPACE_DECL

 ::CORBA::Boolean operator<< (TAO_OutputCDR &, const OpenDDS::XTypes::TypeLookup_getTypeDependencies_In &);
 ::CORBA::Boolean operator>> (TAO_InputCDR &, OpenDDS::XTypes::TypeLookup_getTypeDependencies_In &);

TAO_END_VERSIONED_NAMESPACE_DECL


#if defined (__ACE_INLINE__)
#include "i3C.inl"
#endif /* defined INLINE */

#endif /* ifndef _TYPE_LOOKUP_GET_DEPENDENCIES_IN_H_ */
