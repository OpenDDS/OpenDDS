/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef MONITORDATASTORAGE_H
#define MONITORDATASTORAGE_H

#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/monitor/monitorC.h"

#include <map>
#include <string>
#include <algorithm>

// For the inline implementations.
#include "MonitorData.h"
#include "TreeNode.h"
#include "QosFormatter.h"
#include "dds/DCPS/Discovery.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"

namespace Monitor {

using OpenDDS::DCPS::GUID_tKeyLessThan;

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
 * naturally have Guid values associated with them, we include containers
 * accesible with the natural keys for these types.  These include
 * process, transport, and instance information.
 *
 * Process information is accessed using a combination of hostname and
 * process Id.  Transports are identified by process (hostname+pid) and
 * the unique transport Id value within that process.  Instance
 * information (Publisher and Subscriber access) is accessed using the
 * containing DomainParticipant Guid and the Entity instance handle.
 *
 * The GUI represents the data as a hierarchical tree.  The structure of
 * the tree is consistent in what node types are represented at similar
 * levels of the tree hierarchy.  Currently, this hierarchy includes:
 *
 *    host.pid.transport-id.data
 *    host.pid.participant-guid.data
 *    host.pid.participant-guid.topic-guid.data
 *    host.pid.participant-guid.publisher-handle.data
 *    host.pid.participant-guid.publisher-handle.writer-guid.data
 *    host.pid.participant-guid.subscriber-handle.data
 *    host.pid.participant-guid.subscriber-handle.reader-guid.data
 *
 * Nodes in the tree can contain one of several types of contents.  These
 * contents are displayed as a name/value pair of string data.  Node types
 * include:
 *
 *   root node   - header information
 *                 (name = "Element", value = "Value")
 *   host node   - holds a host name
 *                 (name = "Host", value = <name>)
 *   pid node    - holds a process identifier
 *                 (name = "Process", value = <pid>)
 *   entity node - holds an entity identified by GUID_t value
 *                 (name = "<type>", value = <stringified id value>)
 *   data node   - holds simple data with no global identity
 *                 (name = "<name>", value = <value>)
 *
 * where the entity <types> are:
 *
 *   DomainParticipant
 *   Topic
 *   Publisher
 *   Subscriber
 *   DataWriter
 *   DataReader
 *   Transport
 *
 * Nodes in the tree can contain nodes of several types as children.
 * These include:
 *
 *   <host> nodes
 *     can contain: <pid> nodes
 *   <pid> nodes
 *     can contain: Transports, DomainParticipants, data
 *   DomainParticipant
 *     can contain: Topics, Publishers, Subscribers, Qos, data
 *   Topic
 *     can contain: Qos, data
 *   Publisher
 *     can contain: Qos, DataWriters, data
 *   Subscriber
 *     can contain: Qos, DataReaders, data
 *   DataWriter
 *     can contain: Qos, data
 *   DataReader
 *     can contain: Qos, data
 *   Transport
 *     can contain: data
 */
class MonitorDataStorage {
  public:
    /// Construct with a reference to the model facade.
    MonitorDataStorage( MonitorData* model);

    virtual ~MonitorDataStorage();

    /// Clean contents from all storage and restart the generators.
    /// N.B. It is a mistake to clear this storage without destroying the
    ///      GUI tree it references as well.
    void reset();

    /// Access the active repository IOR.
    std::string& activeIor();
    std::string  activeIor() const;

    void currentIor( const std::string& ior);

    /// @name GUI data access methods.
    /// @{

    /// @}

    /// @name OpenDDS service access methods.
    /// @{

    /// Add, modify, or remove a data element in the model.
    template<typename DataType>
    void update(const DataType& data,
                DDS::DomainParticipant_ptr participant,
                bool remove = false);

    /// Completely delete a process node from the model.
    void deleteProcessNode( TreeNode* node);

    /// Completely delete a tree node from the model.
    template< class MapType>
    void deleteNode( MapType& map, TreeNode* node);

    /// @}

  private:
    /// Clear the maps and delete the generators.
    void clear();

    /// Recursively descend a tree and erase the contents from the maps.
    void cleanMaps( TreeNode* node);

    /// Find a map key for a tree node in the map.
    template< class MapType>
    std::pair< bool, typename MapType::key_type>
    findKey( MapType& map, TreeNode* node);

