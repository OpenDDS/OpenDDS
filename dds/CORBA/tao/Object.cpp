// @(#) $Id$
//
// Copyright 1994-1995 by Sun Microsystems Inc.
// Copyright 1997-2002 by Washington University
// All Rights Reserved
//
// ORB:         CORBA::Object operations

#include "tao/Object.h"
#include "tao/SystemException.h"

#include "ace/Dynamic_Service.h"
#include "ace/OS_NS_string.h"
#include "ace/CORBA_macros.h"

#if !defined (__ACE_INLINE__)
# include "tao/Object.inl"
#endif /* ! __ACE_INLINE__ */

CORBA::Object::~Object (void)
{
}

CORBA::Object::Object (TAO_Stub * protocol_proxy,
                       CORBA::Boolean collocated,
                       TAO_Abstract_ServantBase * servant,
                       TAO_ORB_Core *orb_core)
  : is_local_ (false)
  , is_evaluated_ (true)
  , ior_ (0)
  , orb_core_ (orb_core)
  , protocol_proxy_ (protocol_proxy)
  , object_init_lock_ (0)
{
}

CORBA::Object::Object (IOP::IOR *ior,
                       TAO_ORB_Core *orb_core)
  : is_local_ (false)
  , is_evaluated_ (false)
  , ior_ (ior)
  , orb_core_ (orb_core)
  , protocol_proxy_ (0)
  , object_init_lock_ (0)
{
}

void
CORBA::Object::_add_ref (void)
{
  this->refcount_.increment ();
}

void
CORBA::Object::_remove_ref (void)
{
  if (this->refcount_.decrement () != 0)
    return;

  delete this;
}

CORBA::ULong
CORBA::Object::_refcount_value(void) const
{
  return static_cast<CORBA::ULong> (this->refcount_.value ());
}

void
CORBA::Object::_tao_any_destructor (void *x)
{
}

// virtual -- do not inline
CORBA::Boolean
CORBA::Object::marshal (TAO_OutputCDR &cdr)
{
  return false;
}

/*static*/ CORBA::Boolean
CORBA::Object::marshal (const CORBA::Object_ptr x,
                        TAO_OutputCDR &cdr)
{
  return false;
}

bool
CORBA::Object::can_convert_to_ior (void) const
{
  return false;
}

char*
CORBA::Object::convert_to_ior (bool, const char*) const
{
  return 0;
}

TAO_Abstract_ServantBase*
CORBA::Object::_servant (void) const
{
  return 0;
}

// IS_A ... ask the object if it's an instance of the type whose
// logical type ID is passed as a parameter.

CORBA::Boolean
CORBA::Object::_is_a (const char *type_id)
{
  throw ::CORBA::NO_IMPLEMENT();
}

const char*
CORBA::Object::_interface_repository_id (void) const
{
  return "IDL:omg.org/CORBA/Object:1.0";
}

CORBA::Boolean
CORBA::Object::_is_collocated (void) const
{
  return false;
}

void
CORBA::Object::set_collocated_servant (TAO_Abstract_ServantBase *b)
{
}

CORBA::Boolean
CORBA::Object::_is_local (void) const
{
  return this->is_local_;
}

TAO_Stub *
CORBA::Object::_stubobj (void) const
{
  return this->protocol_proxy_;
}

TAO_Stub *
CORBA::Object::_stubobj (void)
{
  
  return this->protocol_proxy_;
}

CORBA::ULong
CORBA::Object::_hash (CORBA::ULong maximum)
{
    {
      // Locality-constrained object.

      // Note that we reinterpret_cast to an "unsigned long" instead
      // of CORBA::ULong since we need to first cast to an integer
      // large enough to hold an address to avoid compile-time
      // warnings on some 64-bit platforms.
      const CORBA::ULong hash =
        static_cast<CORBA::ULong> (reinterpret_cast<ptrdiff_t> (this));

      return hash % maximum;
    }
}

CORBA::Boolean
CORBA::Object::_is_equivalent (CORBA::Object_ptr other_obj)
{
  if (other_obj == 0)
    {
      return false;
    }

  if (other_obj == this)
    {
      return true;
    }

  return false;
}

// TAO's extensions

TAO::ObjectKey *
CORBA::Object::_key (void)
{
  return 0;
}

void
CORBA::Object::_proxy_broker (TAO::Object_Proxy_Broker *proxy_broker)
{
}

CORBA::Boolean
CORBA::Object::is_nil_i (CORBA::Object_ptr obj)
{
  return !obj;
}



#if (TAO_HAS_MINIMUM_CORBA == 0)

#if !defined (CORBA_E_COMPACT) && !defined (CORBA_E_MICRO)
void
CORBA::Object::_create_request (CORBA::Context_ptr ctx,
                                const char *operation,
                                CORBA::NVList_ptr arg_list,
                                CORBA::NamedValue_ptr result,
                                CORBA::Request_ptr &request,
                                CORBA::Flags req_flags)
{
}
#endif

