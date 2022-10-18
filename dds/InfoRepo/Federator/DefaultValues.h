/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DEFAULTVALUES_H
#define DEFAULTVALUES_H

#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Federator {
namespace Defaults {

enum {
  DiscoveryRequestPort = 10022,
  DiscoveryReplyPort = 10023
};

} // namespace Defaults
} // namespace Federator
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* DEFAULTVALUES_H */