    /// Install or update a child value node.
    bool manageChildValue(
      TreeNode*      parent,
      TreeNode*&     node,
      const QString& label,
      const QString& value
    );

    /// Install or update a QoS policy node.
    template< class PolicyType>
    bool manageQosPolicy(
           TreeNode*         node,
           const QString&    label,
           const PolicyType& value
         );

    /// Convenience type for storing information.
    typedef std::pair< int, TreeNode*> RowNodePair;

    /// Uniquely identify processes.
    struct ProcessKey {
      ProcessKey( const std::string& h = "", int p = 0)
        : host( h), pid( p)
      { }
      std::string host;
      int         pid;
      bool operator<( const ProcessKey& rhs) const;
    };

    /// Uniquely identify transports.
    struct TransportKey {
      TransportKey( const std::string& h = "", int p = 0, int t = 0)
        : host( h), pid( p), transport( t)
      { }
      std::string host;
      int         pid;
      int         transport;
      bool operator<( const TransportKey& rhs) const;
    };

    /// Uniquely identify publishers and subscribers.
    struct InstanceKey {
      InstanceKey(
        const OpenDDS::DCPS::GUID_t g = OpenDDS::DCPS::GUID_UNKNOWN,
        const DDS::InstanceHandle_t h = 0
      ) : guid( g), handle( h)
      { }
      OpenDDS::DCPS::GUID_t guid;
      DDS::InstanceHandle_t handle;
      bool operator<( const InstanceKey& rhs) const;
    };

    /**
     * @brief Obtain a PID TreeNode from a key value.  Possibly create
     *        one if needed.
     *
     * @param key    key of node to find or possibly create
     * @param create boolean value indicating whether to create a node if
     *               one is not found.  It is set before returning to
     *               indicate whether a node was created (true) or found
     *               (false).
     * @return pointer to the node representing the key value.
     */
    TreeNode* getProcessNode( const ProcessKey& key, bool& create);

    /**
     * @brief Obtain a Transport TreeNode from a key value.  Possibly
     *        create one if needed.
     *
     * @param key    key of node to find or possibly create
     * @param create boolean value indicating whether to create a node if
     *               one is not found.  It is set before returning to
     *               indicate whether a node was created (true) or found
     *               (false).
     * @return pointer to the node representing the key value.
     */
    TreeNode* getTransportNode(
                const TransportKey& key,
                bool&               create
              );

    /**
     * @brief Obtain a possibly new DomainParticipant node.  Possibly
     *        create on if needed.
     *
     * @param pid    key of parent node
     * @param key    key of node to find or possibly create
     * @param create boolean value indicating whether to create a node if
     *               one is not found.  It is set before returning to
     *               indicate whether a node was created (true) or found
     *               (false).
     * @return pointer to the node representing the key value.
     */
    TreeNode* getParticipantNode(
                const ProcessKey&            pid,
                const OpenDDS::DCPS::GUID_t& id,
                bool&                        create
              );

    /**
     * @brief Create a DomainParticipant TreeNode from a key value.
     *
     * @param key    key of node to create
     * @param create boolean value indicating whether a parent node
     *               should be created if one is not found.  It is set
     *               before returning to indicate wheter a parent was
     *               created.
     * @return pointer to the new node.
     */
    TreeNode* createParticipantNode(
      const ProcessKey&            key,
      const OpenDDS::DCPS::GUID_t& id,
      bool&                        create
    );

    /**
     * @brief Obtain a possibly new instance handle node.  Possibly
     *        create on if needed.
     *
     * @param pid    key of parent node
     * @param key    key of node to find or possibly create
     * @param create boolean value indicating whether to create a node if
     *               one is not found.  It is set before returning to
     *               indicate whether a node was created (true) or found
     *               (false).
     * @return pointer to the node representing the key value.
     */
    TreeNode* getInstanceNode(
                const std::string& label,
                const InstanceKey& key,
                bool&              create
              );

    /**
     * @brief Obtain a possibly new endpoint handle node.  Possibly
     *        create on if needed.
     *
     * @param label  label string for the node if it is created.
     * @param key    key of node to find or possibly create.
     * @param create boolean value indicating whether to create a node if
     *               one is not found.  It is set before returning to
     *               indicate whether a node was created (true) or found
     *               (false).
     * @return pointer to the node representing the key value.
     */
    TreeNode* getEndpointNode(
                const std::string&           label,
                const InstanceKey&           key,
                const OpenDDS::DCPS::GUID_t& id,
                bool&                        create
              );

