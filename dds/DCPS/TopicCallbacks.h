/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TOPICCALLBACKS_H
#define OPENDDS_DCPS_TOPICCALLBACKS_H

#include "RcObject.h"
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
* @class TopicCallbacks
*
* @brief Defines the interface for Discovery callbacks into the Topic.
*
*/
class TopicCallbacks
  : public virtual RcObject {
public:

  TopicCallbacks() {}

  virtual ~TopicCallbacks() {}

  // Report the current number of inconsistent topics.
  virtual void inconsistent_topic(int count) = 0;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TOPICCALLBACKS_H  */
