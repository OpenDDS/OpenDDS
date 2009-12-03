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

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s ServiceParticipantReport, host: %C, pid: %d.\n"),
    remove? "removing": "processing",
    (const char*)data.host,
    data.pid
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = false;
  bool dataChanged   = false;

  // HOST

  std::string host( data.host);
  TreeNode* hostNode = 0;
  HostToTreeMap::iterator hostLocation
    = this->hostToTreeMap_.find( host);
  if( hostLocation == this->hostToTreeMap_.end()) {
    // We are done if we are removing.
    if( remove) return;

    // We need to add a new host.  Host nodes are children of the
    // root node.
    TreeNode* root = this->model_->modelRoot();

    // Host first.
    QList<QVariant> list;
    list << QString("Host") << QString( data.host);
    hostNode = new TreeNode( list, root);
    root->append( hostNode);
    layoutChanged = true;

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
    list << QString("Process") << QString::number(data.pid);
    pidNode = new TreeNode( list, hostNode);
    hostNode->append( pidNode);
    layoutChanged = true;

    // Install the new node.
    this->processToTreeMap_[ key]
      = std::make_pair( pidNode->row(), pidNode);

  } else {
    pidNode = pidLocation->second.second;
  }

  if( remove) {
    // Descend from the pidNode and remove it and all its children from
    // the maps.
    this->cleanMaps( pidNode);
    this->processToTreeMap_.erase( pidLocation);
    hostNode->removeChildren( pidNode->row(), 1);

    // Check and remove the host node if there are no pid nodes remaining
    // after the removal (no children).
    if( hostNode->size() == 0) {
      this->hostToTreeMap_.erase( hostLocation);
      delete hostNode;
    }

    // Nothing else to do on removal.
    this->model_->changed();
    return;
  }

  // PARTICIPANTS
  // NOTE: The following makes sure that any new DomainParticipants are
  //       added to the host/pid as they are received by this update.  It
  //       does *not* remove any deleted participants.  This is left for
  //       the DomainParticipantReport updates.
  int size = data.domain_participants.length();
  for( int index = 0; index < size; ++index) {
    const OpenDDS::DCPS::GUID_t& id = data.domain_participants[ index];
    GuidToTreeMap::iterator location = this->guidToTreeMap_.find( id);
    if( location == this->guidToTreeMap_.end()) {
      // We need to add this participant.
      QList<QVariant> list;
      OpenDDS::DCPS::GuidConverter converter( id);
      list << QString("Participant")
           << QString( std::string(converter).c_str());
      TreeNode* node = new TreeNode( list, pidNode);
      pidNode->append( node);
      layoutChanged = true;

      // Install the new node.
      this->guidToTreeMap_[ id]
        = std::make_pair( node->row(), node);

    // } else {
      // This participant is already loaded, we can ignore it here since
      // we have nothing to update from this data sample.
    }
  }

  // TRANSPORTS
  size = data.transports.length();
  for( int index = 0; index < size; ++index) {
    int transport = data.transports[ index];
    TransportKey key( host, data.pid, transport);
    TransportToGuidMap::iterator guidLocation
      = this->transportToGuidMap_.find( key);
    if( guidLocation == this->transportToGuidMap_.end()) {
      // This transport needs to be installed.
      OpenDDS::DCPS::GUID_t id = this->transportIdGenerator_->next();
      this->transportToGuidMap_[ key] = id;

      QList<QVariant> list;
      OpenDDS::DCPS::GuidConverter converter( id);
      list << QString("Transport")
           << QString::number( transport);
      TreeNode* node = new TreeNode( list, pidNode);
      pidNode->append( node);
      layoutChanged = true;

      // Install the new node.
      this->guidToTreeMap_[ id]
        = std::make_pair( node->row(), node);

    // } else {
      // This transport is already loaded, we can ignore it here since
      // we have nothing to update from this data sample.
    }
  }

  // NAME / VALUE DATA

  // Notify the GUI if we have changed the underlying model.
  if( layoutChanged) {
    /// @TODO: Check that we really do not need to do updated here.
    this->model_->changed();

  } else if( dataChanged) {
    this->model_->updated( pidNode, 1, (*pidNode)[ pidNode->size()-1], 1);
  }
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DomainParticipantReport>(
  const OpenDDS::DCPS::DomainParticipantReport& data,
  bool remove
)
{
  // struct DomainParticipantReport {
  //   GUID_t           dp_id;
  //   DDS::DomainId_t  domain_id;
  //   NVPSeq           values;
  // };

  OpenDDS::DCPS::GuidConverter converter( data.dp_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s DomainParticipantReport, id: %C, domain: data.domain_id.\n"),
    remove? "removing": "processing",
    std::string(converter).c_str(),
    data.domain_id
  ));
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::TopicReport>(
  const OpenDDS::DCPS::TopicReport& data,
  bool remove
)
{
  // struct TopicReport {
  //   GUID_t  topic_id;
  //   string  topic_name;
  //   string  type_name;
  //   NVPSeq  values;
  // };

  OpenDDS::DCPS::GuidConverter converter( data.topic_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s TopicReport, id: %C, name: %C, type: %C.\n"),
    remove? "removing": "processing",
    std::string(converter).c_str(),
    (const char*)data.topic_name,
    (const char*)data.type_name
  ));
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::PublisherReport>(
  const OpenDDS::DCPS::PublisherReport& data,
  bool remove
)
{
  // struct PublisherReport {
  //   DDS::InstanceHandle_t handle;
  //   GUID_t        dp_id;
  //   unsigned long transport_id;
  //   GUIDSeq       writers;
  //   NVPSeq        values;
  // };

  OpenDDS::DCPS::GuidConverter converter( data.dp_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s PublisherReport, id: %C, handle: %d, transport: %d.\n"),
    remove? "removing": "processing",
    std::string(converter).c_str(),
    data.handle,
    data.transport_id
  ));
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::SubscriberReport>(
  const OpenDDS::DCPS::SubscriberReport& data,
  bool remove
)
{
  // struct SubscriberReport {
  //   DDS::InstanceHandle_t handle;
  //   GUID_t        dp_id;
  //   unsigned long transport_id;
  //   GUIDSeq       readers;
  //   NVPSeq        values;
  // };

  OpenDDS::DCPS::GuidConverter converter( data.dp_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s SubscriberReport, id: %C, handle: %d, transport: %d.\n"),
    remove? "removing": "processing",
    std::string(converter).c_str(),
    data.handle,
    data.transport_id
  ));
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataWriterReport>(
  const OpenDDS::DCPS::DataWriterReport& data,
  bool remove
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

  OpenDDS::DCPS::GuidConverter idconverter( data.dw_id);
  OpenDDS::DCPS::GuidConverter topicconverter( data.topic_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s DataWriterReport, id: %C, topic: %C.\n"),
    remove? "removing": "processing",
    std::string(idconverter).c_str(),
    std::string(topicconverter).c_str()
  ));
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataWriterPeriodicReport>(
  const OpenDDS::DCPS::DataWriterPeriodicReport& data,
  bool remove
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

  OpenDDS::DCPS::GuidConverter converter( data.dw_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s DataWriterPeriodicReport, id: %C.\n"),
    remove? "removing": "processing",
    std::string(converter).c_str()
  ));
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataReaderReport>(
  const OpenDDS::DCPS::DataReaderReport& data,
  bool remove
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

  OpenDDS::DCPS::GuidConverter idconverter( data.dr_id);
  OpenDDS::DCPS::GuidConverter topicconverter( data.topic_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s DataReaderReport, id: %C, topic: %C.\n"),
    remove? "removing": "processing",
    std::string(idconverter).c_str(),
    std::string(topicconverter).c_str()
  ));
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataReaderPeriodicReport>(
  const OpenDDS::DCPS::DataReaderPeriodicReport& data,
  bool remove
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

  OpenDDS::DCPS::GuidConverter converter( data.dr_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s DataReaderPeriodicReport, id: %C.\n"),
    remove? "removing": "processing",
    std::string(converter).c_str()
  ));
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::TransportReport>(
  const OpenDDS::DCPS::TransportReport& data,
  bool remove
)
{
  // struct TransportReport {
  //   string        host;
  //   long          pid;
  //   unsigned long transport_id;
  //   string        transport_type;
  //   NVPSeq        values;
  // };

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s TransportReport, host: %C, pid: %d, ")
    ACE_TEXT("transport: %d, type: %C.\n"),
    remove? "removing": "processing",
    (const char*)data.host,
    data.pid,
    data.transport_id,
    (const char*)data.transport_type
  ));
}

