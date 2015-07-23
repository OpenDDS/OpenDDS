#include "tao/SystemException.h"
#include "tao/ORB_Constants.h"
#include "tao/CORBA_String.h"
#include "tao/debug.h"
#include "tao/Allocation_Macros.h"

#include "ace/Malloc.h"
#include "ace/SString.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_stdio.h"

#if !defined (__ACE_INLINE__)
# include "tao/SystemException.inl"
#endif /* __ACE_INLINE__ */

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

/**
 * @name @c errno Encoding
 *
 * The @c errno encoding is located in the bottom 7 bits.
 */
//@{
const CORBA::ULong TAO_UNSPECIFIED_MINOR_CODE        = 0x0U;
const CORBA::ULong TAO_ETIMEDOUT_MINOR_CODE          = 0x1U;
const CORBA::ULong TAO_ENFILE_MINOR_CODE             = 0x2U;
const CORBA::ULong TAO_EMFILE_MINOR_CODE             = 0x3U;
const CORBA::ULong TAO_EPIPE_MINOR_CODE              = 0x4U;
const CORBA::ULong TAO_ECONNREFUSED_MINOR_CODE       = 0x5U;
const CORBA::ULong TAO_ENOENT_MINOR_CODE             = 0x6U;
const CORBA::ULong TAO_EBADF_MINOR_CODE              = 0x7U;
const CORBA::ULong TAO_ENOSYS_MINOR_CODE             = 0x8U;
const CORBA::ULong TAO_EPERM_MINOR_CODE              = 0x9U;
const CORBA::ULong TAO_EAFNOSUPPORT_MINOR_CODE       = 0xAU;
const CORBA::ULong TAO_EAGAIN_MINOR_CODE             = 0xBU;
const CORBA::ULong TAO_ENOMEM_MINOR_CODE             = 0xCU;
const CORBA::ULong TAO_EACCES_MINOR_CODE             = 0xDU;
const CORBA::ULong TAO_EFAULT_MINOR_CODE             = 0xEU;
const CORBA::ULong TAO_EBUSY_MINOR_CODE              = 0xFU;
const CORBA::ULong TAO_EEXIST_MINOR_CODE             = 0x10U;
const CORBA::ULong TAO_EINVAL_MINOR_CODE             = 0x11U;
const CORBA::ULong TAO_ECOMM_MINOR_CODE              = 0x12U;
const CORBA::ULong TAO_ECONNRESET_MINOR_CODE         = 0x13U;
const CORBA::ULong TAO_ENOTSUP_MINOR_CODE            = 0x14U;
// *Don't* use TAO_<errno>_MINOR_CODE greater than 0x7FU!
//@}

// ****************************************************************

CORBA::SystemException::SystemException (void)
  : minor_ (0),
    completed_ (CORBA::COMPLETED_NO)
{
}

CORBA::SystemException::SystemException (const char *repository_id,
                                         const char *local_name,
                                         CORBA::ULong code,
                                         CORBA::CompletionStatus completed)
  : CORBA::Exception (repository_id,
                      local_name),
    minor_ (code),
    completed_ (completed)
{
}

CORBA::SystemException::SystemException (CORBA::ULong code,
                                         CORBA::CompletionStatus completed)
  : minor_ (code),
    completed_ (completed)
{
}

CORBA::SystemException::SystemException (const CORBA::SystemException &src)
  : CORBA::Exception (src),
    minor_ (src.minor_),
    completed_ (src.completed_)
{
}

CORBA::SystemException::~SystemException (void)
{
}

CORBA::SystemException &
CORBA::SystemException::operator= (const CORBA::SystemException &src)
{
  this->Exception::operator= (src);

  this->minor_ = src.minor_;
  this->completed_ = src.completed_;

  return *this;
}

void
CORBA::SystemException::_tao_encode(TAO_OutputCDR&) const
{
  throw ::CORBA::MARSHAL ();
}

void
CORBA::SystemException::_tao_decode(TAO_InputCDR&)
{
  throw ::CORBA::MARSHAL ();
}