    /**
     * @brief Obtain a possibly new tree node.  Possibly create on
     *        if needed.
     *
     * @param label    label string for the node if it is created.
     * @param parentId GUID of parent node.
     * @param id       GUID of node to find or possibly create.
     * @param create   boolean value indicating whether to create a node if
     *                 one is not found.  It is set before returning to
     *                 indicate whether a node was created (true) or found
     *                 (false).
     * @return pointer to the node representing the key value.
     */
    TreeNode* getNode(
                const std::string&           label,
                const OpenDDS::DCPS::GUID_t& parentId,
                const OpenDDS::DCPS::GUID_t& id,
                bool&                        create
              );

    /**
     * @brief Find or create a node in which to place Qos value nodes.
     *
     * @param label   label string for the node if it is created.
     * @param key     key of node to find or possibly create.
     * @param created indicates whether a new node was created.
     * @return pointer to the node containing the Qos value nodes.
     */
    TreeNode* createQosNode(
      const std::string& label,
      TreeNode*          node,
      bool&              created
    );

    /// Manage the QoS policy values for a DomainParticipant.
    void manageParticipantQos(
           TreeNode*                               parent,
           const DDS::ParticipantBuiltinTopicData& data,
           bool&                                   layoutChanged,
           bool&                                   dataChanged
         );

    /// Manage the QoS policy values for a Topic.
    void manageTopicQos(
           TreeNode*                         parent,
           const DDS::TopicBuiltinTopicData& data,
           bool&                             layoutChanged,
           bool&                             dataChanged
         );

    /// Manage the QoS policy values for a Publication.
    void managePublicationQos(
           TreeNode*                               parent,
           const DDS::PublicationBuiltinTopicData& data,
           bool&                                   layoutChanged,
           bool&                                   dataChanged
         );

    /// Manage the QoS policy values for a Subscription.
    void manageSubscriptionQos(
           TreeNode*                                parent,
           const DDS::SubscriptionBuiltinTopicData& data,
           bool&                                    layoutChanged,
           bool&                                    dataChanged
         );

    /// Manage the link to a transport node reference for a publisher or
    /// subscriber node.
    void manageTransportLink(
           TreeNode* node,
           int       transport_id,
           bool&     create
         );

    /// Manage the link to a topic node reference for an endpoint.
    void manageTopicLink(
           TreeNode*                    node,
           const OpenDDS::DCPS::GUID_t& dp_id,
           const OpenDDS::DCPS::GUID_t& topic_id,
           bool&                        create
         );

    /// Display Name/Value pairs in the tree.  Notify the GUI if the
    /// layout or data has changed as well.
    void displayNvp(
           TreeNode*                    node,
           const OpenDDS::DCPS::NVPSeq& nvp,
           bool                         layoutChanged,
           bool                         dataChanged
         );

    /// Reference to the model.
    MonitorData* model_;

    /// Map GUID_t values to TreeNode elements.
    typedef
      std::map< OpenDDS::DCPS::GUID_t, RowNodePair, GUID_tKeyLessThan>
      GuidToTreeMap;
    GuidToTreeMap guidToTreeMap_;

    /// Map host names to TreeNode elements.
    typedef std::map< std::string, RowNodePair> HostToTreeMap;
    HostToTreeMap hostToTreeMap_;

    /// Map process identifiers to TreeNode elements.
    typedef std::map< ProcessKey, RowNodePair> ProcessToTreeMap;
    ProcessToTreeMap processToTreeMap_;

    /// Map participant/handle identifiers to TreeNode elements.
    typedef std::map< InstanceKey, RowNodePair> InstanceToTreeMap;
    InstanceToTreeMap instanceToTreeMap_;

    /// Map transport identifiers to TreeNode elements.
    typedef std::map< TransportKey, RowNodePair> TransportToTreeMap;
    TransportToTreeMap transportToTreeMap_;

    /// Active repository IOR.
    std::string activeIor_;

    // for testing
    static bool b1;
    static bool b2;
    static bool b3;
    static bool b4;
};

#include "MonitorDataStorage.inl"

} // End of namespace Monitor

#endif /* MONITORDATASTORAGE_H */

