// -*- C++ -*-
//
// $Id$

#include "FederatorRemoteData.h"

#if !defined (__ACE_INLINE__)
# include "FederatorRemoteData.inl"
#endif /* ! __ACE_INLINE__ */

namespace { // Anonymous namespace for file scope.

  // Size of PARTITION names for federation communications.
  enum { PARTITIONNAME_SIZE = 17 };

} // End of anonymous namespace.

namespace OpenDDS { namespace Federator {

RemoteData::RemoteData( RepoKey self, RepoKey remote)
 : federationId_( remote)
{
  char buffer[PARTITIONNAME_SIZE];

  ACE_OS::sprintf( &buffer[0], "%08.8x-%08.8x", remote, self);
  this->inbound_ = &buffer[0];

  ACE_OS::sprintf( &buffer[0], "%08.8x-%08.8x", self, remote);
  this->external_ = &buffer[0];
}

RemoteData::~RemoteData()
{
}

}} // End namespace OpenDDS::Federator