CORBA::ULong
CORBA::SystemException::_tao_errno (int errno_value)
{
  switch (errno_value)
    {
    case 0:
      return TAO_UNSPECIFIED_MINOR_CODE;
    case ETIMEDOUT:
      return TAO_ETIMEDOUT_MINOR_CODE;
    case ENFILE:
      return TAO_ENFILE_MINOR_CODE;
    case EPIPE:
      return TAO_EPIPE_MINOR_CODE;
    case ECONNREFUSED:
      return TAO_ECONNREFUSED_MINOR_CODE;
    case ENOENT:
      return TAO_ENOENT_MINOR_CODE;

#if !defined (ACE_HAS_WINCE)
    case EMFILE:
      return TAO_EMFILE_MINOR_CODE;
    case EBADF:
      return TAO_EBADF_MINOR_CODE;
    case EPERM:
      return TAO_EPERM_MINOR_CODE;
    case EINVAL:
      return TAO_EINVAL_MINOR_CODE;
#endif  // ACE_HAS_WINCE

#if (ENOSYS != EFAULT)
    case ENOSYS:
      return TAO_ENOSYS_MINOR_CODE;
#endif /* ENOSYS != EFAULT */
    case EAFNOSUPPORT:
      return TAO_EAFNOSUPPORT_MINOR_CODE;
    case EAGAIN:
      return TAO_EAGAIN_MINOR_CODE;
    case ENOMEM:
      return TAO_ENOMEM_MINOR_CODE;
    case EACCES:
      return TAO_EACCES_MINOR_CODE;
    case EFAULT:
      return TAO_EFAULT_MINOR_CODE;
    case EBUSY:
      return TAO_EBUSY_MINOR_CODE;
    case EEXIST:
      return TAO_EEXIST_MINOR_CODE;
    case ECOMM:
      return TAO_ECOMM_MINOR_CODE;
    case ECONNRESET:
      return TAO_ECONNRESET_MINOR_CODE;
#if (ENOTSUP != ENOSYS)
    case ENOTSUP:
      return TAO_ENOTSUP_MINOR_CODE;
#endif /* ENOSYS != EFAULT */
    default:
      // Mask off bottom 7 bits and return them.
      return errno_value & 0x7FU;
    }
}

CORBA::Exception *
CORBA::SystemException::_tao_duplicate (void) const
{
  return 0;
}

CORBA::ULong
CORBA::SystemException::_tao_minor_code (u_int location, int errno_value)
{
  return
    TAO::VMCID
    | location
    | _tao_errno (errno_value);
}

void
CORBA::SystemException::_tao_print_system_exception (FILE *) const
{
  ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) system exception, ID '%C'\n"),
              this->_info ().c_str ()));
}

