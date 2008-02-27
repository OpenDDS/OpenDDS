// -*- C++ -*-
//
// $Id$
#ifndef LINKSTATEMANAGER_H
#define LINKSTATEMANAGER_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS { namespace Federator {

class LinkStateManager {
  public:
    /// Default constructor
    LinkStateManager();

    /// Virtual destructor
    virtual ~LinkStateManager();

};

}} // End of namespace OpenDDS::Federator

#endif /* LINKSTATEMANAGER_H  */

