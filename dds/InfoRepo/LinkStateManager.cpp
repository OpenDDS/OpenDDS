// -*- C++ -*-
//
// $Id$
#include "LinkStateManager.h"
#include "dds/DCPS/Definitions.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include <algorithm>

namespace { // Anonymous namespace for file scope.
  //
  // Predicate functor for partitioning edges by candidacy in the
  // implementation of Prim's algorithm.  This will be used by the
  // std::partition() algorithm to find candidate edges.
  //
  class EdgePartition {
    public:
      // Store the value to use for comparisons.
      EdgePartition( OpenDDS::Federator::RepoKey nodeId) : nodeId_( nodeId) { }

      // Check for candidacy.  An edge is a candidate if it is directly
      // connected on at least one end to the node being considered.
      bool operator()( const OpenDDS::Federator::LinkStateManager::Link& link) {
        return ((link.first == this->nodeId_) || (link.second == this->nodeId_));
      }

    private:
      // The value to be compared.
      OpenDDS::Federator::RepoKey nodeId_;
  };

} // End of anonymous namespace.

namespace OpenDDS { namespace Federator {

LinkStateManager::LinkStateManager()
 : repoId_( 0xc0edbeef)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) LinkStateManager::LinkStateManager\n")
  ));
}

LinkStateManager::LinkStateManager( RepoKey repoId)
 : repoId_( repoId)
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

RepoKey
LinkStateManager::repoId() const
{
  return this->repoId_;
}

RepoKey&
LinkStateManager::repoId()
{
  return this->repoId_;
}

void
LinkStateManager::generateMst( LinkSet& mst)
{
  // Default to using Prim's algorithm to determine the MST.
  this->prim( mst);
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
    // This link has been previously observed, compare the sequence numbers.
    OpenDDS::DCPS::SequenceNumber previous
      = this->adjacencies_[ linkState.source][ linkState.destination].second;
    if( previous >= linkState.packet) {
      // Do not process if the incoming sequence value is not greater than
      // the currently held sequence value for this link.
      return false;
    }
  }

  // Store the new link state.
  this->adjacencies_[ linkState.source][ linkState.destination]
    = AdjacencyCell( linkState.cost, linkState.packet);

  // Compute the new Minimal Spanning Tree.
  LinkSet currentMst;
  this->generateMst( currentMst);

  // Determine any changes from the previous MST.
  LinkList::iterator end;

  // Previous MST MINUS current MST (removals).
  removedFromMst.resize( this->mst_.size());
  end = std::set_difference( this->mst_.begin(), this->mst_.end(),
                             currentMst.begin(), currentMst.end(),
                             removedFromMst.begin(), LinkLess());
  removedFromMst.resize( end - removedFromMst.begin());

  // Current MST MINUS previous MST (additions).
  addedToMst.resize( currentMst.size());
  end = std::set_difference( currentMst.begin(), currentMst.end(),
                             this->mst_.begin(), this->mst_.end(),
                             addedToMst.begin(), LinkLess());
  addedToMst.resize( end - addedToMst.begin());

  // Now copy the new MST into the instance.
  this->mst_.erase( this->mst_.begin(), this->mst_.end());
  this->mst_.insert( currentMst.begin(), currentMst.end());

  // The inbound data was valid.
  return true;
}