ACE_CString
CORBA::SystemException::_info (void) const
{
  // @@ there are a few other "user exceptions" in the CORBA scope,
  // they're not all standard/system exceptions ... really need to
  // either compare exhaustively against all those IDs (yeech) or
  // (preferably) to represent the exception type directly in the
  // exception value so it can be queried.

  ACE_CString info = "system exception, ID '";
  info += this->_rep_id ();
  info += "'\n";

  CORBA::ULong const VMCID = this->minor () & 0xFFFFF000u;

  if (VMCID == TAO::VMCID)
    {
      // @@ Move the following code to a subroutine, it is too long already!
      const char *location = 0;
      switch (this->minor () & 0x00000F80u)
        {
        case TAO_INVOCATION_LOCATION_FORWARD_MINOR_CODE:
          location = "location forward failed";
          break;
        case TAO_INVOCATION_SEND_REQUEST_MINOR_CODE:
          location = "send request failed";
          break;
        case TAO_POA_DISCARDING:
          location = "poa in discarding state";
          break;
        case TAO_POA_HOLDING:
          location = "poa in holding state";
          break;
        case TAO_POA_INACTIVE:
          location = "poa in inactive state";
          break;
        case TAO_UNHANDLED_SERVER_CXX_EXCEPTION:
          location = "unhandled c++ exception in server side";
          break;
        case TAO_INVOCATION_RECV_REQUEST_MINOR_CODE:
          location = "failed to recv request response";
          break;
        case TAO_CONNECTOR_REGISTRY_NO_USABLE_PROTOCOL:
          location = "all protocols failed to parse the IOR";
          break;
        case TAO_MPROFILE_CREATION_ERROR:
          location = "error during MProfile creation";
          break;
        case TAO_TIMEOUT_CONNECT_MINOR_CODE:
          location = "timeout during connect";
          break;
        case TAO_TIMEOUT_SEND_MINOR_CODE:
          location = "timeout during send";
          break;
        case TAO_TIMEOUT_RECV_MINOR_CODE:
          location = "timeout during recv";
          break;
        case TAO_IMPLREPO_MINOR_CODE:
          location = "implrepo server exception";
          break;
        case TAO_ACCEPTOR_REGISTRY_OPEN_LOCATION_CODE:
          location = "endpoint initialization failure in Acceptor Registry";
          break;
        case TAO_ORB_CORE_INIT_LOCATION_CODE:
          location = "ORB Core initialization failed";
          break;
        case TAO_POLICY_NARROW_CODE:
          location = "Failure when narrowing a Policy";
          break;
        case TAO_GUARD_FAILURE:
          location = "Failure when trying to acquire a guard/monitor";
          break;
        case TAO_POA_BEING_DESTROYED:
          location = "POA has been destroyed or is currently being destroyed";
          break;
        case TAO_AMH_REPLY_LOCATION_CODE:
          location = "Failure when trying to send AMH reply";
          break;
        case TAO_RTCORBA_THREAD_CREATION_LOCATION_CODE:
          location = "Failure in thread creation for RTCORBA thread pool";
          break;
        default:
          location = "unknown location";
        }

      const char *errno_indication;
      char unknown_errno [255];
      CORBA::ULong minor_code = this->minor () & 0x7FU;
      switch (minor_code)
        {
        case TAO_UNSPECIFIED_MINOR_CODE:
          errno_indication = "unspecified errno";
          break;
        case TAO_ETIMEDOUT_MINOR_CODE:
          errno_indication = "ETIMEOUT";
          break;
        case TAO_ENFILE_MINOR_CODE:
          errno_indication = "ENFILE";
          break;
        case TAO_EMFILE_MINOR_CODE:
          errno_indication = "EMFILE";
          break;
        case TAO_EPIPE_MINOR_CODE:
          errno_indication = "EPIPE";
          break;
        case TAO_ECONNREFUSED_MINOR_CODE:
          errno_indication = "ECONNREFUSED";
          break;
        case TAO_ENOENT_MINOR_CODE:
          errno_indication = "ENOENT";
          break;
        case TAO_EBADF_MINOR_CODE:
          errno_indication = "EBADF";
          break;
        case TAO_ENOSYS_MINOR_CODE:
          errno_indication = "ENOSYS";
          break;
        case TAO_EPERM_MINOR_CODE:
          errno_indication = "EPERM";
          break;
        case TAO_EAFNOSUPPORT_MINOR_CODE:
          errno_indication = "EAFNOSUPPORT";
          break;
        case TAO_EAGAIN_MINOR_CODE:
          errno_indication = "EAGAIN";
          break;
        case TAO_ENOMEM_MINOR_CODE:
          errno_indication = "ENOMEM";
          break;
        case TAO_EACCES_MINOR_CODE:
          errno_indication = "EACCES";
          break;
        case TAO_EFAULT_MINOR_CODE:
          errno_indication = "EFAULT";
          break;
        case TAO_EBUSY_MINOR_CODE:
          errno_indication = "EBUSY";
          break;
        case TAO_EEXIST_MINOR_CODE:
          errno_indication = "EEXIST";
          break;
        case TAO_EINVAL_MINOR_CODE:
          errno_indication = "EINVAL";
          break;
        case TAO_ECOMM_MINOR_CODE:
          errno_indication = "ECOMM";
          break;
        case TAO_ECONNRESET_MINOR_CODE:
          errno_indication = "ECONNRESET";
          break;
        case TAO_ENOTSUP_MINOR_CODE:
          errno_indication = "ENOTSUP";
          break;
        default:
          {
            // 7 bits of some other errno.
            ACE_OS::snprintf (unknown_errno,
                             255,
                             "low 7 bits of errno: %3u %s",
                             minor_code, ACE_OS::strerror (minor_code));

            errno_indication = unknown_errno;
          }
        }

      char buffer[BUFSIZ];
      ACE_OS::snprintf (buffer,
                       BUFSIZ,
                       "TAO exception, "
                       "minor code = %x (%s; %s), "
                       "completed = %s\n",
                       minor_code,
                       location,
                       errno_indication,
                       (completed () == CORBA::COMPLETED_YES) ? "YES" :
                       (completed () == CORBA::COMPLETED_NO) ? "NO" :
                       (completed () == CORBA::COMPLETED_MAYBE) ? "MAYBE" :
                       "garbage");

      info += buffer;
    }
  else if (VMCID == CORBA::OMGVMCID)
    {
      CORBA::ULong const minor_code = this->minor () & 0xFFFU;

      const char *minor_description = 0;

      if (minor_code > 0)
          minor_description =
            CORBA::SystemException::_tao_get_omg_exception_description (
              *this,
              minor_code);
      else
        minor_description = "*unknown description*";

      char buffer[BUFSIZ];
      ACE_OS::snprintf (buffer,
                       BUFSIZ,
                       "OMG minor code (%d), "
                       "described as '%s', "
                       "completed = %s\n",
                       minor_code,
                       minor_description,
                       (completed () == CORBA::COMPLETED_YES) ? "YES" :
                       (completed () == CORBA::COMPLETED_NO) ? "NO" :
                       (completed () == CORBA::COMPLETED_MAYBE) ? "MAYBE" :
                       "garbage");

      info += buffer;
    }
  else
    {
      char buffer[BUFSIZ];
      ACE_OS::snprintf (buffer,
                       BUFSIZ,
                       "Unknown vendor minor code id (%x), "
                       "minor code = %x, completed = %s\n",
                       VMCID,
                       this->minor (),  // Use the raw minor code
                       (completed () == CORBA::COMPLETED_YES) ? "YES" :
                       (completed () == CORBA::COMPLETED_NO) ? "NO" :
                       (completed () == CORBA::COMPLETED_MAYBE) ? "MAYBE" :
                       "garbage");

      info += buffer;
    }

  return info;
}

