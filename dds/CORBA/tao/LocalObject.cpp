// $Id$

#include "tao/LocalObject.h"

#if !defined (__ACE_INLINE__)
# include "tao/LocalObject.inl"
#endif /* ! __ACE_INLINE__ */

#include "tao/SystemException.h"
#include "tao/debug.h"
#include "tao/ORB_Constants.h"

#include "ace/Log_Msg.h"
#include "ace/Guard_T.h"

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

CORBA::LocalObject::~LocalObject (void)
{
}

// Quickly hash an object reference's representation data.  Used to
// create hash tables.

CORBA::ULong
CORBA::LocalObject::_hash (CORBA::ULong maximum)
{
  // Note that we reinterpret_cast to an "ptrdiff_t" instead of
  // CORBA::ULong since we need to first cast to an integer large
  // enough to hold an address to avoid compile-time warnings on some
  // 64-bit platforms.

  CORBA::ULong const hash =
    static_cast<CORBA::ULong> (reinterpret_cast<ptrdiff_t> (this));

  return hash % maximum;
}

// Compare two object references to see if they point to the same
// object.  Used in linear searches, as in hash buckets.
//
// XXX would be useful to also have a trivalued comparison predicate,
// such as strcmp(), to allow more comparison algorithms.

CORBA::Boolean
CORBA::LocalObject::_is_equivalent (CORBA::Object_ptr other_obj)
{
  return (other_obj == this) ? true : false;
}

// TAO's extensions


TAO::ObjectKey *
CORBA::LocalObject::_key (void)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot get _key from a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT ();
}

#if (TAO_HAS_MINIMUM_CORBA == 0)

// NON_EXISTENT ... send a simple call to the object, which will
// either elicit a FALSE response or a OBJECT_NOT_EXIST exception.  In
// the latter case, return FALSE.

CORBA::Boolean
CORBA::LocalObject::_non_existent (void)
{
  // Always return false.
  return false;
}

char *
CORBA::LocalObject::_repository_id (void)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot get _repository_id from a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 8, CORBA::COMPLETED_NO);
}

#if ! defined (CORBA_E_COMPACT) && ! defined (CORBA_E_MICRO)
void
CORBA::LocalObject::_create_request (CORBA::Context_ptr,
                                     const char *,
                                     CORBA::NVList_ptr,
                                     CORBA::NamedValue_ptr,
                                     CORBA::Request_ptr &,
                                     CORBA::Flags)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _create_request for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 4, CORBA::COMPLETED_NO);
}

void
CORBA::LocalObject::_create_request (CORBA::Context_ptr,
                                     const char *,
                                     CORBA::NVList_ptr,
                                     CORBA::NamedValue_ptr,
                                     CORBA::ExceptionList_ptr,
                                     CORBA::ContextList_ptr,
                                     CORBA::Request_ptr &,
                                     CORBA::Flags)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _create_request for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 4, CORBA::COMPLETED_NO);
}

CORBA::Request_ptr
CORBA::LocalObject::_request (const char *)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _request for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 4, CORBA::COMPLETED_NO);
}

CORBA::Object_ptr
CORBA::LocalObject::_get_component (void)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _get_component for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 8, CORBA::COMPLETED_NO);
}

CORBA::InterfaceDef_ptr
CORBA::LocalObject::_get_interface (void)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _get_interface for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 8, CORBA::COMPLETED_NO);
}
#endif

#endif /* TAO_HAS_MINIMUM_CORBA */

#if (TAO_HAS_CORBA_MESSAGING == 1)

CORBA::Policy_ptr
CORBA::LocalObject::_get_policy (CORBA::PolicyType)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _get_policy for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 8, CORBA::COMPLETED_NO);
}

CORBA::Policy_ptr
CORBA::LocalObject::_get_cached_policy (TAO_Cached_Policy_Type)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _get_cached_policy for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT ();
}

CORBA::Object_ptr
CORBA::LocalObject::_set_policy_overrides (const CORBA::PolicyList &,
                                           CORBA::SetOverrideType)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _set_policy_overrides for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 8, CORBA::COMPLETED_NO);
}

CORBA::PolicyList *
CORBA::LocalObject::_get_policy_overrides (const CORBA::PolicyTypeSeq &)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _get_policy_overrides for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 8, CORBA::COMPLETED_NO);
}

CORBA::Boolean
CORBA::LocalObject::_validate_connection (CORBA::PolicyList_out)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _validate_connection for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 8, CORBA::COMPLETED_NO);
}

#endif /* TAO_HAS_CORBA_MESSAGING == 1 */

CORBA::ORB_ptr
CORBA::LocalObject::_get_orb (void)
{
  if (TAO_debug_level > 0)
    TAOLIB_ERROR ((LM_ERROR,
                ACE_TEXT ("TAO (%P|%t) - Cannot call _get_orb for a LocalObject!\n")));

  throw ::CORBA::NO_IMPLEMENT (CORBA::OMGVMCID | 8, CORBA::COMPLETED_NO);
}

TAO_END_VERSIONED_NAMESPACE_DECL
