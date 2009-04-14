// -*- C++ -*-


//=============================================================================
/**
 *  @file    DefaultValues.h
 *
 *  $Id$
 *
 *  Defines a class that listens to a multicast address for client requests
 *  for ior of a bootstrappable service.
 *
 *
 *  @author   Sergio Flores-Gaitan
 */
//=============================================================================


#ifndef DEFAULTVALUES_H
#define DEFAULTVALUES_H
#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS { namespace Federator {

  namespace Defaults
  {

    enum
    {
      DiscoveryRequestPort = 10022,
      DiscoveryReplyPort = 10023
    };

  }

} }

#include /**/ "ace/post.h"
#endif /* DEFAULTVALUES_H */
