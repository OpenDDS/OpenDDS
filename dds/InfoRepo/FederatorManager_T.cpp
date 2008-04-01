// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORMANAGER_T_CPP
#define FEDERATORMANAGER_T_CPP

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "FederatorManagerImpl.h"

namespace OpenDDS { namespace Federator {

template< class SampleType>
void
ManagerImpl::update( SampleType& /* sample */, ::DDS::SampleInfo& /* info */)
{
  ACE_ERROR((LM_ERROR,
    ACE_TEXT("(%P|%t) ERROR: ManagerImpl::update - ")
    ACE_TEXT("unrecognized (unhandled) update type.\n")
  ));
}

}} // End of OpenDDS::Federator

#endif /* FEDERATORMANAGER_T_CPP */

