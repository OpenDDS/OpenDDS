/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef MONITORDATA_H
#define MONITORDATA_H

namespace OpenDDS { namespace DCPS {
  class GUID_t;
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
    MonitorData( const Options& options, MonitorDataModel* model);

    /// Virtual destructor.
    virtual ~MonitorData();

    /// Disable operation for orderly shutdown.
    void disable();

    /// @name Messages and queries from GUI to DDS.
    /// @{

    /// Obtain a list of existing repository IOR values.
    void getIorList( QList<QString>& iorList);

    /// Establish a binding to a repository.  There can be only one per IOR.
    bool setRepoIor( const QString& ior);

    /// Remove a binding to a repository.  There should be only one with this IOR.
    bool removeRepo( const QString& ior);

    /// Evict the currently monitored repository.
    bool clearData();

    /// @}

    /// @name Messages and queries from DDS to GUI.
    /// @{

    /// Grab the root of the model tree.
    TreeNode* modelRoot();

    /// Update data values for a single node.
    void updated( TreeNode* node, int column);

    /// Update data values for a range of nodes.
    void updated( TreeNode* left, int lcol, TreeNode* right, int rcol);

    /// Modify the layout of the GUI.
    void changed();
 
    /// Add a new data element to the model.
    void addDataElement(
           const OpenDDS::DCPS::GUID_t& parent,
           const OpenDDS::DCPS::GUID_t& id,
           char* value
         );
 
    /// Update the value of a data element in the model.
    void updateDataElement(
           const OpenDDS::DCPS::GUID_t& id,
           char* value
         );

    /// Update or remove a data element in the model.
    template< typename DataType>
    void update( const DataType& data, bool remove = false);

    /// Remove a data element from the model.
    void removeDataElement( const OpenDDS::DCPS::GUID_t& id);

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

