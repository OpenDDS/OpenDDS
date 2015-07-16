/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

template< class MapType>
std::pair< bool, typename MapType::key_type>
Monitor::MonitorDataStorage::findKey( MapType& map, TreeNode* node)
{
  // This search is predicated on a node only being present once in any
  // tree.
  // Need to build a reverse index to do this efficiently.
  for( typename MapType::iterator current = map.begin();
       current != map.end();
       ++current) {
    if( node == current->second.second) {
      return std::make_pair( true, current->first);
    }
  }
  return std::make_pair( false, typename MapType::key_type());
}

template< class MapType>
void
MonitorDataStorage::deleteNode( MapType& map, TreeNode* node)
{
  std::pair< bool, typename MapType::key_type> result
    = this->findKey( map, node);
  if( result.first) {
    map.erase( result.second);
  }

  TreeNode* parent = node->parent();
  if( parent) {
    parent->removeChildren( node->row(), 1);

  } else {
    delete node;
  }

  // Notify the GUI to update.
  this->model_->changed();
}

template< class PolicyType>
bool
MonitorDataStorage::manageQosPolicy(
  TreeNode*         node,
  const QString&    label,
  const PolicyType& value
)
{
  TreeNode* child = 0;
  return this->manageChildValue( node, child, label, QosToQString( value));
}

