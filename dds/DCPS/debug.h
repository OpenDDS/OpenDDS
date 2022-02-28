/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DEBUG_H
#define OPENDDS_DCPS_DEBUG_H

#include "dcps_export.h"

#ifndef OPENDDS_UTIL_BUILD
#include "transport/framework/TransportDebug.h"
#endif

#include <ace/ace_wchar.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * General control for logging in OpenDDS.
 *
 * Access using the log_level global object.
 */
class OpenDDS_Dcps_Export LogLevel {
public:
  enum Value {
    None,
    Error,
    Warning,
    Notice,
    Info,
    Debug
  };

  LogLevel(Value value)
  {
    set(value);
  }

  void set(Value value);
  void set_from_string(const char* name);

  Value get() const
  {
    return level_;
  }
  const char* get_as_string() const;

private:
  Value level_;
};
extern OpenDDS_Dcps_Export LogLevel log_level;

inline bool operator>=(const LogLevel& ll, LogLevel::Value value)
{
  return ll.get() >= value;
}

/// Logging verbosity level.
/// set by Service_Participant
/// value guidelines:
/// 1 - logs that should happen once per process
/// 2 - logs that should happen once per DDS entity
/// 4 - logs that are related to administrative interfaces
/// 6 - logs that should happen every Nth sample write/read
/// 8 - logs that should happen once per sample write/read
/// 10 - logs that may happen more than once per sample write/read
extern OpenDDS_Dcps_Export unsigned int DCPS_debug_level;

/// The proper way to set the DCPS_debug_level.
/// This function allows for possible side-effects of setting the level.
extern void OpenDDS_Dcps_Export set_DCPS_debug_level(unsigned int lvl);

#ifdef OPENDDS_SECURITY
/**
 * Global Security Debug Settings
 */
class OpenDDS_Dcps_Export SecurityDebug {
public:
  SecurityDebug();

  /// Set all security debug message flags to this value
  void set_all_flags_to(bool value);

  /**
   * Parse a comma delimited string and set the corresponding flags.
   * Unknown ones are ignored and "all" enables all the flags.
   * Ex: "bookkeeping,showkeys"
   */
  void parse_flags(const ACE_TCHAR* flags);

  /**
   * Set debug level similarly to DCPSDebugLevel
   */
  void set_debug_level(unsigned level);

  /** @name SecurityFlags
   * These are the categories of Security Debug Messages
   */
  ///@{
  /// Encrypting and Decrypting
  bool encdec_error;
  bool encdec_warn;
  bool encdec_debug;

  /// Authentication and Handshake
  bool auth_debug;
  bool auth_warn;

  /// New entity creating
  bool new_entity_error;
  bool new_entity_warn;

  /// Cleanup
  bool cleanup_error;

  /// Permissions and Governance
  bool access_error;
  bool access_warn;

  /// Generation and Tracking of Crypto Handles and Keys
  bool bookkeeping;

  /// Print the Key when Generating it or Using It
  bool showkeys;

  /// Print Verbose Search Info About Getting the Crypto Handle from a Key id
  bool chlookup;
  ///@}

  /// Disable all encryption for security, even the required builtin encryption.
  bool fake_encryption;

  /**
   * Force role in authentication handshake. Like fake encryption this will
   * break everything if applied inconsistently.
   */
  enum ForceAuthRole {
    FORCE_AUTH_ROLE_NORMAL,
    FORCE_AUTH_ROLE_LEADER,
    FORCE_AUTH_ROLE_FOLLOWER
  } force_auth_role;
};
extern OpenDDS_Dcps_Export SecurityDebug security_debug;
#endif

#ifndef OPENDDS_UTIL_BUILD
class LogRestore {
public:
  LogRestore()
    : orig_log_level_(log_level)
    , orig_dcps_debug_level_(DCPS_debug_level)
    , orig_transport_debug_level_(Transport_debug_level)
    , orig_transport_debug_(transport_debug)
#ifdef OPENDDS_SECURITY
    , orig_security_debug_(security_debug)
#endif
  {
  }

  ~LogRestore()
  {
    log_level = orig_log_level_;
    DCPS_debug_level = orig_dcps_debug_level_;
    Transport_debug_level = orig_transport_debug_level_;
    transport_debug = orig_transport_debug_;
#ifdef OPENDDS_SECURITY
    security_debug = orig_security_debug_;
#endif
  }

private:
  LogLevel orig_log_level_;
  unsigned orig_dcps_debug_level_;
  unsigned orig_transport_debug_level_;
  TransportDebug orig_transport_debug_;
#ifdef OPENDDS_SECURITY
  SecurityDebug orig_security_debug_;
#endif
};
#endif // OPENDDS_UTIL_BUILD

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DEBUG_H */