const char *
CORBA::SystemException::_tao_get_omg_exception_description (
  const CORBA::SystemException &exc,
  CORBA::ULong minor_code)
{
#ifndef ACE_NDEBUG

  static const char *UNKNOWN_TABLE[] =
    {
      "Unlisted user exception received by client.",    // 1
      "Non-standard SystemException not supported.",    // 2
      "An unknown user exception received by a portable interceptor." // 3
    };

  static const char *BAD_PARAM_TABLE[] =
    {
      "Failure to register, unregister, or lookup value factory.", // 1
      "RID already defined in IFR.",                               // 2
      "Name already used in the context in IFR.",                  // 3
      "Target is not a valid container.",                          // 4
      "Name clash in inherited context.",                          // 5
      "Incorrect type for abstract interface.",                    // 6
      "string_to_object conversion failed due to a bad scheme name.", // 7
      "string_to_object conversion failed due to a bad address.",  // 8
      "string_to_object conversion failed due to a bad schema specific part.",// 9
      "string_to_object conversion failed due to non specific reason.", // 10
      "Attempt to derive abstract interface from non-abstract base interface in the Interface Repository.", // 11
      "Attempt to let a ValueDef support more than one non-abstract interface in the Interface Repository.", // 12
      "Attempt to use an incomplete TypeCode as a parameter.",     // 13
      "Invalid object id passed to POA::create_reference_by_id.",  // 14
      "Bad name argument in TypeCode operation.",                  // 15
      "Bad RepositoryId argument in TypeCode operation.",          // 16
      "Invalid member name in TypeCode operation.",                 // 17
      "Duplicate label value in create_union_tc.",                 // 18
      "Incompatible TypeCode of label and discriminator in create_union_tc.", // 19
      "Supplied discriminator type illegitimate in create_union_tc.", // 20
      "Any passed to ServerRequest::set_exception does not contain an exception.", // 21
      "Unlisted user exception passed to ServerRequest::set_exception", // 22
      "wchar transmission code set not in service context.",       // 23
      "Service context is not in OMG-defined range.",              // 24
      "Enum value out of range.",                                  // 25
      "Invalid service context Id in portable interceptor.",       // 26
      "Attempt to call register_initial_reference with a null Object.", // 27
      "Invalid component Id in portable interceptor.",             // 28
      "Invalid profile Id in portable interceptor.",               // 29
      "Two or more Policy objects with the same PolicyType value supplied to Object::set_policy_overrides or PolicyManager::set_policy_overrides." // 30
      "Attempt to define a oneway operation with non-void result, out or inout parameters or user exceptions.", // 31
      "DII asked to create request for an implicit operation.",     // 32,
      "An OTS/XA integration xa_ call returned XAER_INVAL.",        // 33
      "Union branch modifier called with bad case label discriminator.", // 34
      "Illegal IDL context property name.",   // 35
      "Illegal IDL property search string.",  // 36
      "Illegal IDL context name.",            // 37
      "Non-empty IDL context.",               // 38
      "Unsupported RMI/IDL customer value type stream format.",  // 39
      "ORB output stream does not support ValueOutputStream interface.", // 40
      "ORB input stream does not support ValueInputStream interface.",    // 41
      "Character support limited to ISO 8859-1 for this object reference", // 42
      "Attempt to add a Pollable to a second PollableSet." // 43
    };

  static const char *IMP_LIMIT_TABLE[] =
    {
      "Unable to use any profile in IOR." // 1
    };

  static const char *INITIALIZE_TABLE[] =
    {
      "Priority range too restricted for ORB." // 1
    };


  static const char *INV_OBJREF_TABLE[] =
    {
      "wchar Code Set support not specified.", // 1
      "Codeset component required for type using wchar or wstring data." // 2
    };

  static const char *MARSHAL_TABLE[] =
    {
      "Unable to locate value factory.",  // 1
      "ServerRequest::set_result called before ServerRequest::ctx when the operation IDL contains a context clause.", // 2
      "NVList passed to ServerRequest::arguments does not describe all parameters passed by client.", // 3
      "Attempt to marshal Local object.", // 4
      "wchar or wstring data erroneously sent by client over GIOP 1.0 connection.", // 5
      "wchar or wstring data erroneously returned by server over GIOP 1.0 connection.", //6
      "Unsupported RMI/IDL custom value type stream format.", // 7
      "Custom data not compatible with ValueHandler read operation.", // 8
      "Codeset service contexts with different values recieved on the same connection." // 9

    };

  static const char *BAD_TYPECODE_TABLE[] =
    {
      "Attempt to marshal incomplete TypeCode.",              // 1
      "Member type code illegitimate in TypeCode operation.", // 2
      "Illegal parameter type."                               // 3
    };

  static const char *NO_IMPLEMENT_TABLE[] =
    {
      "Missing local value implementation.",        // 1
      "Incompatible value implementation version.", // 2
      "Unable to use any profile in IOR.",          // 3
      "Attempt to use DII on Local object.",        // 4
      "Biomolecular Sequence Analysis iterator cannot be reset.",         // 5
      "Biomolecular Sequence Analysis metadata is not available as XML.", // 6
      "Genomic Maps iterator cannot be rest.",       // 7
      "Operation not implemented in local object"   // 8
    };

  static const char *NO_RESOURCES_TABLE[] =
    {
      "Portable Interceptor operation not support in this binding.", // 1
      "No connection for request's priority."                        // 2
    };

  static const char *BAD_INV_ORDER_TABLE[] =
    {
      "Dependency exists in IFR preventing destruction of this object", // 1
      "Attempt to destroy indestructible objects in IFR.", // 2
      "Operation would deadlock.",                         // 3
      "ORB has shutdown.",                                 // 4
      "Attempt to invoke \"send\" or \"invoke\" operation of the same \"Request\" object more than once.", // 5
      "Attempt to set a servant manager after one has already been set.", // 6
      "ServerRequest::arguments called more than once or after a call to ServerRequest::set_exception.", // 7
      "ServerRequest::ctx called more than once or before ServerRequest::arguments or after ServerRequest::ctx, ServerRequest::set_result or ServerRequest::set_exception.", // 8
      "ServerRequest::set_result called more than once or before ServerRequest::arguments or after ServerRequest::set_result or ServerRequest::set_exception.", // 9
      "Attempt to send a DII request after it was sent previously.", // 10
      "Attempt to poll a DII request or to retrieve its result before the request was sent.", // 11
      "Attempt to poll a DII request or to retrieve its result after the result was retrieved previously.", // 12
      "Attempt to poll a synchronous DII request or to retrieve results from a synchronous DII request.", // 13
      "Invalid portable interceptor call",                 // 14
      "Service context add failed in portable interceptor because a service context with the given id already exists.", // 15
      "Registration of PolicyFactory failed because a factory already exists for the given type.", // 16
      "POA cannot create POAs while undergoing destruction.", // 17
      "Attempt to reassign priority.", // 18
      "An OTS/XA integration xa_start call returned XAER_OUTSIDE.", // 19
      "An OTS/XA integration xa_call returned XAER_PROTO.", // 20
      "Transaction context of request & client threads do not match in interceptor.", // 21
      "Poller has not returned any response yet.", // 22
      "Registration of TaggedProfileFactory failed because a factory already exists for the given id.", // 23
      "Registration of TaggedComponentFactory failed because a factory already exists for the given id.", // 24
       "Iteration has no more elements.", // 25
       "Invocation of this operation not allowed in post_init." // 26

    };

  static const char *TRANSIENT_TABLE[] =
    {
      "Request discarded because of resource exhaustion in POA, or because POA is in discarding state.", // 1
      "No usable profile in IOR.",                            // 2
      "Request cancelled.",                                   // 3
      "POA destroyed."                                        // 4
    };

  static const char *OBJ_ADAPTER_TABLE[] =
    {
      "System exception in AdapterActivator::unknown_adapter.",              // 1
      "Incorrect servant type returned by servant manager",     // 2
      "No default servant available [POA policy].",             // 3
      "No servant manager available [POA policy].",             // 4
      "Violation of POA policy by ServantActivator::incarnate.",// 5
      "Exception in PortableInterceptor::IORInterceptor.components_established.", // 6
      "Null servant returned by servant manager."               // 7
    };

  static const char *DATA_CONVERSION_TABLE[] =
    {
      "Character does not map to negotiated transmission code set.", // 1
      "Failure of PriorityMapping object."                           // 2
    };

  static const char *OBJECT_NOT_EXIST_TABLE[] =
    {
      "Attempt to pass an unactivated (unregistered) value as an object reference.", // 1
      "Failed to create or locate Object Adapter.", // 2
      "Biomolecular Sequence Analysis Service is no longer available.", // 3
      "Object Adapter inactive.", // 4
      "This Poller has already delivered a reply to some client." // 5
    };

  static const char *INV_POLICY_TABLE[] =
    {
      "Unable to reconcile IOR specified policy with the effective policy override.", // 1
      "Invalid PolicyType.", // 2
      "No PolicyFactory has been registered for the given PolicyType." // 3
    };

  static const char *ACTIVITY_COMPLETED_TABLE[] =
    {
      "Activity context completed through timeout, or in some way other then requested." // 1
    };

  static const char *ACTIVITY_REQUIRED_TABLE[] =
    {
      "Calling thread lacks required activity context." // 1
    };

  static const char *BAD_OPERATION_TABLE[] =
    {
      "ServantManager returned wrong servant type.", // 1
      "Operation or attribute not known to target object." // 2
    };

  static const char *BAD_CONTEXT_TABLE[] =
    {
      "IDL context not found.", // 1
      "No matching IDL context property." // 2
    };

  static const char *CODESET_INCOMPATIBLE_TABLE[] =
    {
      "Codeset negotiation failed.", // 1
      "Codeset delivered in CodeSetContext is not supported by server as transmission codeset." // 2
    };

  static const char *INTF_REPOS_TABLE[] =
    {
      "Interface Repository not available.", // 1
      "No entry for requested interface in Interface Repository." // 2
    };

  if (minor_code == 0)
    return "*unknown description*";

  --minor_code;  // Adjust to match table offset.

  CORBA::UNKNOWN const * unknown_exception =
    dynamic_cast <const CORBA::UNKNOWN *> (&exc);
  if (unknown_exception != 0
      && minor_code < sizeof UNKNOWN_TABLE / sizeof (char *))
    return UNKNOWN_TABLE[minor_code];

  CORBA::BAD_PARAM const * bad_param__exception =
    dynamic_cast <const CORBA::BAD_PARAM *> (&exc);
  if (bad_param__exception != 0
      && minor_code < sizeof BAD_PARAM_TABLE / sizeof (char *))
    return BAD_PARAM_TABLE[minor_code];

  CORBA::IMP_LIMIT const * imp_limit_exception =
    dynamic_cast <const CORBA::IMP_LIMIT *> (&exc);
  if (imp_limit_exception != 0
      && minor_code < sizeof IMP_LIMIT_TABLE / sizeof (char *))
    return IMP_LIMIT_TABLE[minor_code];

  CORBA::INITIALIZE const * initialize_exception =
    dynamic_cast <const CORBA::INITIALIZE *> (&exc);
  if (initialize_exception != 0
      && minor_code < sizeof INITIALIZE_TABLE / sizeof (char *))
    return INITIALIZE_TABLE[minor_code];

  CORBA::INV_OBJREF const * inv_objref_exception =
    dynamic_cast <const CORBA::INV_OBJREF *> (&exc);
  if (inv_objref_exception != 0
      && minor_code < sizeof INV_OBJREF_TABLE / sizeof (char *))
    return INV_OBJREF_TABLE[minor_code];

  CORBA::MARSHAL const * marshal_exception =
    dynamic_cast <const CORBA::MARSHAL *> (&exc);
  if (marshal_exception != 0
      && minor_code < sizeof MARSHAL_TABLE / sizeof (char *))
    return MARSHAL_TABLE[minor_code];

  CORBA::BAD_TYPECODE const * bad_typecode_exception =
    dynamic_cast <const CORBA::BAD_TYPECODE *> (&exc);
  if (bad_typecode_exception != 0
      && minor_code < sizeof BAD_TYPECODE_TABLE / sizeof (char *))
    return BAD_TYPECODE_TABLE[minor_code];

  CORBA::NO_IMPLEMENT const * no_implement_exception =
    dynamic_cast <const CORBA::NO_IMPLEMENT *> (&exc);
  if (no_implement_exception != 0
      && minor_code < sizeof NO_IMPLEMENT_TABLE / sizeof (char *))
    return NO_IMPLEMENT_TABLE[minor_code];

  CORBA::NO_RESOURCES const * no_resource_exception =
    dynamic_cast <const CORBA::NO_RESOURCES *> (&exc);
  if (no_resource_exception != 0
      && minor_code < sizeof NO_RESOURCES_TABLE / sizeof (char *))
    return NO_RESOURCES_TABLE[minor_code];

  CORBA::BAD_INV_ORDER const * bad_inv_order_exception =
    dynamic_cast <const CORBA::BAD_INV_ORDER *> (&exc);
  if (bad_inv_order_exception != 0
      && minor_code < sizeof BAD_INV_ORDER_TABLE / sizeof (char *))
    return BAD_INV_ORDER_TABLE[minor_code];

  CORBA::TRANSIENT const * transient_exception =
    dynamic_cast <const CORBA::TRANSIENT *> (&exc);
  if (transient_exception != 0
      && minor_code < sizeof TRANSIENT_TABLE / sizeof (char *))
    return TRANSIENT_TABLE[minor_code];

  CORBA::OBJ_ADAPTER const * obj_adapter_exception =
    dynamic_cast <const CORBA::OBJ_ADAPTER *> (&exc);
  if (obj_adapter_exception != 0
      && minor_code < sizeof OBJ_ADAPTER_TABLE / sizeof (char *))
    return OBJ_ADAPTER_TABLE[minor_code];

  CORBA::DATA_CONVERSION const * data_conversion_exception =
    dynamic_cast <const CORBA::DATA_CONVERSION *> (&exc);
  if (data_conversion_exception != 0
      && minor_code < sizeof DATA_CONVERSION_TABLE / sizeof (char *))
    return DATA_CONVERSION_TABLE[minor_code];

  CORBA::OBJECT_NOT_EXIST const * object_not_exist_exception =
    dynamic_cast <const CORBA::OBJECT_NOT_EXIST *> (&exc);
  if (object_not_exist_exception != 0
      && minor_code < sizeof OBJECT_NOT_EXIST_TABLE / sizeof (char *))
    return OBJECT_NOT_EXIST_TABLE[minor_code];

  CORBA::INV_POLICY const * inv_policy_exception =
    dynamic_cast <const CORBA::INV_POLICY *> (&exc);
  if (inv_policy_exception != 0
      && minor_code < sizeof INV_POLICY_TABLE / sizeof (char *))
    return INV_POLICY_TABLE[minor_code];

  CORBA::ACTIVITY_COMPLETED const * activity_completed_exception =
    dynamic_cast <const CORBA::ACTIVITY_COMPLETED *> (&exc);
  if (activity_completed_exception != 0
      && minor_code < sizeof ACTIVITY_COMPLETED_TABLE / sizeof (char *))
    return ACTIVITY_COMPLETED_TABLE[minor_code];

  CORBA::ACTIVITY_REQUIRED const * activity_required_exception =
    dynamic_cast <const CORBA::ACTIVITY_REQUIRED *> (&exc);
  if (activity_required_exception != 0
      && minor_code < sizeof ACTIVITY_REQUIRED_TABLE / sizeof (char *))
    return ACTIVITY_REQUIRED_TABLE[minor_code];

  CORBA::BAD_OPERATION const * bad_operation_exception =
    dynamic_cast <const CORBA::BAD_OPERATION *> (&exc);
  if (bad_operation_exception != 0
      && minor_code < sizeof BAD_OPERATION_TABLE / sizeof (char *))
    return BAD_OPERATION_TABLE[minor_code];

  CORBA::BAD_CONTEXT const * bad_context_exception =
    dynamic_cast <const CORBA::BAD_CONTEXT *> (&exc);
  if (bad_context_exception != 0
      && minor_code < sizeof BAD_CONTEXT_TABLE / sizeof (char *))
    return BAD_CONTEXT_TABLE[minor_code];

  CORBA::CODESET_INCOMPATIBLE const * codeset_incompatible_exception =
    dynamic_cast <const CORBA::CODESET_INCOMPATIBLE *> (&exc);
  if (codeset_incompatible_exception != 0
      && minor_code < sizeof CODESET_INCOMPATIBLE_TABLE / sizeof (char *))
    return CODESET_INCOMPATIBLE_TABLE[minor_code];

  CORBA::INTF_REPOS const * intf_repos_exception =
    dynamic_cast <const CORBA::INTF_REPOS *> (&exc);
  if (intf_repos_exception != 0
      && minor_code < sizeof INTF_REPOS_TABLE / sizeof (char *))
    return INTF_REPOS_TABLE[minor_code];

#else
  ACE_UNUSED_ARG (exc);
  ACE_UNUSED_ARG (minor_code);
#endif  /* !ACE_NDEBUG */

  return "*unknown description*";
}

