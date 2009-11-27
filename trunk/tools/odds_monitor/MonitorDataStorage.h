/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef MONITORDATASTORAGE_H
#define MONITORDATASTORAGE_H

#include "dds/DCPS/GuidUtils.h"

#include <map>

class RepoIdGenerator;

namespace Monitor {

class TreeNode;

/**
 * @class MonitorDataStorage
 *
 * @brief manange the storage of data for both DDS and the GUI.
 *
 * This class provides access to underlying storage of instrumentation
 * data for both the OpenDDS service and the GUI.  The GUI accesses data
 * via a row/colum/parent schema, while the service accesses data
 * directly via Guid values.
 *
 * Since the OpenDDS service implementation uses Guid values to identify
 * most of its internal entities, we maintain a map keyed by Guid values
 * to easily access data for the service.  Since some entities do not
 * naturally have Guid values associated with them, we extend the Guid
 * values to include some user defined Guid values local to this process
 * and specific to the types needing them.  Maps from the OpenDDS key
 * values to the local Guid values are maintained to allow simple
 * searches to be performed.
 *
 * Currently, the entities providing instrumentation data that require
 * lcoal Guid values to be generated include: Publisher, Subscriber, and
 * Transport entities.  Publisher and Subscriber Entities are keyed by
 * the containing participant and the InstanceHandle value within that
 * participant.  Transport entities are keyed some other way.
 *
 * The current scheme simply assigns an OpenDDS specific Vendor entityKind
 * value for the three different types of entities, then increments the
 * entityId value for subsequent generated values.  This means that after
 * observing 16 million entities of a type, the Guid values will wrap -
 * and operation is undefined after that point.  Other limits are likely
 * to be exceeded before this becomes a problem.
 */
class MonitorDataStorage {
  public:
    /// Construct with an IOR only.
    MonitorDataStorage();

    /// Virtual destructor.
    virtual ~MonitorDataStorage();

    /// Clean contents from all storage.
    void clear();

    /// @name GUI data access methods.
    /// @{

    /// @}

    /// @name OpenDDS service access methods.
    /// @{

    /// @}

  private:
    /// Map GUID_t values to TreeNode elements.
    typedef
      std::map< OpenDDS::DCPS::GUID_t,
                std::pair< int, TreeNode*>,
                GUID_tKeyLessThan>
      GuidToTreeMap;
    GuidToTreeMap guidToTreeMap_;

    /// Generate Guid values for publishers.
    RepoIdGenerator* publisherIdGenerator_;

    /// Generate Guid values for subscribers.
    RepoIdGenerator* subscriberIdGenerator_;

    /// Generate Guid values for transports.
    RepoIdGenerator* transportIdGenerator_;
};

} // End of namespace Monitor

#endif /* MONITORDATASTORAGE_H */

