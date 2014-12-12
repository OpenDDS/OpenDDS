// @(#) $Id$
//
// Copyright 1994-1995 by Sun Microsystems Inc.
// Copyright 1997-2002 by Washington University
// All Rights Reserved
//
// ORB:         CORBA::Object operations

#include "tao/Object.h"
#include "tao/Stub.h"
#include "tao/Profile.h"
#include "tao/ORB_Core.h"
#include "tao/Connector_Registry.h"
#include "tao/LocateRequest_Invocation_Adapter.h"
#include "tao/debug.h"
#include "tao/Dynamic_Adapter.h"
#include "tao/IFR_Client_Adapter.h"
#include "tao/Remote_Object_Proxy_Broker.h"
#include "tao/CDR.h"
#include "tao/SystemException.h"
#include "tao/PolicyC.h"

#include "ace/Dynamic_Service.h"
#include "ace/OS_NS_string.h"
#include "ace/CORBA_macros.h"

#if !defined (__ACE_INLINE__)
# include "tao/Object.inl"
#endif /* ! __ACE_INLINE__ */

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

CORBA::Object::~Object (void)
{
  if (this->protocol_proxy_)
    (void) this->protocol_proxy_->_decr_refcnt ();
}

CORBA::Object::Object (TAO_Stub * protocol_proxy,
                       CORBA::Boolean collocated,
                       TAO_Abstract_ServantBase * servant,
                       TAO_ORB_Core *orb_core)
  : refcount_ (1)
    , is_local_ (false)
    , is_evaluated_ (true)
    , ior_ (0)
    , orb_core_ (orb_core)
    , protocol_proxy_ (protocol_proxy)
{
  /// This constructor should not be called when the protocol proxy is
  /// null ie. when the object is a LocalObject. Assert that
  /// requirement.
  ACE_ASSERT (this->protocol_proxy_ != 0);

  if (this->orb_core_ == 0)
    this->orb_core_ = this->protocol_proxy_->orb_core ();

  // Set the collocation marker on the stub. This may not be news to it.
  // This may also change the stub's object proxy broker.
  this->protocol_proxy_->is_collocated (collocated);

  // Set the collocated servant pointer (null if not collocated) on the stub.
  this->protocol_proxy_->collocated_servant (servant);
}

CORBA::Object::Object (IOP::IOR *ior,
                       TAO_ORB_Core *orb_core)
  : refcount_ (1)
    , is_local_ (false)
    , is_evaluated_ (false)
    , ior_ (ior)
    , orb_core_ (orb_core)
    , protocol_proxy_ (0)
{
}

// Too lazy to do this check in every method properly! This is useful
// only  for lazily evaluated IOR's
#define TAO_OBJECT_IOR_EVALUATE \
if (!this->is_evaluated_) \
  { \
    ACE_GUARD (TAO_SYNCH_MUTEX , mon, this->object_init_lock_); \
      if (!this->is_evaluated_) \
        CORBA::Object::tao_object_initialize (this); \
  }

#define TAO_OBJECT_IOR_EVALUATE_RETURN \
if (!this->is_evaluated_) \
  { \
    ACE_GUARD_RETURN (TAO_SYNCH_MUTEX , mon, this->object_init_lock_, 0); \
    if (!this->is_evaluated_) \
      CORBA::Object::tao_object_initialize (this); \
  }

void
CORBA::Object::_add_ref (void)
{
  ++this->refcount_;
}

void
CORBA::Object::_remove_ref (void)
{
  if (--this->refcount_ == 0)
    {
      delete this;
    }
}

CORBA::ULong
CORBA::Object::_refcount_value(void) const
{
  return static_cast<CORBA::ULong> (this->refcount_.value ());
}

void
CORBA::Object::_tao_any_destructor (void *x)
{
  CORBA::Object_ptr tmp = static_cast<CORBA::Object_ptr> (x);
  ::CORBA::release (tmp);
}

// virtual -- do not inline
CORBA::Boolean
CORBA::Object::marshal (TAO_OutputCDR &cdr)
{
  return (cdr << this);
}

/*static*/ CORBA::Boolean
CORBA::Object::marshal (const CORBA::Object_ptr x, TAO_OutputCDR &cdr)
{
  if (x == 0)
    {
      // NIL objrefs ... marshal as empty type hint, no elements.
      cdr.write_ulong (1);
      cdr.write_char ('\0');
      cdr.write_ulong (0);
      return (CORBA::Boolean) cdr.good_bit ();
    }

  return x->marshal (cdr);
}