TAO_END_VERSIONED_NAMESPACE_DECL

#if defined (THREAD_CANCELLED)
#undef THREAD_CANCELLED
#endif /* THREAD_CANCELLED */

// List of standard/system exceptions ... used to create static
// storage for their typecodes, then later to initialize that storage
// using the routine above. (It's just too painful to init these
// typecodes statically in all cases!)

#define STANDARD_EXCEPTION_LIST \
    TAO_SYSTEM_EXCEPTION (UNKNOWN) \
    TAO_SYSTEM_EXCEPTION (BAD_PARAM) \
    TAO_SYSTEM_EXCEPTION (NO_MEMORY) \
    TAO_SYSTEM_EXCEPTION (IMP_LIMIT) \
    TAO_SYSTEM_EXCEPTION (COMM_FAILURE) \
    TAO_SYSTEM_EXCEPTION (INV_OBJREF) \
    TAO_SYSTEM_EXCEPTION (OBJECT_NOT_EXIST) \
    TAO_SYSTEM_EXCEPTION (NO_PERMISSION) \
    TAO_SYSTEM_EXCEPTION (INTERNAL) \
    TAO_SYSTEM_EXCEPTION (MARSHAL) \
    TAO_SYSTEM_EXCEPTION (INITIALIZE) \
    TAO_SYSTEM_EXCEPTION (NO_IMPLEMENT) \
    TAO_SYSTEM_EXCEPTION (BAD_TYPECODE) \
    TAO_SYSTEM_EXCEPTION (BAD_OPERATION) \
    TAO_SYSTEM_EXCEPTION (NO_RESOURCES) \
    TAO_SYSTEM_EXCEPTION (NO_RESPONSE) \
    TAO_SYSTEM_EXCEPTION (PERSIST_STORE) \
    TAO_SYSTEM_EXCEPTION (BAD_INV_ORDER) \
    TAO_SYSTEM_EXCEPTION (TRANSIENT) \
    TAO_SYSTEM_EXCEPTION (FREE_MEM) \
    TAO_SYSTEM_EXCEPTION (INV_IDENT) \
    TAO_SYSTEM_EXCEPTION (INV_FLAG) \
    TAO_SYSTEM_EXCEPTION (INTF_REPOS) \
    TAO_SYSTEM_EXCEPTION (BAD_CONTEXT) \
    TAO_SYSTEM_EXCEPTION (OBJ_ADAPTER) \
    TAO_SYSTEM_EXCEPTION (DATA_CONVERSION) \
    TAO_SYSTEM_EXCEPTION (INV_POLICY) \
    TAO_SYSTEM_EXCEPTION (REBIND) \
    TAO_SYSTEM_EXCEPTION (TIMEOUT) \
    TAO_SYSTEM_EXCEPTION (TRANSACTION_UNAVAILABLE) \
    TAO_SYSTEM_EXCEPTION (TRANSACTION_MODE) \
    TAO_SYSTEM_EXCEPTION (TRANSACTION_REQUIRED) \
    TAO_SYSTEM_EXCEPTION (TRANSACTION_ROLLEDBACK) \
    TAO_SYSTEM_EXCEPTION (INVALID_TRANSACTION) \
    TAO_SYSTEM_EXCEPTION (CODESET_INCOMPATIBLE) \
    TAO_SYSTEM_EXCEPTION (BAD_QOS) \
    TAO_SYSTEM_EXCEPTION (INVALID_ACTIVITY) \
    TAO_SYSTEM_EXCEPTION (ACTIVITY_COMPLETED) \
    TAO_SYSTEM_EXCEPTION (ACTIVITY_REQUIRED) \
    TAO_SYSTEM_EXCEPTION (THREAD_CANCELLED)

