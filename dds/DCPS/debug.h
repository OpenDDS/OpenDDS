/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DEBUG_H
#define OPENDDS_DCPS_DEBUG_H

#include "dcps_export.h"

#include <ace/ace_wchar.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

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

  Value get() const
  {
    return level_;
  }

private:
  Value level_;
};
extern OpenDDS_Dcps_Export LogLevel log_level;

inline bool operator>=(const LogLevel& ll, LogLevel::Value value)
{
  return ll.get() >= value;
}

/*
if (log_level >= LogLevel::Error) {
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: funky_function: error\n"));
}

if (log_level >= LogLevel::Warning) {
  ACE_DEBUG((LM_WARN, "(%P|%t) WARNING: funky_function: warning\n"));
}

if (log_level >= LogLevel::Notice) {
  ACE_DEBUG((LM_NOTICE, "(%P|%t) NOTICE: funky_function: notice\n"));
}

if (log_level >= LogLevel::Info) {
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: funky_function: info\n"));
}
*/

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

class DebugRestore {
public:
  DebugRestore()
    : orig_dcps_debug_level_(DCPS_debug_level)
#ifdef OPENDDS_SECURITY
    , orig_security_debug_(security_debug)
#endif
  {
  }

  ~DebugRestore()
  {
    DCPS_debug_level = orig_dcps_debug_level_;
#ifdef OPENDDS_SECURITY
    security_debug = orig_security_debug_;
#endif
  }

private:
  unsigned orig_dcps_debug_level_;
#ifdef OPENDDS_SECURITY
  SecurityDebug orig_security_debug_;
#endif
};

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DEBUG_H */