void
LinkStateManager::prim( LinkSet& mst)
{
  typedef std::set< RepoKey> NodeSet;

  // Set of unattached nodes to be connected.
  NodeSet unattached;

  // Set of nodes currently in the MST under construction.
  NodeSet currentNodes;

  // List of edges from the topology.
  LinkList edges;

  // Find all the unattached nodes.  Scan each row.
  for( AdjacencyRow::const_iterator currentRow = this->adjacencies_.begin();
       currentRow != this->adjacencies_.end();
       ++currentRow) {
    // Add the current row node.
    unattached.insert( currentRow->first);

    // Scan each colum of the current row.
    for( AdjacencyColumn::const_iterator currentColumn = currentRow->second.begin();
         currentColumn != currentRow->second.end();
         ++currentColumn) {
      // Add the current row node.
      unattached.insert( currentColumn->first);
    }
  }

  // Find all the edges.  Scan each row.
  for( AdjacencyRow::const_iterator currentRow = this->adjacencies_.begin();
       currentRow != this->adjacencies_.end();
       ++currentRow) {
    // Scan each colum of the current row.
    for( AdjacencyColumn::const_iterator currentColumn = currentRow->second.begin();
         currentColumn != currentRow->second.end();
         ++currentColumn) {
      //
      // See if the reverse link is already in the edges list.
      //
      LinkList::const_iterator reverseEdgeLocation
        = find(
            edges.begin(),
            edges.end(),
            Link( currentColumn->first, currentRow->first)
          );
      if( reverseEdgeLocation != edges.end()) {
        // We do not need to have both directions since our application
        // topology is not a directed graph.
        continue;
      }

      // Current edge cost.
      LinkCost currentCost = currentColumn->second.first;

      // Reverse edge cost.
      //
      // N.B. We need to check the cost for *both* directions and use the
      //      highest cost of the two here since the LinkState for a newly
      //      unreachable node will never arrive, and we _do_ need to
      //      remove the link from consideration.
      LinkCost reverseCost = LINK_OFF;
      AdjacencyColumn::iterator reverseColumnLocation
        = this->adjacencies_[ currentColumn->first].find( currentRow->first);
      if( reverseColumnLocation != this->adjacencies_[ currentColumn->first].end()) {
        reverseCost = reverseColumnLocation->second.first;
      }

      //
      // Use the cost to consider whether we should include an edge or not.
      //
      if( (currentCost == LINK_ON) && (reverseCost == LINK_ON)) {
        edges.push_back( Link( currentRow->first, currentColumn->first));
      }
    }
  }

  // Keep generating MSTs until we find one that contains the current
  // node or we run out of nodes to include.
  while( unattached.size() > 0) {
    // Always select the first node in the set of nodes to start with.
    //
    // N.B. We do this instead of using our own repository as the
    //      starting point to ensure that _all_ repositories which
    //      receive the same LinkState data will produce the same
    //      resultant MST.  This is a necessary constraint for the
    //      routing of data to work correctly.
    currentNodes.insert( *unattached.begin());
    unattached.erase(     unattached.begin());

    bool edgeFound = false;
    do {
      edgeFound = false;  // We have not found an(other) edge yet.

      // Find the first candidate to add to the MST.
      // Always add the first candidate.  It _does_ say 'arbitrarily'!
      // Since our containers are ordered, the result should be the same
      // every time we examine the same topology.
      for( NodeSet::const_iterator currentNodeLocation = currentNodes.begin();
           (currentNodeLocation != currentNodes.end()) && (edgeFound == false);
           ++currentNodeLocation) {

        // Generate a list of candidate edges.
        LinkList::iterator candidateEnd
          = std::partition( edges.begin(), edges.end(), EdgePartition( *currentNodeLocation));

        // Search the candidates for one that does not create a cycle if added.
        for( LinkList::const_iterator candidateLocation = edges.begin();
             (candidateLocation != candidateEnd) && (edgeFound == false);
             ++candidateLocation) {

          // This is ugly.
          RepoKey newNode = candidateLocation->first;
          if( newNode == *currentNodeLocation) {
            newNode = candidateLocation->second;
          }

          // Check for an acyclic direct connection to the current node
          // in the search.
          if( newNode != *currentNodeLocation) {
            NodeSet::const_iterator location = currentNodes.find( newNode);
            if( location == currentNodes.end()) {
              // We found a candidate directly connected to the current node.
              edgeFound = true;

              // Add the edge to the MST.
              mst.insert( *candidateLocation);

              // Add the new node to the set of nodes in the MST.
              currentNodes.insert( newNode);

              // Remove the new node from the set of unattached nodes.
              NodeSet::iterator unattachedLocation = unattached.find( newNode);
              if( unattachedLocation != unattached.end()) {
                unattached.erase( unattachedLocation);
              }
            }
          }
        }
      }

    // Collect edges until no more can be added without creating a cycle.
    } while( edgeFound);

    // If the current node is _not_ in the current set, we found the MST
    // for an unreachable island of nodes.  Use the remaining unattached
    // nodes to find the next MST.
    NodeSet::const_iterator nodeLocation = currentNodes.find( this->repoId_);
    if( nodeLocation == currentNodes.end()) {
      // Start over and generate another MST from the remaining nodes.
      currentNodes.erase( currentNodes.begin(), currentNodes.end());
      mst.erase( mst.begin(), mst.end());

    } else {
      // We are in the current MST, so we are done.
      break;
    }
  }
}

}} // End of namespace OpenDDS::Federator