static const char *repo_id_array[] = {
#define TAO_SYSTEM_EXCEPTION(name) \
                  (char *) "IDL:omg.org/CORBA/" #name ":1.0",
      STANDARD_EXCEPTION_LIST
#undef  TAO_SYSTEM_EXCEPTION
      0
  };

// Since we add an extra element subtract 1
static const CORBA::ULong array_sz =
  (sizeof (repo_id_array) / sizeof (char const *)) - 1;

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

TAO::excp_factory excp_array [] = {
#define TAO_SYSTEM_EXCEPTION(name) \
      &CORBA::name::_tao_create,
      STANDARD_EXCEPTION_LIST
#undef  TAO_SYSTEM_EXCEPTION
      0
};

// Concrete SystemException constructors
#define TAO_SYSTEM_EXCEPTION(name) \
CORBA::name ::name (void) \
  :  CORBA::SystemException ("IDL:omg.org/CORBA/" #name ":1.0", \
                             #name, \
                             0, \
                             CORBA::COMPLETED_NO) \
{ \
} \
\
CORBA::name ::name (CORBA::ULong code, CORBA::CompletionStatus completed) \
  : CORBA::SystemException ("IDL:omg.org/CORBA/" #name ":1.0", \
                            #name, \
                            code, \
                            completed) \
{ \
}

