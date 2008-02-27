// -*- C++ -*-
//
// $Id$
#include "LinkStateManager.h"
#include "dds/DCPS/Definitions.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include <algorithm>

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

bool
LinkStateManager::update(
  const LinkState& linkState,
  LinkList&        removedFromMst,
  LinkList&        addedToMst
)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) LinkStateManager::update\n")
  ));

  // See if this link has been observed before.
  AdjacencyColumn::const_iterator location
    = this->adjacencies_[ linkState.source].find( linkState.destination);
  if( location != this->adjacencies_[ linkState.source].end()) {
    // This link has been previously observed, check the sequence numbers.
    OpenDDS::DCPS::SequenceNumber previous
      = this->adjacencies_[ linkState.source][ linkState.destination].second;
    if( previous <= linkState.packet) {
      // Do not process if the incoming sequence value is not greater than
      // the currently held sequence value for this link.
      return false;
    }
  }

  // Store the new link state.
  this->adjacencies_[ linkState.source][ linkState.destination].first  = linkState.cost;
  this->adjacencies_[ linkState.source][ linkState.destination].second = linkState.packet;

  // Compute the new Minimal Spanning Tree.
  LinkSet currentMst = this->prim();

  // Determine any changes from the previous MST.
  LinkList::iterator end;

  // Previous MST MINUS current MST (removals).
  removedFromMst.resize( this->mst_.size());
  end = std::set_difference( this->mst_.begin(), this->mst_.end(),
                             currentMst.begin(), currentMst.end(),
                             removedFromMst.begin(), LinkLess());
  if( end < removedFromMst.end()) {
    removedFromMst.erase( ++end, removedFromMst.end());
  }

  // Current MST MINUS previous MST (additions).
  addedToMst.resize( currentMst.size());
  end = std::set_difference( currentMst.begin(), currentMst.end(),
                             this->mst_.begin(), this->mst_.end(),
                             addedToMst.begin(), LinkLess());
  if( end < addedToMst.end()) {
    addedToMst.erase( ++end, addedToMst.end());
  }

  return (removedFromMst.size() != 0) || (addedToMst.size() != 0);
}

LinkStateManager::LinkSet
LinkStateManager::prim()
{
  LinkSet mst;

  /** @TODO: Implement Prim's algorithm here. **/

  return mst;
}

}} // End of namespace OpenDDS::Federator