#if defined (GEN_OSTREAM_OPS)

/*static*/ std::ostream &
CORBA::Object::_tao_stream (std::ostream &strm,
                            const CORBA::Object_ptr _tao_objref)
{
  return _tao_objref->_tao_stream_v (strm);
}

std::ostream &
CORBA::Object::_tao_stream_v (std::ostream &strm) const
{
  return strm << "\"IDL:omg.org/CORBA/Object:1.0\"";
}

#endif /* GEN_OSTREAM_OPS */

bool
CORBA::Object::can_convert_to_ior (void) const
{
  // By default, objects can not be stringified if they are local
  return !this->_is_local ();
}

char*
CORBA::Object::convert_to_ior (bool, const char*) const
{
  return 0;
}

TAO_Abstract_ServantBase*
CORBA::Object::_servant (void) const
{
  if (this->protocol_proxy_ == 0)
    {
      // No stub set. Should not happen.
      return 0;
    }

  return this->protocol_proxy_->collocated_servant ();
}

// IS_A ... ask the object if it's an instance of the type whose
// logical type ID is passed as a parameter.

CORBA::Boolean
CORBA::Object::_is_a (const char *type_id)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;

  // NOTE: if _stub->type_id is nonzero and we have local knowledge of
  // it, we can answer this question without a costly remote call.
  //
  // That "local knowledge" could come from stubs or skeletons linked
  // into this process in the best case, or a "near" repository in a
  // slightly worse case.  Or in a trivial case, if the ID being asked
  // about is the ID we have recorded, we don't need to ask about the
  // inheritance relationships at all!
  //
  // In real systems having local knowledge will be common, though as
  // the systems built atop ORBs become richer it'll also become
  // common to have the "real type ID" not be directly understood
  // because it's more deeply derived than any locally known types.
  //
  // XXX if type_id is that of CORBA::Object, "yes, we comply" :-)

  if (this->protocol_proxy_ == 0)
    {
      if (TAO_debug_level > 0)
        TAOLIB_ERROR ((LM_ERROR,
                    ACE_TEXT ("TAO (%P|%t) - No protocol proxy for %C\n"),
                    type_id));

      throw ::CORBA::NO_IMPLEMENT ();
    }

  if (this->_stubobj ()->type_id.in () != 0
      && ACE_OS::strcmp (type_id,
                         this->_stubobj ()->type_id.in ()) == 0)
    return true;

  return this->proxy_broker ()->_is_a (this, type_id);
}

const char*
CORBA::Object::_interface_repository_id (void) const
{
  return "IDL:omg.org/CORBA/Object:1.0";
}

CORBA::Boolean
CORBA::Object::_is_collocated (void) const
{
  if (this->protocol_proxy_)
    {
      return this->protocol_proxy_->is_collocated ();
    }

  return false;
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
  TAO_OBJECT_IOR_EVALUATE_RETURN;
  return this->protocol_proxy_;
}

CORBA::ULong
CORBA::Object::_hash (CORBA::ULong maximum)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;

  if (this->protocol_proxy_ != 0)
    return this->protocol_proxy_->hash (maximum);
  else
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

  TAO_OBJECT_IOR_EVALUATE_RETURN;

  if (this->protocol_proxy_ != 0)
    return this->protocol_proxy_->is_equivalent (other_obj);

  return false;
}

// TAO's extensions

TAO::ObjectKey *
CORBA::Object::_key (void)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;

  if (this->_stubobj () && this->_stubobj ()->profile_in_use ())
    return this->_stubobj ()->profile_in_use ()->_key ();

  if (TAO_debug_level > 2)
    {
      TAOLIB_ERROR ((LM_ERROR,
                  ACE_TEXT ("TAO (%P|%t) Null object key return from ")
                  ACE_TEXT ("profile in use\n")));
    }

  throw ::CORBA::INTERNAL (
    CORBA::SystemException::_tao_minor_code (
      0,
      EINVAL),
    CORBA::COMPLETED_NO);
}

void
CORBA::Object::_proxy_broker (TAO::Object_Proxy_Broker *proxy_broker)
{
  this->protocol_proxy_->object_proxy_broker (proxy_broker);
}

