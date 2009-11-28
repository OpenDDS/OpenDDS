/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLEMCASTFACTORY_H
#define OPENDDS_DCPS_SIMPLEMCASTFACTORY_H

#include "SimpleUnreliableDgram_export.h"
#include "dds/DCPS/transport/framework/TransportImplFactory.h"

class SimpleMcastTransport;

namespace OpenDDS {
namespace DCPS {

class SimpleUnreliableDgram_Export SimpleMcastFactory : public TransportImplFactory {
public:

  SimpleMcastFactory();
  virtual ~SimpleMcastFactory();

  virtual int requires_reactor() const;

protected:

  virtual TransportImpl* create();
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleMcastFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DDS_SIMPLEMCASTFACTORY_H */
