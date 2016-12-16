/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef MONITORDATA_H
#define MONITORDATA_H

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace DCPS {
  struct GUID_t;
  struct ServiceParticipantReport;
  struct DomainParticipantReport;
  struct TopicReport;
  struct PublisherReport;
  struct SubscriberReport;
  struct DataWriterReport;
  struct DataWriterPeriodicReport;
  struct DataReaderReport;
  struct DataReaderPeriodicReport;
  struct TransportReport;

}} // End of namespace OpenDDS::DCPS

namespace DDS {
  struct ParticipantBuiltinTopicData;
  struct TopicBuiltinTopicData;
  struct PublicationBuiltinTopicData;
  struct SubscriptionBuiltinTopicData;

} // End of namespace DDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

class QString;
template< class T> class QList;

namespace Monitor {

class Options;
class MonitorDataModel;
class MonitorDataStorage;
class MonitorTask;
class TreeNode;

/**
 * @class MonitorData
 *
 * brief mediate the interaction between the GUI and the DDS service.
 *
 * This class provides the mechanism for the GUI to send messages to the
 * DDS service and the service to send messages to the GUI.  This class
 * also stores the data used by the data model so that it can be accessed
 * by the GUI data model as well as by the service.
 */
class MonitorData {
  public:
    /// Construct with an IOR only.
    MonitorData( const Options& options, MonitorDataModel* model, bool mapExistingIORKeys = true);

    virtual ~MonitorData();

    /// Disable operation for orderly shutdown.
    void disable();

    /// @name Messages and queries to DDS.
    /// @{

    /// Obtain a list of existing repository IOR values.
    void getIorList( QList<QString>& iorList);

    /// Establish a binding to a repository.  There can be only one per IOR.
    bool setRepoIor( const QString& ior);

    /// Remove a binding to a repository.  There should be only one with this IOR.
    bool removeRepo( const QString& ior);

    /// Evict the currently monitored repository.
    bool clearData();

    /// Stop processing inbound instrumentation data.
    bool stopInstrumentation();

    /// @NOTE: the following methods are not templated to reduce
    ///        dependencies, since the implementation would need to be
    ///        visible from this header.

    /// Obtain Builtin Topic data for a Participant.
    bool getBuiltinTopicData(
           const OpenDDS::DCPS::GUID_t&      id,
           DDS::ParticipantBuiltinTopicData& data
         );

    /// Obtain Builtin Topic data for a Topic.
    bool getBuiltinTopicData(
           const OpenDDS::DCPS::GUID_t& id,
           DDS::TopicBuiltinTopicData&  data
         );

    /// Obtain Builtin Topic data for a Publication.
    bool getBuiltinTopicData(
           const OpenDDS::DCPS::GUID_t&      id,
           DDS::PublicationBuiltinTopicData& data
         );

    /// Obtain Builtin Topic data for a Subscription.
    bool getBuiltinTopicData(
           const OpenDDS::DCPS::GUID_t&       id,
           DDS::SubscriptionBuiltinTopicData& data
         );

    /// @}

    /// @name Messages and queries to GUI.
    /// @{

    /// Grab the root of the model tree.
    TreeNode* modelRoot();

    /// Update data values for a single node.
    void updated( TreeNode* node, int column);

    /// Update data values for a range of nodes.
    void updated( TreeNode* left, int lcol, TreeNode* right, int rcol);

    /// Modify the layout of the GUI.
    void changed();

    /// @}

  private:

    /// Enabled flag.
    bool enabled_;

    /// Configuration information.
    const Options& options_;

    /// The GUI model.
    MonitorDataModel* model_;

    /// The DDS data source.
    MonitorTask* dataSource_;

    /// The instrumentation data storage.
    MonitorDataStorage* storage_;
};

} // End of namespace Monitor

#endif /* MONITORDATA_H */
