/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef SHUTDOWNINTERFACE_H
#define SHUTDOWNINTERFACE_H

class ShutdownInterface {
public:
  // Virtual destructor.
  virtual ~ShutdownInterface();

  // Request to shutdown.
  virtual void shutdown() = 0;
};

inline
ShutdownInterface::~ShutdownInterface()
{
}

#endif /* SHUTDOWNINTERFACE_H */