CORBA::Boolean
CORBA::Object::is_nil_i (CORBA::Object_ptr obj)
{

  // If the profile length is zero for a non-evaluted IOR it is a
  // null-object.
  if ((!obj->is_evaluated ()) && obj->ior ().profiles.length () == 0)
    return true;

  // To accomodate new definitions.
  if (obj->orb_core_)
    {
      return obj->orb_core_->object_is_nil (obj);
    }

  return false;
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
  TAO_OBJECT_IOR_EVALUATE;

  // Since we don't really support Context, anything but a null pointer
  // is a no-no.
  // Neither can we create a request object from locality constrained
  // object references.
  if (ctx != 0 || this->protocol_proxy_ == 0)
    {
      if (TAO_debug_level > 0)
        TAOLIB_ERROR ((LM_ERROR,
                    ACE_TEXT ("TAO (%P|%t) - No protocol proxy for %C\n"),
                    operation));

      throw ::CORBA::NO_IMPLEMENT ();
    }

  TAO_Dynamic_Adapter *dynamic_adapter =
    ACE_Dynamic_Service<TAO_Dynamic_Adapter>::instance (
        TAO_ORB_Core::dynamic_adapter_name ()
      );

  dynamic_adapter->create_request (
                       this,
                       this->protocol_proxy_->orb_core ()-> orb (),
                       operation,
                       arg_list,
                       result,
                       0,
                       request,
                       req_flags
                     );
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
  TAO_OBJECT_IOR_EVALUATE;

  // Since we don't really support Context, anything but a null pointer
  // is a no-no.
  // Neither can we create a request object from locality constrained
  // object references.
  if (ctx != 0 || this->protocol_proxy_ == 0)
    {
      if (TAO_debug_level > 0)
        TAOLIB_ERROR ((LM_ERROR,
                    ACE_TEXT ("TAO (%P|%t) - No protocol proxy for %C\n"),
                    operation));

      throw ::CORBA::NO_IMPLEMENT ();
    }

  TAO_Dynamic_Adapter *dynamic_adapter =
    ACE_Dynamic_Service<TAO_Dynamic_Adapter>::instance (
        TAO_ORB_Core::dynamic_adapter_name ()
      );

  dynamic_adapter->create_request (
                       this,
                       this->protocol_proxy_->orb_core ()-> orb (),
                       operation,
                       arg_list,
                       result,
                       exceptions,
                       request,
                       req_flags);
}
#endif

#if !defined (CORBA_E_COMPACT) && !defined (CORBA_E_MICRO)
CORBA::Request_ptr
CORBA::Object::_request (const char *operation)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;
  if (this->protocol_proxy_)
    {
      TAO_Dynamic_Adapter *dynamic_adapter =
        ACE_Dynamic_Service<TAO_Dynamic_Adapter>::instance (
            TAO_ORB_Core::dynamic_adapter_name ());

      return dynamic_adapter->request (
                                  this,
                                  this->protocol_proxy_->orb_core ()->orb (),
                                  operation);
    }
  else
    {
      if (TAO_debug_level > 0)
        TAOLIB_ERROR ((LM_ERROR,
                    ACE_TEXT ("TAO (%P|%t) - No protocol proxy for %C\n"),
                    operation));

      throw ::CORBA::NO_IMPLEMENT ();
    }
}
#endif

// NON_EXISTENT ... send a simple call to the object, which will
// either elicit a false response or a OBJECT_NOT_EXIST exception.  In
// the latter case, return true.

CORBA::Boolean
CORBA::Object::_non_existent (void)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;

  CORBA::Boolean retval = false;

  try
    {
      retval = this->proxy_broker ()->_non_existent (this);
    }
  catch (const ::CORBA::OBJECT_NOT_EXIST&)
    {
      retval = true;
    }

  return retval;
}


#if ! defined (CORBA_E_COMPACT) && ! defined (CORBA_E_MICRO)
CORBA::InterfaceDef_ptr
CORBA::Object::_get_interface (void)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;
  return this->proxy_broker ()->_get_interface (this);
}

CORBA::Object_ptr
CORBA::Object::_get_component (void)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;
  return this->proxy_broker ()->_get_component (this);
}
#endif

char*
CORBA::Object::_repository_id (void)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;
  return this->proxy_broker ()->_repository_id (this);
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
  TAO_OBJECT_IOR_EVALUATE_RETURN;

  if (this->protocol_proxy_)
    return this->protocol_proxy_->get_policy (type);
  else
    {
      if (TAO_debug_level > 0)
        TAOLIB_ERROR ((LM_ERROR,
                    ACE_TEXT ("TAO (%P|%t) - No protocol proxy in _get_policy\n")));

      throw ::CORBA::NO_IMPLEMENT ();
    }
}