STANDARD_EXCEPTION_LIST
#undef TAO_SYSTEM_EXCEPTION

#define TAO_SYSTEM_EXCEPTION(name) \
CORBA::TypeCode_ptr \
CORBA::name ::_tao_type (void) const \
{ \
  return 0;                                     \
}

STANDARD_EXCEPTION_LIST
#undef  TAO_SYSTEM_EXCEPTION

CORBA::SystemException *
TAO::create_system_exception (const char *id)
{
  for (CORBA::ULong i = 0; i < array_sz; ++i)
    {
      if (ACE_OS::strcmp (id, repo_id_array[i]) == 0)
        return (*(excp_array[i])) ();
    }

  return 0;
}

#define TAO_SYSTEM_EXCEPTION(name) \
void \
CORBA::name ::_raise (void) const \
{ \
  throw *this; \
}

STANDARD_EXCEPTION_LIST
#undef TAO_SYSTEM_EXCEPTION

#define TAO_SYSTEM_EXCEPTION(name) \
CORBA::Exception * \
CORBA::name ::_tao_duplicate (void) const \
{ \
  CORBA::Exception * result = 0; \
  ACE_ALLOCATOR_NEW (result, CORBA::name (*this), 0); \
  return result; \
}

STANDARD_EXCEPTION_LIST
#undef TAO_SYSTEM_EXCEPTION

#define TAO_SYSTEM_EXCEPTION(name) \
CORBA::SystemException * \
CORBA::name ::_tao_create (void) \
{ \
  CORBA::name *result = 0; \
  ACE_ALLOCATOR_NEW (result, CORBA::name, 0); \
  return result; \
}

STANDARD_EXCEPTION_LIST
#undef TAO_SYSTEM_EXCEPTION

TAO_END_VERSIONED_NAMESPACE_DECL
