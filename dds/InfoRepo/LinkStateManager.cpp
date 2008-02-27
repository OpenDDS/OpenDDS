// -*- C++ -*-
//
// $Id$
#include "LinkStateManager.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

namespace OpenDDS { namespace Federator {

LinkStateManager::LinkStateManager()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) LinkStateManager::LinkStateManager\n")
  ));
}

LinkStateManager::~LinkStateManager (void)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) LinkStateManager::~LinkStateManager\n")
  ));
}

}} // End of namespace OpenDDS::Federator