CORBA::Policy_ptr
CORBA::Object::_get_cached_policy (TAO_Cached_Policy_Type type)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;

  if (this->protocol_proxy_)
    return this->protocol_proxy_->get_cached_policy (type);
  else
    {
      if (TAO_debug_level > 0)
        TAOLIB_ERROR ((LM_ERROR,
                    ACE_TEXT ("TAO (%P|%t) - No protocol proxy in _get_policy\n")));

      throw ::CORBA::NO_IMPLEMENT ();
    }
}

CORBA::Object_ptr
CORBA::Object::_set_policy_overrides (
  const CORBA::PolicyList & policies,
  CORBA::SetOverrideType set_add)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;

  if (!this->protocol_proxy_)
    {
      if (TAO_debug_level > 0)
        TAOLIB_ERROR ((LM_ERROR,
                    ACE_TEXT ("TAO (%P|%t) - No protocol proxy in _get_policy\n")));

      throw ::CORBA::NO_IMPLEMENT ();
    }

  TAO_Stub* stub =
    this->protocol_proxy_->set_policy_overrides (policies, set_add);

  TAO_Stub_Auto_Ptr safe_stub (stub);

  CORBA::Object_ptr obj = CORBA::Object::_nil ();

  ACE_NEW_THROW_EX (obj,
                    CORBA::Object (stub,
                                  this->_is_collocated ()),
                    CORBA::NO_MEMORY (
                      CORBA::SystemException::_tao_minor_code (
                        0,
                        ENOMEM),
                      CORBA::COMPLETED_MAYBE));

  // If the stub is collocated and we don't have a collocated server we need
  // to reinitialize it to get it.
  if (stub->is_collocated () && stub->collocated_servant () == 0)
    {
      obj->orb_core ()->reinitialize_object (stub);
    }

  (void) safe_stub.release ();

  return obj;
}

CORBA::PolicyList *
CORBA::Object::_get_policy_overrides (const CORBA::PolicyTypeSeq & types)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;
  if (this->protocol_proxy_)
    return this->protocol_proxy_->get_policy_overrides (types);
  else
    {
      if (TAO_debug_level > 0)
        TAOLIB_ERROR ((LM_ERROR,
                    ACE_TEXT ("TAO (%P|%t) - No protocol proxy in _get_policy\n")));

      throw ::CORBA::NO_IMPLEMENT ();
    }
}

CORBA::Boolean
CORBA::Object::_validate_connection (
  CORBA::PolicyList_out inconsistent_policies)
{
  TAO_OBJECT_IOR_EVALUATE_RETURN;

  inconsistent_policies = 0;
  CORBA::Boolean retval = true;

#if (TAO_HAS_MINIMUM_CORBA == 0)
  // Note that the OBJECT_NOT_EXIST exception should be propagated to
  // the caller rather than return false, which is why we do not use
  // CORBA::Object::_non_existent().  This behavior is consistent
  // with the non-collocated case.
  if (this->_is_collocated ())
      return !(this->proxy_broker ()->_non_existent (this));

  TAO::LocateRequest_Invocation_Adapter tao_call (this);
  try
    {
      tao_call.invoke ();
    }
  catch (const ::CORBA::INV_POLICY&)
    {
      inconsistent_policies = tao_call.get_inconsistent_policies ();
      retval = false;
    }
#else
  retval = false;
#endif /* TAO_HAS_MINIMUM_CORBA */

  return retval;
}

#endif /* TAO_HAS_CORBA_MESSAGING == 1 */


CORBA::ORB_ptr
CORBA::Object::_get_orb (void)
{
  if (this->orb_core_ != 0)
    {
      return CORBA::ORB::_duplicate (this->orb_core_->orb ());
    }
  else
    {
      TAO_OBJECT_IOR_EVALUATE_RETURN;
      if (this->protocol_proxy_)
        return CORBA::ORB::_duplicate (this->protocol_proxy_->orb_core ()->orb ());
      else
        {
          if (TAO_debug_level > 0)
            TAOLIB_ERROR ((LM_ERROR,
                        ACE_TEXT ("TAO (%P|%t) - No protocol proxy in _get_policy\n")));

          throw ::CORBA::NO_IMPLEMENT ();
        }
    }
}

TAO::Object_Proxy_Broker *
CORBA::Object::proxy_broker (void) const
{
  // Paranoid check. We *should* never access the proxy_broker
  // when the object has not been initialised so there *should*
  // alway be a stub, but just in case...

  if (this->protocol_proxy_)
    {
      return this->protocol_proxy_->object_proxy_broker ();
    }

  // We have no stub. We cannot be collocated.
  return the_tao_remote_object_proxy_broker ();
}