#if !defined (CORBA_E_COMPACT) && !defined (CORBA_E_MICRO)
void
CORBA::Object::_create_request (CORBA::Context_ptr ctx,
                                const char *operation,
                                CORBA::NVList_ptr arg_list,
                                CORBA::NamedValue_ptr result,
                                CORBA::ExceptionList_ptr exceptions,
                                CORBA::ContextList_ptr,
                                CORBA::Request_ptr &request,
                                CORBA::Flags req_flags)
{
}
#endif

#if !defined (CORBA_E_COMPACT) && !defined (CORBA_E_MICRO)
CORBA::Request_ptr
CORBA::Object::_request (const char *operation)
{
   return 0;
}
#endif

CORBA::Boolean
CORBA::Object::_non_existent (void)
{
  return false;
}


#if ! defined (CORBA_E_COMPACT) && ! defined (CORBA_E_MICRO)
CORBA::InterfaceDef_ptr
CORBA::Object::_get_interface (void)
{
  return 0;
}

CORBA::Object_ptr
CORBA::Object::_get_component (void)
{
  return 0;
}
#endif

char*
CORBA::Object::_repository_id (void)
{
  return 0;
}

#endif /* TAO_HAS_MINIMUM_CORBA */

// ****************************************************************

// @@ Does it make sense to support policy stuff for locality constrained
//    objects?  Also, does it make sense to bind policies with stub object?
//    - nw.

#if (TAO_HAS_CORBA_MESSAGING == 1)

CORBA::Policy_ptr
CORBA::Object::_get_policy (CORBA::PolicyType type)
{
  throw ::CORBA::NO_IMPLEMENT();
}

CORBA::Policy_ptr
CORBA::Object::_get_cached_policy (TAO_Cached_Policy_Type type)
{
  throw ::CORBA::NO_IMPLEMENT();
}

CORBA::Object_ptr
CORBA::Object::_set_policy_overrides (
  const CORBA::PolicyList & policies,
  CORBA::SetOverrideType set_add)
{
  throw ::CORBA::NO_IMPLEMENT();
}

CORBA::PolicyList *
CORBA::Object::_get_policy_overrides (const CORBA::PolicyTypeSeq & types)
{
  throw ::CORBA::NO_IMPLEMENT();
}

CORBA::Boolean
CORBA::Object::_validate_connection (
  CORBA::PolicyList_out inconsistent_policies)
{
  return true;
}

#endif /* TAO_HAS_CORBA_MESSAGING == 1 */


CORBA::ORB_ptr
CORBA::Object::_get_orb (void)
{
  throw ::CORBA::INTERNAL();
}

TAO::Object_Proxy_Broker *
CORBA::Object::proxy_broker (void) const
{
  return 0;
}

/*****************************************************************
 * Global Functions
 ****************************************************************/

CORBA::Boolean
operator<< (TAO_OutputCDR& cdr, const CORBA::Object* x)
{
  return false;
}

/*static*/ void
CORBA::Object::tao_object_initialize (CORBA::Object *obj)
{
  return;
}

CORBA::Boolean
operator>> (TAO_InputCDR& cdr, CORBA::Object*& x)
{
  return false;
}

#if defined (GEN_OSTREAM_OPS)

std::ostream&
operator<< (std::ostream &strm, CORBA::Object_ptr _tao_objref)
{
  return strm;
}

#endif /* GEN_OSTREAM_OPS */

// =========================================================
// Traits specializations for CORBA::Object.
namespace TAO
{

  void In_Object_Argument_Cloner_T<CORBA::InterfaceDef_ptr>::duplicate
                                              (CORBA::InterfaceDef_ptr)
  {
  }

  void In_Object_Argument_Cloner_T<CORBA::InterfaceDef_ptr>::release
                                              (CORBA::InterfaceDef_ptr)
  {
  }

  CORBA::Object_ptr
  Objref_Traits<CORBA::Object>::duplicate (CORBA::Object_ptr p)
  {
    return CORBA::Object::_duplicate (p);
  }

  void
  Objref_Traits<CORBA::Object>::release (CORBA::Object_ptr p)
  {
    ::CORBA::release (p);
  }

  CORBA::Object_ptr
  Objref_Traits<CORBA::Object>::nil (void)
  {
    return CORBA::Object::_nil ();
  }

  CORBA::Boolean
  Objref_Traits<CORBA::Object>::marshal (const CORBA::Object_ptr p,
                                         TAO_OutputCDR & cdr)
  {
    return ::CORBA::Object::marshal (p, cdr);
  }
} // close TAO namespace


TAO::Object_Proxy_Broker * (*_TAO_Object_Proxy_Broker_Factory_function_pointer) (void) = 0;

