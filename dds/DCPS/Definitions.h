/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DEFINITIONS_H
#define OPENDDS_DCPS_DEFINITIONS_H

#include <ace/config-lite.h>

#include <dds/OpenDDSConfigWrapper.h>

#include <dds/Versioned_Namespace.h>

#include <ace/Message_Block.h>
#include <ace/Global_Macros.h>
#include <ace/Null_Mutex.h>

#include <functional>
#include <utility>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

// More strict check than ACE does: if we have GNU lib C++ without support for
// wchar_t (std::wstring, std::wostream, etc.) then we don't have DDS_HAS_WCHAR
#if defined ACE_HAS_WCHAR && (!defined _GLIBCPP_VERSION || defined_GLIBCPP_USE_WCHAR_T)
#  define DDS_HAS_WCHAR
#endif

#ifdef ACE_HAS_CPP11
#  define OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(CLASS) \
  CLASS(const CLASS&) = delete; \
  CLASS(CLASS&&) = delete; \
  CLASS& operator=(const CLASS&) = delete; \
  CLASS& operator=(CLASS&&) = delete;
#else
#  define OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(CLASS) \
  ACE_UNIMPLEMENTED_FUNC(CLASS(const CLASS&)) \
  ACE_UNIMPLEMENTED_FUNC(CLASS& operator=(const CLASS&))
#endif

#if defined ACE_DES_FREE_THIS
#  define OPENDDS_DES_FREE_THIS ACE_DES_FREE_THIS
#else
// The macro ACE_DES_FREE_THIS is part of ACE 6.4.2 or newer, define it within
// OpenDDS at the moment we compile against an older ACE version
#  define OPENDDS_DES_FREE_THIS(DEALLOCATOR,CLASS) \
   do { \
        this->~CLASS (); \
        DEALLOCATOR (this); \
      } \
   while (0)
#endif

// If features content_filtered_topic, multi_topic, and query_condition
// are all disabled, define a macro to indicate common code these
// three features depend on should not be built.
#if defined OPENDDS_NO_QUERY_CONDITION && defined OPENDDS_NO_CONTENT_FILTERED_TOPIC && defined OPENDDS_NO_MULTI_TOPIC
#  define OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#endif

#ifndef OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
#  if !defined OPENDDS_SAFETY_PROFILE && !defined OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#    define OPENDDS_HAS_DYNAMIC_DATA_ADAPTER 1
#  else
#    define OPENDDS_HAS_DYNAMIC_DATA_ADAPTER 0
#  endif
#endif

#ifdef OPENDDS_SAFETY_PROFILE
#  define OPENDDS_ASSERT(C) ((void) 0)
#else
#  include <cassert>
#  define OPENDDS_ASSERT(C) assert(C)
#endif

#define OPENDDS_TEST_AND_CALL(TYPE, TEST, CALL) do { TYPE temp = TEST; if (temp) { temp->CALL; } } while (false);
#define OPENDDS_TEST_AND_CALL_ASSIGN(TYPE, TEST, CALL, VAL) do { TYPE temp = TEST; if (temp) { VAL = temp->CALL; } } while (false);

#include <tao/orbconf.h>
#if defined TAO_HAS_IDL_FEATURES && TAO_HAS_IDL_FEATURES
#  include <tao/idl_features.h>
#  define OPENDDS_HAS_EXPLICIT_INTS TAO_IDL_HAS_EXPLICIT_INTS
#else
#  define OPENDDS_HAS_EXPLICIT_INTS 0
#endif

#if defined __GNUC__ && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || defined __clang__)
#  define OPENDDS_GCC_HAS_DIAG_PUSHPOP 1
#else
#  define OPENDDS_GCC_HAS_DIAG_PUSHPOP 0
#endif

#ifndef OPENDDS_CONFIG_AUTO_STATIC_INCLUDES
#  define OPENDDS_CONFIG_AUTO_STATIC_INCLUDES 0
#endif

#if !OPENDDS_CONFIG_AUTO_STATIC_INCLUDES && defined(ACE_AS_STATIC_LIBS)
#  define OPENDDS_DO_MANUAL_STATIC_INCLUDES 1
#else
#  define OPENDDS_DO_MANUAL_STATIC_INCLUDES 0
#endif

#ifndef OPENDDS_CONFIG_BOOTTIME_TIMERS
#  define OPENDDS_CONFIG_BOOTTIME_TIMERS 0
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// This struct holds both object reference and the corresponding servant.
template <typename T_impl, typename T, typename T_ptr, typename T_var>
struct Objref_Servant_Pair {
  Objref_Servant_Pair()
    : svt_(0)
  {}

  Objref_Servant_Pair(T_impl* svt, T_ptr obj, bool dup)
    : svt_(svt)
  {
    if (dup) {
      obj_ = T::_duplicate(obj);

    } else {
      obj_ = obj;
    }
  }

  ~Objref_Servant_Pair()
  {}

  bool operator==(const Objref_Servant_Pair & pair) const {
    return pair.svt_ == this->svt_;
  }

  bool operator<(const Objref_Servant_Pair & pair) const {
    return this->svt_ < pair.svt_;
  }

  T_impl* svt_;
  T_var obj_;
};

/// Use a Foo_var in a std::set or std::map with this comparison function,
/// for example std::set<Foo_var, VarLess<Foo> >
template <class T, class V = typename T::_var_type>
struct VarLess {
  typedef V first_argument_type;
  typedef V second_argument_type;
  typedef bool result_type;

  bool operator()(const V& x, const V& y) const {
    return x.in() < y.in();
  }
};

/// Size of TCHAR buffer for use with ACE::timestamp, e.g.,
///  ACE_TCHAR buffer[AceTimestampSize];
///  ACE::timestamp(buffer, AceTimestampSize);
const size_t AceTimestampSize = 27;

/// Size of TCHAR buffer for use with addr_to_string.
const size_t AddrToStringSize = 256;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DEFINITION_H */