/*****************************************************************
 * Global Functions
 ****************************************************************/

CORBA::Boolean
operator<< (TAO_OutputCDR& cdr, const CORBA::Object* x)
{
  if (x == 0)
    {
      // NIL objrefs ... marshal as empty type hint, no elements.
      cdr.write_ulong (1);
      cdr.write_char ('\0');
      cdr.write_ulong (0);
      return (CORBA::Boolean) cdr.good_bit ();
    }

  if (!x->is_evaluated ())
    {
      // @@ This is too inefficient. Need to speed this up if this is
      // a bottle neck.
      cdr << const_cast<IOP::IOR &> (x->ior ());
      return cdr.good_bit ();
    }

   TAO_Stub *stubobj = x->_stubobj ();

   if (stubobj == 0)
     return false;

  return (stubobj->marshal (cdr));
}

/*static*/ void
CORBA::Object::tao_object_initialize (CORBA::Object *obj)
{
  CORBA::ULong const profile_count =
    obj->ior_->profiles.length ();

  // Assumption is that after calling this method, folks should test
  // for protocol_proxy_ or whatever to make sure that things have
  // been initialized!
  if (profile_count == 0)
    return;

  // get a profile container to store all profiles in the IOR.
  TAO_MProfile mp (profile_count);

  TAO_ORB_Core *&orb_core = obj->orb_core_;
  if (orb_core == 0)
    {
      orb_core = TAO_ORB_Core_instance ();
      if (TAO_debug_level > 0)
        {
          TAOLIB_DEBUG ((LM_WARNING,
                      ACE_TEXT ("TAO (%P|%t) - Object::tao_object_initialize ")
                      ACE_TEXT ("WARNING: extracting object from ")
                      ACE_TEXT ("default ORB_Core\n")));
        }
    }

  TAO_Stub *objdata = 0;

  try
    {
      TAO_Connector_Registry *connector_registry =
        orb_core->connector_registry ();

      for (CORBA::ULong i = 0; i != profile_count; ++i)
        {
          IOP::TaggedProfile &tpfile = obj->ior_->profiles[i];

          // NOTE: This is a place for optimizations. Here we have an
          // 2 allocations and 2 copies. Future optimizations should
          // target this place.
          TAO_OutputCDR o_cdr;

          o_cdr << tpfile;

          TAO_InputCDR cdr (o_cdr,
                            orb_core->input_cdr_buffer_allocator (),
                            orb_core->input_cdr_dblock_allocator (),
                            orb_core->input_cdr_msgblock_allocator (),
                            orb_core);

          TAO_Profile *pfile = connector_registry->create_profile (cdr);

          if (pfile != 0)
            {
              if (mp.give_profile (pfile) == -1)
              {
                TAOLIB_ERROR ((LM_ERROR,
                            ACE_TEXT ("TAO (%P|%t) ERROR: give_profile\n")
                            ACE_TEXT (" returned -1\n")));
              }
            }
        }

      // Make sure we got some profiles!
      if (mp.profile_count () != profile_count)
        {
          // @@ This occurs when profile creation fails when decoding the
          //    profile from the IOR.
          TAOLIB_ERROR ((LM_ERROR,
                      ACE_TEXT ("TAO (%P|%t) ERROR: XXXXX Could not create all ")
                      ACE_TEXT ("profiles while extracting object\n")
                      ACE_TEXT ("TAO (%P|%t) ERROR: reference from the ")
                      ACE_TEXT ("CDR stream.\n")));
        }


      objdata = orb_core->create_stub (obj->ior_->type_id.in (), mp);
    }
  catch (const ::CORBA::Exception& ex)
    {
      if (TAO_debug_level > 0)
        ex._tao_print_exception (
          ACE_TEXT ("TAO - ERROR creating stub ")
          ACE_TEXT ("object when demarshaling object ")
          ACE_TEXT ("reference."));

      return;
    }

  TAO_Stub_Auto_Ptr safe_objdata (objdata);

  // This call will set the stub proxy broker if necessary
  if (orb_core->initialize_object (safe_objdata.get (), obj) == -1)
    return;

  obj->protocol_proxy_ = objdata;

  obj->is_evaluated_ = true;

  // Release the contents of the ior to keep memory consumption down.
  obj->ior_ = 0;

  // Transfer ownership to the CORBA::Object
  (void) safe_objdata.release ();
  return;
}

