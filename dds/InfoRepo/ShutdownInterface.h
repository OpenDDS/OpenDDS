/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef SHUTDOWNINTERFACE_H
#define SHUTDOWNINTERFACE_H

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

class ShutdownInterface {
public:
  virtual ~ShutdownInterface();

  // Request to shutdown.
  virtual void shutdown() = 0;
};

inline
ShutdownInterface::~ShutdownInterface()
{
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* SHUTDOWNINTERFACE_H */
