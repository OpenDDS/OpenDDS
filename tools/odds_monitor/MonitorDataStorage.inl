/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
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
  int row = node->indexOf( 0, label);
  if( row == -1) {
    // New value, add a node.
    QList<QVariant> list;
    list << label << QosToQString( value);
    TreeNode* policyNode = new TreeNode( list, node);
    node->append( policyNode);
    return true;

  } else {
    // Value update.
    (*node)[ row]->setData( 1, QosToQString( value));
  }
  return false;
}
 
template< typename DataType>
inline
void
MonitorDataStorage::update( const DataType&, bool)
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
MonitorDataStorage::update< OpenDDS::DCPS::DomainParticipantReport>(
  const OpenDDS::DCPS::DomainParticipantReport& data,
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
  this->model_->getBuiltinTopicData( data.dp_id, qosData);
  this->manageParticipantQos( node, qosData, layoutChanged, dataChanged);

  // Domain Id value.
  QString label( QObject::tr( "Domain Id"));
  int row = node->indexOf( 0, label);
  if( row == -1) {
    // New data, insert.
    QList<QVariant> list;
    list << label << QString::number( data.domain_id);
    TreeNode* domainNode = new TreeNode( list, node);
    node->append( domainNode);
    layoutChanged = true;

  } else {
    // Existing data, update.
    TreeNode* domainNode = (*node)[ row];
    domainNode->setData( 1, QString::number( data.domain_id));
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
MonitorDataStorage::update< OpenDDS::DCPS::TopicReport>(
  const OpenDDS::DCPS::TopicReport& data,
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
  this->model_->getBuiltinTopicData( data.topic_id, qosData);
  this->manageTopicQos( node, qosData, layoutChanged, dataChanged);

  // Topic name value.
  TreeNode* nameNode = 0;
  QString nameLabel( QObject::tr( "Topic Name"));
  int row = node->indexOf( 0, nameLabel);
  if( row == -1) {
    // New data, insert.
    QList<QVariant> list;
    list << nameLabel
         << QString( QObject::tr( static_cast<const char*>(data.topic_name)));
    nameNode = new TreeNode( list, node);
    node->append( nameNode);
    layoutChanged = true;

  } else {
    // Existing data, update.
    /// @TODO: check to see if we are really changing the value.
    nameNode = (*node)[ row];
    nameNode->setData(
      1,
      QString( QObject::tr( static_cast<const char*>(data.topic_name)))
    );
    dataChanged = true;
  }

  // Move any references to this topic node to the more useful name node
  // just obtained.
  if( node->reassignValueRefs( nameNode)) {
nameNode->setColor( 1, QColor("#bfffbf"));
    dataChanged = true;
  }

  // Data type value.
  QString typeLabel( QObject::tr( "Data Type"));
  row = node->indexOf( 0, typeLabel);
  if( row == -1) {
    // New data, insert.
    QList<QVariant> list;
    list << typeLabel
         << QString( QObject::tr( static_cast<const char*>(data.type_name)));
    TreeNode* typeNode = new TreeNode( list, node);
    node->append( typeNode);
    layoutChanged = true;

  } else {
    // Existing data, update.
    TreeNode* typeNode = (*node)[ row];
    typeNode->setData(
      1,
      QString( QObject::tr( static_cast<const char*>(data.type_name)))
    );
    dataChanged = true;
  }

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
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
  this->manageTransportLink( node, data.transport_id, create);
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
  this->manageTransportLink( node, data.transport_id, create);
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
MonitorDataStorage::update< OpenDDS::DCPS::DataWriterReport>(
  const OpenDDS::DCPS::DataWriterReport& data,
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
  this->model_->getBuiltinTopicData( data.dw_id, qosData);
  this->managePublicationQos( node, qosData, layoutChanged, dataChanged);

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
    (data.dw_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str()
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = false;
  bool dataChanged   = false;

  TreeNode* node = 0;

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataReaderReport>(
  const OpenDDS::DCPS::DataReaderReport& data,
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
  this->model_->getBuiltinTopicData( data.dr_id, qosData);
  this->manageSubscriptionQos( node, qosData, layoutChanged, dataChanged);

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
    (data.dr_id == OpenDDS::DCPS::GUID_UNKNOWN)? "GUID_UNKNOWN":
    std::string(converter).c_str()
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = false;
  bool dataChanged   = false;

  TreeNode* node = 0;

  // NAME / VALUE DATA, notify GUI of changes.
  this->displayNvp( node, data.values, layoutChanged, dataChanged);
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
    ACE_TEXT("transport: 0x%x, type: %C.\n"),
    remove? "removing": "processing",
    (const char*)data.host,
    data.pid,
    data.transport_id,
    (const char*)data.transport_type
  ));

  // Retain knowledge of node insertions, updates, and deletions.
  bool layoutChanged = !remove; // Updated by getTransportNode()
  bool dataChanged   = false;

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
    dataChanged = true;
  }
}

template<>
inline
void
MonitorDataStorage::update< DDS::ParticipantBuiltinTopicData>(
  const DDS::ParticipantBuiltinTopicData& data,
  bool remove
)
{
  //  struct ParticipantBuiltinTopicData {
  //    BuiltinTopicKey_t key;
  //    UserDataQosPolicy user_data;
  //  };

  // Extract a GUID from the key.
  OpenDDS::DCPS::RepoId id = OpenDDS::DCPS::RepoIdBuilder::create();
  OpenDDS::DCPS::RepoIdBuilder builder(id);
  builder.from_BuiltinTopicKey(data.key);

  OpenDDS::DCPS::GuidConverter converter( id);
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
    node = this->createParticipantNode( ProcessKey(), id, layoutChanged);
    layoutChanged = true;
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
MonitorDataStorage::update< DDS::TopicBuiltinTopicData>(
  const DDS::TopicBuiltinTopicData& data,
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
  OpenDDS::DCPS::RepoId id = OpenDDS::DCPS::RepoIdBuilder::create();
  OpenDDS::DCPS::RepoIdBuilder builder(id);
  builder.from_BuiltinTopicKey(data.key);

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
  bool create        = !remove;
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
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorDataStorage::update() - ")
      ACE_TEXT("unable to find place for Topic Qos in GUI.\n")
    ));
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
MonitorDataStorage::update< DDS::PublicationBuiltinTopicData>(
  const DDS::PublicationBuiltinTopicData& data,
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
  OpenDDS::DCPS::RepoId id = OpenDDS::DCPS::RepoIdBuilder::create();
  OpenDDS::DCPS::RepoIdBuilder builder(id);
  builder.from_BuiltinTopicKey(data.key);

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
  bool create        = !remove;
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
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorDataStorage::update() - ")
      ACE_TEXT("unable to find place for Topic Qos in GUI.\n")
    ));
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
MonitorDataStorage::update< DDS::SubscriptionBuiltinTopicData>(
  const DDS::SubscriptionBuiltinTopicData& data,
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
  OpenDDS::DCPS::RepoId id = OpenDDS::DCPS::RepoIdBuilder::create();
  OpenDDS::DCPS::RepoIdBuilder builder(id);
  builder.from_BuiltinTopicKey(data.key);

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
  bool create        = !remove;
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
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorDataStorage::update() - ")
      ACE_TEXT("unable to find place for Topic Qos in GUI.\n")
    ));
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