CORBA::Boolean
operator>> (TAO_InputCDR& cdr, CORBA::Object*& x)
{
  bool lazy_strategy = false;
  TAO_ORB_Core *orb_core = cdr.orb_core ();

  if (orb_core == 0)
    {
      orb_core = TAO_ORB_Core_instance ();
      if (TAO_debug_level > 0)
        {
          TAOLIB_DEBUG ((LM_WARNING,
                      ACE_TEXT ("TAO (%P|%t) WARNING: extracting object from ")
                      ACE_TEXT ("default ORB_Core\n")));
        }
    }
  else
    {
      if (orb_core->resource_factory ()->resource_usage_strategy () ==
          TAO_Resource_Factory::TAO_LAZY)
        {
          lazy_strategy = true;
        }
    }

  if (!lazy_strategy)
    {
      // If the user has set up a eager strategy..
      CORBA::String_var type_hint;

      if (!(cdr >> type_hint.inout ()))
        return false;

      CORBA::ULong profile_count;
      if (!(cdr >> profile_count))
        return false;

      if (profile_count == 0)
        {
          x = CORBA::Object::_nil ();
          return (CORBA::Boolean) cdr.good_bit ();
        }

      // get a profile container to store all profiles in the IOR.
      TAO_MProfile mp (profile_count);

      TAO_ORB_Core *orb_core = cdr.orb_core ();
      if (orb_core == 0)
        {
          orb_core = TAO_ORB_Core_instance ();
          if (TAO_debug_level > 0)
            {
              TAOLIB_DEBUG ((LM_WARNING,
                          ACE_TEXT ("TAO (%P|%t) - Object::tao_object_initialize ")
                          ACE_TEXT ("WARNING: extracting object from ")
                          ACE_TEXT ("default ORB_Core\n")));
            }
        }

      // Ownership of type_hint is given to TAO_Stub
      // TAO_Stub will make a copy of mp!

      TAO_Stub *objdata = 0;

      try
        {
          TAO_Connector_Registry *connector_registry =
            orb_core->connector_registry ();

          for (CORBA::ULong i = 0; i != profile_count && cdr.good_bit (); ++i)
            {
              TAO_Profile *pfile = connector_registry->create_profile (cdr);
              if (pfile != 0)
                {
                  if (mp.give_profile (pfile) == -1)
                    {
                      TAOLIB_ERROR ((LM_ERROR,
                                  ACE_TEXT ("TAO (%P|%t) ERROR: give_profile\n")
                                  ACE_TEXT (" returned -1\n")));
                    }
                }
            }

          // Make sure we got some profiles!
          if (mp.profile_count () != profile_count)
            {
              // @@ This occurs when profile creation fails when decoding the
              //    profile from the IOR.
              TAOLIB_ERROR_RETURN ((LM_ERROR,
                                 ACE_TEXT ("TAO (%P|%t) - ERROR: Could not create all ")
                                 ACE_TEXT ("profiles while extracting object [%d, %d]\n")
                                 ACE_TEXT ("TAO (%P|%t) - ERROR: reference from the ")
                                 ACE_TEXT ("CDR stream.\n"),
                                 mp.profile_count (), profile_count),
                                false);
            }

          objdata = orb_core->create_stub (type_hint.in (), mp);
        }
      catch (const ::CORBA::Exception& ex)
        {
          if (TAO_debug_level > 0)
            ex._tao_print_exception (
              ACE_TEXT ("TAO (%P|%t) - ERROR creating stub ")
              ACE_TEXT ("object when demarshaling object ")
              ACE_TEXT ("reference.\n"));

          return false;
        }

      TAO_Stub_Auto_Ptr safe_objdata (objdata);

      x = orb_core->create_object (safe_objdata.get ());
      if (!x)
        {
          return false;
        }

      // Transfer ownership to the CORBA::Object
      (void) safe_objdata.release ();
    }
  else
    {
      // Lazy strategy!
      IOP::IOR *ior = 0;

      ACE_NEW_RETURN (ior,
                      IOP::IOR (),
                      false);

      if (!(cdr >> *ior))
        {
          return false;
        }

      ACE_NEW_RETURN (x,
                      CORBA::Object (ior, orb_core),
                      false);
    }

  return (CORBA::Boolean) cdr.good_bit ();
}

#if defined (GEN_OSTREAM_OPS)

std::ostream&
operator<< (std::ostream &strm, CORBA::Object_ptr _tao_objref)
{
  return CORBA::Object::_tao_stream (strm, _tao_objref);
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


TAO_END_VERSIONED_NAMESPACE_DECL
