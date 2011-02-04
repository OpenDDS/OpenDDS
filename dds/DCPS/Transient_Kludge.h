/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSIENT_KLUDGE_H
#define OPENDDS_DCPS_TRANSIENT_KLUDGE_H

#include "dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if defined(_MSC_VER) && _MSC_VER < 1300 && _MSC_VER >= 1200
# pragma warning( disable : 4231 )
#endif

namespace OpenDDS {
namespace DCPS {

/**
* @class TransientKludge
*
* @brief Simply turn on and off the transient kludge enable flag.
*
* This class provides the methods to set/get transient kludge
* enable flag.
* Only the DCPSInfo repository should set/enable the kludge!!!
*/
class OpenDDS_Dcps_Export Transient_Kludge {
public:

  Transient_Kludge();
  ~Transient_Kludge();

  /// Return a singleton instance of this class.
  static Transient_Kludge* instance();

  /// Turn on enabled_ flag.
  void enable();

  /// Turn off enabled_ flag.
  void disable();

  /// Accessor of enable flag.
  bool is_enabled();

private:
  /// The flag.
  bool  enabled_;
};

#define TheTransientKludge OpenDDS::DCPS::Transient_Kludge::instance()

} // namespace DCPS
} // namespace OpenDDS

#if defined(__ACE_INLINE__)
#include "Transient_Kludge.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSIENT_KLUDGE_H */
