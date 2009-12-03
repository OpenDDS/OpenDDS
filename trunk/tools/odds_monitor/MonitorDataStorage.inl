/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

template< typename DataType>
inline
void
MonitorDataStorage::update( const DataType&, bool)
{
  // Error condition.
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::ServiceParticipantReport>(
  const OpenDDS::DCPS::ServiceParticipantReport& data,
  bool remove
)
{
  // struct ServiceParticipantReport {
  //   string    host;
  //   long      pid;
  //   GUIDSeq   domain_participants;
  //   ULongSeq  transports;
  //   NVPSeq    values;
  // };

  // HOST

  std::string host( data.host);
  TreeNode* hostNode = 0;
  HostToTreeMap::iterator hostLocation
    = this->hostToTreeMap_.find( std::string( host));
  if( hostLocation == this->hostToTreeMap_.end()) {
    // We are done if we are removing.
    if( remove) return;

    // We need to add a new host.  Host nodes are children of the
    // root node.
    TreeNode* root = this->model_->modelRoot();

    // Host first.
    QList<QVariant> list;
    list << QString("HOST") << QString( data.host);
    hostNode = new TreeNode( list, root);
    root->append( hostNode);

    // Install the new node.
    this->hostToTreeMap_[ host]
      = std::make_pair( hostNode->row(), hostNode);

  } else {
    // Retain the current host node.
    hostNode = hostLocation->second.second;
  }

  // PROCESS

  ProcessKey key( host, data.pid);
  TreeNode* pidNode = 0;
  ProcessToTreeMap::iterator pidLocation
    = this->processToTreeMap_.find( key);
  if( pidLocation == this->processToTreeMap_.end()) {
    // We are done if we are removing.
    if( remove) return;

    // We need to add a new PID.  PID nodes are children of the host
    // nodes.  We just found the relevant host node.

    // PID data.
    QList<QVariant> list;
    list << QString("Process") << data.pid;
    pidNode = new TreeNode( list, hostNode);
    hostNode->append( pidNode);

    // Install the new node.
    this->processToTreeMap_[ key]
      = std::make_pair( pidNode->row(), pidNode);

  } else {
    pidNode = pidLocation->second.second;
  }

  if( remove) {
    // Need to build a reverse index to do this efficiently.
    // Or maybe get the QModelIndex and recursively descend from there
    // to find the TreeNodes to remove.  I guess that still leaves us
    // with needing to find them in the maps for removal.

    /// @TODO: implement this.
    return;
  }

  // PARTICIPANTS

  // TRANSPORTS

  // KEY VALUE DATA

}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DomainParticipantReport>(
  const OpenDDS::DCPS::DomainParticipantReport& /* data */,
  bool /* remove */
)
{
  // struct DomainParticipantReport {
  //   GUID_t           dp_id;
  //   DDS::DomainId_t  domain_id;
  //   NVPSeq           values;
  // };
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::TopicReport>(
  const OpenDDS::DCPS::TopicReport& /* data */,
  bool /* remove */
)
{
  // struct TopicReport {
  //   GUID_t  topic_id;
  //   string  topic_name;
  //   string  type_name;
  //   NVPSeq  values;
  // };
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::PublisherReport>(
  const OpenDDS::DCPS::PublisherReport& /* data */,
  bool /* remove */
)
{
  // struct PublisherReport {
  //   DDS::InstanceHandle_t handle;
  //   GUID_t        dp_id;
  //   unsigned long transport_id;
  //   GUIDSeq       writers;
  //   NVPSeq        values;
  // };
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::SubscriberReport>(
  const OpenDDS::DCPS::SubscriberReport& /* data */,
  bool /* remove */
)
{
  // struct SubscriberReport {
  //   DDS::InstanceHandle_t handle;
  //   GUID_t        dp_id;
  //   unsigned long transport_id;
  //   GUIDSeq       readers;
  //   NVPSeq        values;
  // };
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataWriterReport>(
  const OpenDDS::DCPS::DataWriterReport& /* data */,
  bool /* remove */
)
{
  // struct DataWriterAssociation {
  //   GUID_t        dr_id;
  // };
  // typedef sequence<DataWriterAssociation> DWAssociations;
  // struct DataWriterReport {
  //   GUID_t         dw_id;
  //   GUID_t         topic_id;
  //   DDS::InstanceHandleSeq instances;
  //   DWAssociations associations;
  //   NVPSeq         values;
  // };
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataWriterPeriodicReport>(
  const OpenDDS::DCPS::DataWriterPeriodicReport& /* data */,
  bool /* remove */
)
{
  // struct DataWriterAssociationPeriodic {
  //   GUID_t        dr_id;
  //   unsigned long sequence_number;
  // };
  // typedef sequence<DataWriterAssociationPeriodic> DWAssociationsPeriodic;
  // struct DataWriterPeriodicReport {
  //   GUID_t        dw_id;
  //   unsigned long data_dropped_count;
  //   unsigned long data_delivered_count;
  //   unsigned long control_dropped_count;
  //   unsigned long control_delivered_count;
  //   DWAssociationsPeriodic associations;
  //   NVPSeq        values;
  // };
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataReaderReport>(
  const OpenDDS::DCPS::DataReaderReport& /* data */,
  bool /* remove */
)
{
  // struct DataReaderAssociation {
  //   GUID_t        dw_id;
  //   short         state;
  // };
  // typedef sequence<DataReaderAssociation> DRAssociations;
  // struct DataReaderReport {
  //   GUID_t         dr_id;
  //   GUID_t         topic_id;
  //   GUIDSeq        instances;
  //   DRAssociations associations;
  //   NVPSeq         values;
  // };
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataReaderPeriodicReport>(
  const OpenDDS::DCPS::DataReaderPeriodicReport& /* data */,
  bool /* remove */
)
{
  // struct DataReaderAssociationPeriodic {
  //   GUID_t        dw_id;
  //   unsigned long samples_available;
  //   // Stats      latency_stats;
  // };
  // typedef sequence<DataReaderAssociationPeriodic> DRAssociationsPeriodic;
  // struct DataReaderPeriodicReport {
  //   GUID_t        dr_id;
  //   DRAssociationsPeriodic associations;
  //   NVPSeq        values;
  // };
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::TransportReport>(
  const OpenDDS::DCPS::TransportReport& /* data */,
  bool /* remove */
)
{
  // struct TransportReport {
  //   string        host;
  //   long          pid;
  //   unsigned long transport_id;
  //   string        transport_type;
  //   NVPSeq        values;
  // };
}

