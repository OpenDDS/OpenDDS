/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DEBUG_H
#define OPENDDS_DCPS_DEBUG_H

#include "dcps_export.h"
#include "ace/ace_wchar.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Logging verbosity level.
/// set by Service_Participant
/// value guidelines:
/// 1 - logs that should happen once per process or are warnings
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

  /// Permissions and Governance
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
};
extern OpenDDS_Dcps_Export SecurityDebug security_debug;
#endif

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DEBUG_H */