template<typename DataType>
inline
void
MonitorDataStorage::update(const DataType&, DDS::DomainParticipant_ptr, bool)
{
  // Error condition.
  ACE_ERROR((LM_ERROR,
    ACE_TEXT("(%P|%t) ERROR: MonitorDataStorage::update() - ")
    ACE_TEXT("recieved sample of unknown data type.\n")
  ));
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::ServiceParticipantReport>(
  const OpenDDS::DCPS::ServiceParticipantReport& data,
  DDS::DomainParticipant_ptr,
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
  bool create        = !remove;

  // Obtain, or possibly create, the host and PID nodes.
  std::string host( data.host);
  ProcessKey pid( host, data.pid);
  TreeNode* pidNode = this->getProcessNode( pid, create);
  if( !pidNode) {
    return;
  }
  layoutChanged |= create;

  if( remove) {
    this->deleteProcessNode( pidNode);
    return;
  }

  // PARTICIPANTS
  // NOTE: The following makes sure that any new DomainParticipants are
  //       added to the host/pid as they are received by this update.  It
  //       does *not* remove any deleted participants.  This is left for
  //       the DomainParticipantReport updates.
  int size = data.domain_participants.length();
  for( int index = 0; index < size; ++index) {
    create = true;
    (void)this->getParticipantNode(
      pid,
      data.domain_participants[ index],
      create
    );
    layoutChanged |= create;
  }

  // TRANSPORTS
  size = data.transports.length();
  for( int index = 0; index < size; ++index) {
    create = true;
    int transport = data.transports[ index];
    TransportKey key( host, data.pid, transport);
    (void)this->getTransportNode( key, create);
    layoutChanged |= create;
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( pidNode, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::DomainParticipantReport>(
  const OpenDDS::DCPS::DomainParticipantReport& data,
  DDS::DomainParticipant_ptr,
  bool remove
)
{
  //  struct DomainParticipantReport {
  //    string           host;
  //    long             pid;
  //    GUID_t           dp_id;
  //    DDS::DomainId_t  domain_id;
  //    GUIDSeq          topics;
  //    NVPSeq           values;
  //  };

  OpenDDS::DCPS::GuidConverter converter( data.dp_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s DomainParticipantReport, id: %C, domain: %d.\n"),
    remove? "removing": "processing",
    (data.dp_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str(),
    data.domain_id
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = !remove; // Updated by getParticipantNode()
  bool dataChanged   = false;

  std::string host( data.host);
  ProcessKey pid( host, data.pid);
  TreeNode* node = this->getParticipantNode( pid, data.dp_id, layoutChanged);
  if( !node) {
    return;
  }

  if( remove) {
    this->deleteNode( this->guidToTreeMap_, node);
    return;
  }

  // QOS
  DDS::ParticipantBuiltinTopicData qosData;
  if (this->model_->getBuiltinTopicData( data.dp_id, qosData)) {
    this->manageParticipantQos( node, qosData, layoutChanged, dataChanged);
  }

  // Domain Id value.
  QString label( QObject::tr( "Domain Id"));
  QString value = QString::number( data.domain_id);
  TreeNode* child = 0;
  if( this->manageChildValue( node, child, label, value)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  // TOPICS
  // NOTE: The following makes sure that any new Topics are added to
  //       the DomainParticipant as they are received by this update.
  //       It does *not* remove any deleted topics.  This is left for
  //       the TopicReport updates.
  int size = data.topics.length();
  for( int index = 0; index < size; ++index) {
    bool create = true;
    (void)this->getNode(
      std::string( "Topic"),
      data.dp_id,
      data.topics[ index],
      create
    );
    layoutChanged |= create;
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::TopicReport>(
  const OpenDDS::DCPS::TopicReport& data,
  DDS::DomainParticipant_ptr,
  bool remove
)
{
  //  struct TopicReport {
  //    GUID_t  dp_id;
  //    GUID_t  topic_id;
  //    string  topic_name;
  //    string  type_name;
  //    NVPSeq  values;
  //  };

  OpenDDS::DCPS::GuidConverter converter( data.topic_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s TopicReport, id: %C, name: %C, type: %C.\n"),
    remove? "removing": "processing",
    (data.topic_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str(),
    (const char*)data.topic_name,
    (const char*)data.type_name
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = !remove; // Updated by getNode()
  bool dataChanged   = false;

  // Find or create this topic node.
  TreeNode* node = this->getNode(
                     std::string("Topic"),
                     data.dp_id,
                     data.topic_id,
                     layoutChanged
                   );
  if( !node) {
    return;
  }

  if( remove) {
    this->deleteNode( this->guidToTreeMap_, node);
    return;
  }

  // QOS
  DDS::TopicBuiltinTopicData qosData;
  if (this->model_->getBuiltinTopicData( data.topic_id, qosData)) {
    this->manageTopicQos( node, qosData, layoutChanged, dataChanged);
  }

  // Topic name value.
  TreeNode* nameNode = 0;
  QString nameLabel( QObject::tr( "Topic Name"));
  QString nameValue( QObject::tr( static_cast<const char*>(data.topic_name)));
  if( this->manageChildValue( node, nameNode, nameLabel, nameValue)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  // Move any references to this topic node to the more useful name node
  // just obtained.
  if (node->reassignValueRefs(nameNode)) {
    nameNode->setColor(1, QColor("#bfffbf"));
    dataChanged = true;
  }

  // Data type value.
  TreeNode* child = 0;
  QString typeLabel( QObject::tr( "Data Type"));
  QString typeValue( QObject::tr( static_cast<const char*>(data.type_name)));
  if( this->manageChildValue( node, child, typeLabel, typeValue)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::PublisherReport>(
  const OpenDDS::DCPS::PublisherReport& data,
  DDS::DomainParticipant_ptr,
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
    ACE_TEXT("%s PublisherReport, id: %C, handle: %d, transport: 0x%x.\n"),
    remove? "removing": "processing",
    (data.dp_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str(),
    data.handle,
    data.transport_id
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool create        = !remove;
  bool layoutChanged = false;
  bool dataChanged   = false;

  InstanceKey key( data.dp_id, data.handle);
  TreeNode* node = this->getInstanceNode(
                     std::string( "Publisher"),
                     key,
                     create
                   );
  if( !node) {
    return;
  }
  layoutChanged |= create;

  if( remove) {
    this->deleteNode( this->instanceToTreeMap_, node);
    return;
  }

  // TRANSPORT
  create = true;
  if (data.transport_id != 0) {
    this->manageTransportLink( node, data.transport_id, create);
  }
  layoutChanged |= create;

  // WRITERS
  // NOTE: The following makes sure that any new DataWriters are added to
  //       the Publisher as they are received by this update.  It does
  //       *not* remove any deleted writers.  This is left for the
  //       DataWriterReport updates.
  int size = data.writers.length();
  for( int index = 0; index < size; ++index) {
    bool create = true;
    (void)this->getEndpointNode(
      std::string( "Writer"),
      key,
      data.writers[ index],
      create
    );
    layoutChanged |= create;
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::SubscriberReport>(
  const OpenDDS::DCPS::SubscriberReport& data,
  DDS::DomainParticipant_ptr,
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
    ACE_TEXT("%s SubscriberReport, id: %C, handle: %d, transport: 0x%x.\n"),
    remove? "removing": "processing",
    (data.dp_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str(),
    data.handle,
    data.transport_id
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool create        = !remove;
  bool layoutChanged = false;
  bool dataChanged   = false;

  InstanceKey key( data.dp_id, data.handle);
  TreeNode* node = this->getInstanceNode(
                     std::string( "Subscriber"),
                     key,
                     create
                   );
  if( !node) {
    return;
  }
  layoutChanged |= create;

  if( remove) {
    this->deleteNode( this->instanceToTreeMap_, node);
    return;
  }

  // TRANSPORT
  create = true;
  if (data.transport_id != 0) {
    this->manageTransportLink( node, data.transport_id, create);
  }
  layoutChanged |= create;

  // READERS
  // NOTE: The following makes sure that any new DataReaders are added to
  //       the Subscriber as they are received by this update.  It does
  //       *not* remove any deleted writers.  This is left for the
  //       DataReaderReport updates.
  int size = data.readers.length();
  for( int index = 0; index < size; ++index) {
    create = true;
    (void)this->getEndpointNode(
      std::string( "Reader"),
      key,
      data.readers[ index],
      create
    );
    layoutChanged |= create;
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::DataWriterReport>(
  const OpenDDS::DCPS::DataWriterReport& data,
  DDS::DomainParticipant_ptr,
  bool remove
)
{
  //  struct DataWriterAssociation {
  //    GUID_t        dr_id;
  //  };
  //  typedef sequence<DataWriterAssociation> DWAssociations;
  //  struct DataWriterReport {
  //    GUID_t                 dp_id;
  //    DDS::InstanceHandle_t  pub_handle;
  //    GUID_t                 dw_id;
  //    GUID_t                 topic_id;
  //    DDS::InstanceHandleSeq instances;
  //    DWAssociations         associations;
  //    NVPSeq                 values;
  //  };

  OpenDDS::DCPS::GuidConverter idconverter( data.dw_id);
  OpenDDS::DCPS::GuidConverter topicconverter( data.topic_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s DataWriterReport, id: %C, topic: %C.\n"),
    remove? "removing": "processing",
    (data.dw_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(idconverter).c_str(),
    (data.topic_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(topicconverter).c_str()
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool create        = !remove;
  bool layoutChanged = false;
  bool dataChanged   = false;

  InstanceKey key( data.dp_id, data.pub_handle);
  TreeNode* node = this->getEndpointNode(
                     std::string( "Writer"),
                     key,
                     data.dw_id,
                     create
                   );
  if( !node) {
    return;
  }
  layoutChanged |= create;

  if( remove) {
    this->deleteNode( this->guidToTreeMap_, node);
    return;
  }

  // QOS
  DDS::PublicationBuiltinTopicData qosData;
  if (this->model_->getBuiltinTopicData( data.dw_id, qosData)) {
    this->managePublicationQos( node, qosData, layoutChanged, dataChanged);
  }

  // TOPIC
  create = true;
  this->manageTopicLink( node, data.dp_id, data.topic_id, create);
  layoutChanged |= create;

  // ASSOCIATIONS
  int size = data.associations.length();
  for( int index = 0; index < size; ++index) {
    // Create a child node to hold the association if its not already in
    // the tree.
    OpenDDS::DCPS::GuidConverter converter( data.associations[ index].dr_id);
    QString reader( std::string(converter).c_str());
    int row = node->indexOf( 1, reader);
    if( row == -1) {
      // New data, insert.
      QList<QVariant> list;
      list << QString( QObject::tr("Reader")) << reader;
      TreeNode* valueNode = new TreeNode( list, node);
      node->append( valueNode);
      layoutChanged = true;
    }
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::DataWriterPeriodicReport>(
  const OpenDDS::DCPS::DataWriterPeriodicReport& data,
  DDS::DomainParticipant_ptr,
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
    (data.dw_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str()
  ));

  // Ignore remove flag for these samples - the static reports control
  // the lifetime.
  if( remove) {
    return;
  }

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = false;
  bool dataChanged   = false;

  // Find, do not create, a writer node.
  TreeNode* node = this->getNode(
                     std::string("Writer"),
                     OpenDDS::DCPS::GUID_UNKNOWN,
                     data.dw_id,
                     layoutChanged
                   );
  if( !node) {
    return;
  }

  //   unsigned long data_dropped_count;
  TreeNode* child = 0;
  QString dataDroppedLabel( QObject::tr("data dropped"));
  QString dataDroppedValue
    = QString::number( data.data_dropped_count);
  if( this->manageChildValue(
        node, child, dataDroppedLabel, dataDroppedValue
    )) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //   unsigned long data_delivered_count;
  child = 0;
  QString dataDeliveredLabel( QObject::tr("data delivered"));
  QString dataDeliveredValue
    = QString::number( data.data_delivered_count);
  if( this->manageChildValue(
        node, child, dataDeliveredLabel, dataDeliveredValue
    )) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //   unsigned long control_dropped_count;
  child = 0;
  QString controlDroppedLabel( QObject::tr("control dropped"));
  QString controlDroppedValue
    = QString::number( data.control_dropped_count);
  if( this->manageChildValue(
        node, child, controlDroppedLabel, controlDroppedValue
    )) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //   unsigned long control_delivered_count;
  child = 0;
  QString controlDeliveredLabel( QObject::tr("control delivered"));
  QString controlDeliveredValue
    = QString::number( data.control_delivered_count);
  if( this->manageChildValue(
        node, child, controlDeliveredLabel, controlDeliveredValue
    )) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  // ASSOCIATIONS
  int size = data.associations.length();
  for( int index = 0; index < size; ++index) {
    // Create a child node to hold the association if its not already in
    // the tree.
    TreeNode* readerNode = 0;
    OpenDDS::DCPS::GuidConverter converter( data.associations[ index].dr_id);
    QString reader( std::string(converter).c_str());
    int row = node->indexOf( 1, reader);
    if( row == -1) {
      // New data, insert.
      QList<QVariant> list;
      list << QString( QObject::tr("Reader")) << reader;
      readerNode = new TreeNode( list, node);
      node->append( readerNode);
      layoutChanged = true;

    } else {
      // Existing data, use it.
      readerNode = (*node)[ row];
    }

    // Add or update the current sequence number for this association.
    TreeNode* child = 0;
    QString sequenceLabel( QObject::tr("sequence number"));
    QString sequenceValue
      = QString::number( data.associations[ index].sequence_number);
    if( this->manageChildValue(
          readerNode, child, sequenceLabel, sequenceValue
      )) {
      layoutChanged = true;

    } else {
      dataChanged = true;
    }
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::DataReaderReport>(
  const OpenDDS::DCPS::DataReaderReport& data,
  DDS::DomainParticipant_ptr,
  bool remove
)
{
  //  struct DataReaderAssociation {
  //    GUID_t        dw_id;
  //    short         state;
  //  };
  //  typedef sequence<DataReaderAssociation> DRAssociations;
  //  struct DataReaderReport {
  //    GUID_t                 dp_id;
  //    DDS::InstanceHandle_t  sub_handle;
  //    GUID_t                 dr_id;
  //    GUID_t                 topic_id;
  //    DDS::InstanceHandleSeq instances;
  //    DRAssociations         associations;
  //    NVPSeq                 values;
  //  };

  OpenDDS::DCPS::GuidConverter idconverter( data.dr_id);
  OpenDDS::DCPS::GuidConverter topicconverter( data.topic_id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s DataReaderReport, id: %C, topic: %C.\n"),
    remove? "removing": "processing",
    (data.dr_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(idconverter).c_str(),
    (data.topic_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(topicconverter).c_str()
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool create        = !remove;
  bool layoutChanged = false;
  bool dataChanged   = false;

  InstanceKey key( data.dp_id, data.sub_handle);
  TreeNode* node = this->getEndpointNode(
                     std::string( "Reader"),
                     key,
                     data.dr_id,
                     create
                   );
  if( !node) {
    return;
  }
  layoutChanged |= create;

  if( remove) {
    this->deleteNode( this->guidToTreeMap_, node);
    return;
  }

  // QOS
  DDS::SubscriptionBuiltinTopicData qosData;
  if (this->model_->getBuiltinTopicData( data.dr_id, qosData)) {
    this->manageSubscriptionQos( node, qosData, layoutChanged, dataChanged);
  }

  // TOPIC
  create = true;
  this->manageTopicLink( node, data.dp_id, data.topic_id, create);
  layoutChanged |= create;

  // ASSOCIATIONS
  int size = data.associations.length();
  for( int index = 0; index < size; ++index) {
    // Create a child node to hold the association if its not already in
    // the tree.
    OpenDDS::DCPS::GuidConverter converter( data.associations[ index].dw_id);
    QString writer( std::string(converter).c_str());
    int row = node->indexOf( 1, writer);
    if( row == -1) {
      // New data, insert.
      QList<QVariant> list;
      list << QString( QObject::tr("Writer")) << writer;
      TreeNode* valueNode = new TreeNode( list, node);
      node->append( valueNode);
      layoutChanged = true;
    }
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::DataReaderPeriodicReport>(
  const OpenDDS::DCPS::DataReaderPeriodicReport& data,
  DDS::DomainParticipant_ptr,
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
    (data.dr_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str()
  ));

  // Ignore remove flag for these samples - the static reports control
  // the lifetime.
  if( remove) {
    return;
  }

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = false;
  bool dataChanged   = false;

  // Find, do not create, a reader node.
  TreeNode* node = this->getNode(
                     std::string("Reader"),
                     OpenDDS::DCPS::GUID_UNKNOWN,
                     data.dr_id,
                     layoutChanged
                   );
  if( !node) {
    return;
  }

  // ASSOCIATIONS
  int size = data.associations.length();
  for( int index = 0; index < size; ++index) {
    // Create a child node to hold the association if its not already in
    // the tree.
    TreeNode* writerNode = 0;
    OpenDDS::DCPS::GuidConverter converter( data.associations[ index].dw_id);
    QString writer( std::string(converter).c_str());
    int row = node->indexOf( 1, writer);
    if( row == -1) {
      // New data, insert.
      QList<QVariant> list;
      list << QString( QObject::tr("Writer")) << writer;
      writerNode = new TreeNode( list, node);
      node->append( writerNode);
      layoutChanged = true;

    } else {
      // Existing data, use it.
      writerNode = (*node)[ row];
    }

    // Add or update the current number of samples available for this association.
    TreeNode* child = 0;
    QString samplesLabel( QObject::tr("samples available"));
    QString samplesValue
      = QString::number( data.associations[ index].samples_available);
    if( this->manageChildValue(
          writerNode, child, samplesLabel, samplesValue
      )) {
      layoutChanged = true;

    } else {
      dataChanged = true;
    }
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update<OpenDDS::DCPS::TransportReport>(
  const OpenDDS::DCPS::TransportReport& data,
  DDS::DomainParticipant_ptr,
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
    ACE_TEXT("transport: 0x%x, type: %C.\n"),
    remove? "removing": "processing",
    (const char*)data.host,
    data.pid,
    data.transport_id,
    (const char*)data.transport_type
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = !remove; // Updated by getTransportNode()

  TransportKey key( std::string(data.host), data.pid, data.transport_id);
  TreeNode* node = this->getTransportNode( key, layoutChanged);
  if( !node) {
    return;
  }

  if( remove) {
    this->deleteNode( this->transportToTreeMap_, node);
    return;
  }

  // Transport type value.
  QString typeLabel( QObject::tr( "Type"));
  int row = node->indexOf( 0, typeLabel);
  if( row == -1) {
    // New data, insert.
    QList<QVariant> list;
    list << typeLabel
         << QString( QObject::tr( static_cast<const char*>(data.transport_type)));
    TreeNode* nameNode = new TreeNode( list, node);
    node->append( nameNode);
    layoutChanged = true;

  } else {
    // Existing data, update.
    TreeNode* nameNode = (*node)[ row];
    nameNode->setData(
      1,
      QString( QObject::tr( static_cast<const char*>(data.transport_type)))
    );
  }
}

template<>
inline
void
MonitorDataStorage::update<DDS::ParticipantBuiltinTopicData>(
  const DDS::ParticipantBuiltinTopicData& data,
  DDS::DomainParticipant_ptr participant,
  bool remove
)
{
  //  struct ParticipantBuiltinTopicData {
  //    BuiltinTopicKey_t key;
  //    UserDataQosPolicy user_data;
  //  };

  // Extract a GUID from the key.
  OpenDDS::DCPS::Discovery_rch disc =
    TheServiceParticipant->get_discovery(participant->get_domain_id());
  OpenDDS::DCPS::DomainParticipantImpl* dpi =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant);
  OpenDDS::DCPS::RepoId id =
    disc->bit_key_to_repo_id(dpi,
                             OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
                             data.key);

  OpenDDS::DCPS::GuidConverter converter(id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s Participant Builtin Topic %C, key: ")
    ACE_TEXT("[0x%x, 0x%x, 0x%x].\n"),
    remove? "removing": "processing",
    (id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str(),
    data.key.value[0], data.key.value[1], data.key.value[2]
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = false;
  bool dataChanged   = false;

  // Find or create a DomainParticipant node for this data.
  bool create = false;
  TreeNode* node = this->getParticipantNode( ProcessKey(), id, create);
  if( !node) {
    return;
  }

  if( remove) {
    // Indicate visually that this participant has been destroyed.
    // @TODO: This should probably recursively modify the children as well.
    node->setColor(1,QColor("#ffbfbf"));

    this->model_->changed();
    return;
  }

  // Process the Qos data for this node.
  this->manageParticipantQos( node, data, layoutChanged, dataChanged);

  // Notify the GUI if we have changed the underlying model.
  if( layoutChanged) {
    /// @TODO: Check that we really do not need to do updated here.
    this->model_->changed();
    //this->model_->updated( node, 1, (*node)[ node->size()-1], 1);

  } else if( dataChanged) {
    this->model_->changed();
    //this->model_->updated( node, 1, (*node)[ node->size()-1], 1);
  }
}

template<>
inline
void
MonitorDataStorage::update<DDS::TopicBuiltinTopicData>(
  const DDS::TopicBuiltinTopicData& data,
  DDS::DomainParticipant_ptr participant,
  bool remove
)
{
  //  struct TopicBuiltinTopicData {
  //    BuiltinTopicKey_t key;
  //    string name;
  //    string type_name;
  //    DurabilityQosPolicy durability;
  //    DurabilityServiceQosPolicy durability_service;
  //    DeadlineQosPolicy deadline;
  //    LatencyBudgetQosPolicy latency_budget;
  //    LivelinessQosPolicy liveliness;
  //    ReliabilityQosPolicy reliability;
  //    TransportPriorityQosPolicy transport_priority;
  //    LifespanQosPolicy lifespan;
  //    DestinationOrderQosPolicy destination_order;
  //    HistoryQosPolicy history;
  //    ResourceLimitsQosPolicy resource_limits;
  //    OwnershipQosPolicy ownership;
  //    TopicDataQosPolicy topic_data;
  //  };

  // Extract a GUID from the key.
  OpenDDS::DCPS::Discovery_rch disc =
    TheServiceParticipant->get_discovery(participant->get_domain_id());
  OpenDDS::DCPS::DomainParticipantImpl* dpi =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant);
  OpenDDS::DCPS::RepoId id =
    disc->bit_key_to_repo_id(dpi,
                             OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC,
                             data.key);

  OpenDDS::DCPS::GuidConverter converter( id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s Topic Builtin Topic %C, key: ")
    ACE_TEXT("[0x%x, 0x%x, 0x%x].\n"),
    remove? "removing": "processing",
    (id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str(),
    data.key.value[0], data.key.value[1], data.key.value[2]
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool create        = false;
  bool layoutChanged = false;
  bool dataChanged   = false;

  // Find or create a DomainParticipant node for this data.
  TreeNode* node = this->getNode(
                     std::string(),
                     OpenDDS::DCPS::GUID_UNKNOWN,
                     id,
                     create
                   );
  if( !node) {
    return;
  }
  layoutChanged |= create;

  if( remove) {
    // Indicate visually that this participant has been destroyed.
    // @TODO: This should probably recursively modify the children as well.
    node->setColor(1,QColor("#ffbfbf"));

    this->model_->changed();
    return;
  }

  // Process the Qos data for this node.
  this->manageTopicQos( node, data, layoutChanged, dataChanged);

  // Notify the GUI if we have changed the underlying model.
  if( layoutChanged) {
    /// @TODO: Check that we really do not need to do updated here.
    this->model_->changed();
    //this->model_->updated( node, 1, (*node)[ node->size()-1], 1);

  } else if( dataChanged) {
    this->model_->changed();
    //this->model_->updated( node, 1, (*node)[ node->size()-1], 1);
  }
}

template<>
inline
void
MonitorDataStorage::update<DDS::PublicationBuiltinTopicData>(
  const DDS::PublicationBuiltinTopicData& data,
  DDS::DomainParticipant_ptr participant,
  bool remove
)
{
  //  struct PublicationBuiltinTopicData {
  //    BuiltinTopicKey_t key;
  //    BuiltinTopicKey_t participant_key;
  //    string topic_name;
  //    string type_name;
  //    DurabilityQosPolicy durability;
  //    DurabilityServiceQosPolicy durability_service;
  //    DeadlineQosPolicy deadline;
  //    LatencyBudgetQosPolicy latency_budget;
  //    LivelinessQosPolicy liveliness;
  //    ReliabilityQosPolicy reliability;
  //    LifespanQosPolicy lifespan;
  //    UserDataQosPolicy user_data;
  //    OwnershipQosPolicy ownership;
  //    OwnershipStrengthQosPolicy ownership_strength;
  //    DestinationOrderQosPolicy destination_order;
  //    PresentationQosPolicy presentation;
  //    PartitionQosPolicy partition;
  //    TopicDataQosPolicy topic_data;
  //    GroupDataQosPolicy group_data;
  //  };

  // Extract a GUID from the key.
  OpenDDS::DCPS::Discovery_rch disc =
    TheServiceParticipant->get_discovery(participant->get_domain_id());
  OpenDDS::DCPS::DomainParticipantImpl* dpi =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant);
  OpenDDS::DCPS::RepoId id =
    disc->bit_key_to_repo_id(dpi,
                             OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC,
                             data.key);

  OpenDDS::DCPS::GuidConverter converter( id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s Publication Builtin Topic %C, key: ")
    ACE_TEXT("[0x%x, 0x%x, 0x%x].\n"),
    remove? "removing": "processing",
    (id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str(),
    data.key.value[0], data.key.value[1], data.key.value[2]
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool create        = false;
  bool layoutChanged = false;
  bool dataChanged   = false;

  // Find or create a DomainParticipant node for this data.
  TreeNode* node = this->getNode(
                     std::string(),
                     OpenDDS::DCPS::GUID_UNKNOWN,
                     id,
                     create
                   );
  if( !node) {
    return;
  }
  layoutChanged |= create;

  if( remove) {
    // Indicate visually that this participant has been destroyed.
    // @TODO: This should probably recursively modify the children as well.
    node->setColor(1,QColor("#ffbfbf"));

    this->model_->changed();
    return;
  }

  // Process the Qos data for this node.
  this->managePublicationQos( node, data, layoutChanged, dataChanged);

  // Notify the GUI if we have changed the underlying model.
  if( layoutChanged) {
    /// @TODO: Check that we really do not need to do updated here.
    this->model_->changed();
    //this->model_->updated( node, 1, (*node)[ node->size()-1], 1);

  } else if( dataChanged) {
    this->model_->changed();
    //this->model_->updated( node, 1, (*node)[ node->size()-1], 1);
  }
}

template<>
inline
void
MonitorDataStorage::update<DDS::SubscriptionBuiltinTopicData>(
  const DDS::SubscriptionBuiltinTopicData& data,
  DDS::DomainParticipant_ptr participant,
  bool remove
)
{
  //  struct SubscriptionBuiltinTopicData {
  //    BuiltinTopicKey_t key;
  //    BuiltinTopicKey_t participant_key;
  //    string topic_name;
  //    string type_name;
  //    DurabilityQosPolicy durability;
  //    DeadlineQosPolicy deadline;
  //    LatencyBudgetQosPolicy latency_budget;
  //    LivelinessQosPolicy liveliness;
  //    ReliabilityQosPolicy reliability;
  //    OwnershipQosPolicy ownership;
  //    DestinationOrderQosPolicy destination_order;
  //    UserDataQosPolicy user_data;
  //    TimeBasedFilterQosPolicy time_based_filter;
  //    PresentationQosPolicy presentation;
  //    PartitionQosPolicy partition;
  //    TopicDataQosPolicy topic_data;
  //    GroupDataQosPolicy group_data;
  //  };

  // Extract a GUID from the key.
  OpenDDS::DCPS::Discovery_rch disc =
    TheServiceParticipant->get_discovery(participant->get_domain_id());
  OpenDDS::DCPS::DomainParticipantImpl* dpi =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant);
  OpenDDS::DCPS::RepoId id =
    disc->bit_key_to_repo_id(dpi,
                             OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC,
                             data.key);

  OpenDDS::DCPS::GuidConverter converter( id);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorDataStorage::update() - ")
    ACE_TEXT("%s Subscription Builtin Topic %C, key: ")
    ACE_TEXT("[0x%x, 0x%x, 0x%x].\n"),
    remove? "removing": "processing",
    (id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str(),
    data.key.value[0], data.key.value[1], data.key.value[2]
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool create        = false;
  bool layoutChanged = false;
  bool dataChanged   = false;

  // Find or create a DomainParticipant node for this data.
  TreeNode* node = this->getNode(
                     std::string(),
                     OpenDDS::DCPS::GUID_UNKNOWN,
                     id,
                     create
                   );
  if( !node) {
    return;
  }
  layoutChanged |= create;

  if( remove) {
    // Indicate visually that this participant has been destroyed.
    // @TODO: This should probably recursively modify the children as well.
    node->setColor(1,QColor("#ffbfbf"));

    this->model_->changed();
    return;
  }

  // Process the Qos data for this node.
  this->manageSubscriptionQos( node, data, layoutChanged, dataChanged);

  // Notify the GUI if we have changed the underlying model.
  if( layoutChanged) {
    /// @TODO: Check that we really do not need to do updated here.
    this->model_->changed();
    //this->model_->updated( node, 1, (*node)[ node->size()-1], 1);

  } else if( dataChanged) {
    this->model_->changed();
    //this->model_->updated( node, 1, (*node)[ node->size()-1], 1);
  }
}

