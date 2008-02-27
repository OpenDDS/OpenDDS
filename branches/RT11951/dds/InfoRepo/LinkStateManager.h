// -*- C++ -*-
//
// $Id$
#ifndef LINKSTATEMANAGER_H
#define LINKSTATEMANAGER_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/InfoRepo/FederatorC.h"

#include <vector>
#include <set>
#include <map>

namespace OpenDDS { namespace Federator {

/**
 * @class LinkStateManager
 *
 * @brief Manage the topology and a Minimum Spanning Tree (MST) for the
 *        current topology.
 *
 * This class manages a topology consisting of individual links.
 * Information about the links is provided by updating the LinkState
 * information containing the source, destination, cost and sequence of
 * the link.  The resulting topology is then used to generate a MST with
 * removals of links and additions of links resulting from the update
 * provided as a result of the update call.  The topology and MST are
 * also available separately.
 */
class LinkStateManager {
  public:
    /// Links in the topology and MST.
    typedef std::pair< RepoKey, RepoKey> Link;

    /// Unordered list of links.
    typedef std::vector< Link> LinkList;

    /// Default constructor
    LinkStateManager();

    /// Virtual destructor
    virtual ~LinkStateManager();

    /**
     * @brief Update link state.
     *
     * @param linkState      - new link state information
     * @param removedFromMst - links removed from the MST
     * @param addedToMst     - links added to the MST
     *
     * @return bool flag indicating that the MST was modified.
     *
     * This method updates the topology information, held in a sparse
     * adjacency matrix, then determines a (new) Minimum Spanning Tree (MST)
     * and generates a list of links removed from and added to the MST
     * when the link state information was updated.
     *
     */
    bool update(
           const LinkState& linkState,
           LinkList&        removedFromMst,
           LinkList&        addedToMst
         );

  private:
    /*
     * Types
     */

    /// Functor to allow use of Link type in ordered containers.
    class LinkLess { public: bool operator()( const Link& lhs, const Link& rhs) {
      return (lhs.first  < rhs.first)? true:
             (lhs.first == rhs.first)? (lhs.second < rhs.second):
                                       false;
    }};

    /// Ordered set of Links.
    typedef std::set< Link, LinkLess> LinkSet;

    /// Cell of the adjacency matrix has cost and sequence number.
    typedef std::pair< LinkCost, int>           AdjacencyCell;

    /// Column of the adjacency matrix is keyed by destination Id.
    typedef std::map< RepoKey, AdjacencyCell>   AdjacencyColumn;

    /// Row of the adjacency matrix is keyed by source Id.
    typedef std::map< RepoKey, AdjacencyColumn> AdjacencyRow;

    /*
     * Methods
     */

    /// Implement Prim's algorithm to extract a Minimal Spanning Tree
    /// (MST) from the topology.
    LinkSet prim();

    /*
     * Members
     */

    /// Topology storage.
    AdjacencyRow adjacencies_;

    /// Current Minimal Spanning Tree (MST).
    LinkSet mst_;

};

}} // End of namespace OpenDDS::Federator

#endif /* LINKSTATEMANAGER_H  */

