/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MonitorDataStorage.h"

#include <sstream>

Monitor::MonitorDataStorage::MonitorDataStorage( MonitorData* model)
 : model_( model)
{
  this->reset();
}

Monitor::MonitorDataStorage::~MonitorDataStorage()
{
  this->clear();
}

std::string&
Monitor::MonitorDataStorage::activeIor()
{
  return this->activeIor_;
}

std::string
Monitor::MonitorDataStorage::activeIor() const
{
  return this->activeIor_;
}

bool
Monitor::MonitorDataStorage::ProcessKey::operator<( const ProcessKey& rhs) const
{
  if( this->host < rhs.host)      return true;
  else if( rhs.host < this->host) return false;
  else                            return this->pid < rhs.pid;
}

bool
Monitor::MonitorDataStorage::TransportKey::operator<( const TransportKey& rhs) const
{
  if( this->host < rhs.host)      return true;
  else if( rhs.host < this->host) return false;
  else if( this->pid < rhs.pid)   return true;
  else if( rhs.pid < this->pid)   return false;
  else                            return this->transport < rhs.transport;
}

bool
Monitor::MonitorDataStorage::InstanceKey::operator<( const InstanceKey& rhs) const
{
  GUID_tKeyLessThan compare;
  if( compare( this->guid, rhs.guid))      return true;
  else if( compare( rhs.guid, this->guid)) return false;
  else                                     return this->handle < rhs.handle;
}

void
Monitor::MonitorDataStorage::reset()
{
  this->clear();
}

void
Monitor::MonitorDataStorage::clear()
{
  // Empty the maps.
  this->guidToTreeMap_.clear();
  this->hostToTreeMap_.clear();
  this->processToTreeMap_.clear();
  this->instanceToTreeMap_.clear();
  this->transportToTreeMap_.clear();
}

void
Monitor::MonitorDataStorage::cleanMaps( TreeNode* node)
{
  // Remove all our children from the maps first.
  for( int child = 0; child < node->size(); ++child) {
    this->cleanMaps( (*node)[ child]);
  }

  // Now remove ourselves from all the maps.

  std::pair< bool, GuidToTreeMap::key_type> gResult
    = this->findKey( this->guidToTreeMap_, node);
  if( gResult.first) {
    this->guidToTreeMap_.erase( gResult.second);
  }

  std::pair< bool, HostToTreeMap::key_type> hResult
    = this->findKey( this->hostToTreeMap_, node);
  if( hResult.first) {
    this->hostToTreeMap_.erase( hResult.second);
  }

  std::pair< bool, ProcessToTreeMap::key_type> pResult
    = this->findKey( this->processToTreeMap_, node);
  if( pResult.first) {
    this->processToTreeMap_.erase( pResult.second);
  }

  std::pair< bool, InstanceToTreeMap::key_type> iResult
    = this->findKey( this->instanceToTreeMap_, node);
  if( iResult.first) {
    this->instanceToTreeMap_.erase( iResult.second);
  }

  std::pair< bool, TransportToTreeMap::key_type> tResult
    = this->findKey( this->transportToTreeMap_, node);
  if( tResult.first) {
    this->transportToTreeMap_.erase( tResult.second);
  }
}

Monitor::TreeNode*
Monitor::MonitorDataStorage::getProcessNode(
  const ProcessKey& key,
  bool& create
)
{
  // HOST

  TreeNode* hostNode = 0;
  HostToTreeMap::iterator hostLocation
    = this->hostToTreeMap_.find( key.host);
  if( hostLocation == this->hostToTreeMap_.end()) {
    // We are done if not creating a new node.
    if( !create) return 0;

    // We need to add a new host.  Host nodes are children of the
    // root node.
    TreeNode* root = this->model_->modelRoot();

    // Host first.
    QList<QVariant> list;
    list << QString("Host") << QString( QObject::tr( key.host.c_str()));
    hostNode = new TreeNode( list, root);
    root->append( hostNode);

    // Install the new node.
    this->hostToTreeMap_[ key.host]
      = std::make_pair( hostNode->row(), hostNode);

  } else {
    // Retain the current host node.
    hostNode = hostLocation->second.second;
  }

  // PROCESS

  TreeNode* pidNode = 0;
  ProcessToTreeMap::iterator pidLocation
    = this->processToTreeMap_.find( key);
  if( pidLocation == this->processToTreeMap_.end()) {
    // We are done if not creating a new node.
    if( !create) return 0;

    // We need to add a new PID.  PID nodes are children of the host
    // nodes.  We just found the relevant host node.

    // PID data.
    QList<QVariant> list;
    list << QString("Process") << QString::number( key.pid);
    pidNode = new TreeNode( list, hostNode);
    hostNode->append( pidNode);

    // Install the new node.
    this->processToTreeMap_[ key]
      = std::make_pair( pidNode->row(), pidNode);

  } else {
    pidNode = pidLocation->second.second;
    create = false;
  }

  return pidNode;
}

Monitor::TreeNode*
Monitor::MonitorDataStorage::getTransportNode(
  const TransportKey& key,
  bool&               create
)
{
  TreeNode* node = 0;
  TransportToTreeMap::iterator location
    = this->transportToTreeMap_.find( key);
  if( location == this->transportToTreeMap_.end()) {
    // We are done if not creating a new node.
    if( !create) return 0;

    // This transport needs to be installed.

    // Find the parent node, if any.  It is ok to not have a parent node
    // for cases of out-of-order updates.  We handle that as the updates
    // are actually processed.
    ProcessKey pid( key.host, key.pid);
    TreeNode* parent = this->getProcessNode( pid, create);

    QList<QVariant> list;
    QString value = QString("0x%1")
                    .arg( key.transport, 8, 16, QLatin1Char('0'));
    list << QString( QObject::tr( "Transport")) << value;
    node = new TreeNode( list, parent);
    if( parent) {
      parent->append( node);
    }

    // Install the new node.
    this->transportToTreeMap_[ key] = std::make_pair( node->row(), node);

  } else {
    node = location->second.second;
    create = false;
  }

  // If there have been some out-of-order reports, we may have been
  // created without a parent node.  We can fill in that information now
  // if we can.  If we created the node, we already know it has a
  // parent so this will be bypassed.
  if( !node->parent()) {
    create = true;
    ProcessKey pid( key.host, key.pid);
    node->parent() = this->getProcessNode( pid, create);
  }

//node->setColor(1,QColor("#ffbfbf"));
  return node;
}

Monitor::TreeNode*
Monitor::MonitorDataStorage::createParticipantNode(
  const ProcessKey&            pid,
  const OpenDDS::DCPS::GUID_t& id,
  bool&                        create
)
{
  if( id == OpenDDS::DCPS::GUID_UNKNOWN) {
    return 0;
  }

  // Find the parent node, if any.  It is ok to not have a parent node
  // for cases of out-of-order udpates.  We handle that as the updates
  // are actually processed.
  TreeNode* parent = this->getProcessNode( pid, create);

  // DomainParticipant data.
  OpenDDS::DCPS::GuidConverter converter( id);
  QList<QVariant> list;
  list << QString("DomainParticipant")
       << QString( QObject::tr( std::string( converter).c_str()));
  TreeNode* node = new TreeNode( list, parent);
  if( parent) {
    parent->append( node);
  }

  // Install the new node.
  this->guidToTreeMap_[ id] = std::make_pair( node->row(), node);

  return node;
}

Monitor::TreeNode*
Monitor::MonitorDataStorage::getParticipantNode(
  const ProcessKey&            pid,
  const OpenDDS::DCPS::GUID_t& id,
  bool&                        create
)
{
  TreeNode* node   = 0;
  GuidToTreeMap::iterator location = this->guidToTreeMap_.find( id);
  if( location == this->guidToTreeMap_.end()) {
    // We are done if not creating a new node.
    if( !create) return 0;

    // We need to add a new DomainParticipant.
    node = this->createParticipantNode( pid, id, create);

  } else {
    node = location->second.second;
    create = false;
  }

  // If there have been some out-of-order reports, we may have been
  // created without a parent node.  We can now fill in that information
  // if we can.  If we created the node, we already know it has a
  // parent so this will be bypassed.
  if( !node->parent()) {
    create = true;
    node->parent() = this->getProcessNode( pid, create);
  }

  return node;
}

Monitor::TreeNode*
Monitor::MonitorDataStorage::getInstanceNode(
  const std::string& label,
  const InstanceKey& key,
  bool& create
)
{
  TreeNode* node   = 0;
  InstanceToTreeMap::iterator location
    = this->instanceToTreeMap_.find( key);
  if( location == this->instanceToTreeMap_.end()) {
    // We are done if not creating a new node.
    if( !create) return 0;

    // We need to add a new DomainParticipant.

    // Find the parent node, if any.  It is ok to not have a parent node
    // for cases of out-of-order udpates.  We handle that as the updates
    // are actually processed.
    TreeNode* parent = 0;
    GuidToTreeMap::iterator parentLocation
      = this->guidToTreeMap_.find( key.guid);
    if( parentLocation != this->guidToTreeMap_.end()) {
      parent = parentLocation->second.second;
    }

    // Node data.
    QList<QVariant> list;
    list << QString( QObject::tr( label.c_str()))
         << QString::number( key.handle);
    node = new TreeNode( list, parent);
    if( parent) {
      parent->append( node);
    }

    // Install the new node.
    this->instanceToTreeMap_[ key] = std::make_pair( node->row(), node);

  } else {
    node = location->second.second;
    create = false;
  }

  // If there have been some out-of-order reports, we may have been
  // created without a parent node.  We can now fill in that information
  // if we can.  If we created the node, we already know it has a
  // parent so this will be bypassed.
  if( !node->parent()) {
    // Need to search for the parent.
    TreeNode* parent = 0;
    GuidToTreeMap::iterator parentLocation
      = this->guidToTreeMap_.find( key.guid);
    if( parentLocation != this->guidToTreeMap_.end()) {
      parent = parentLocation->second.second;
    }

    // And install anything that is found.
    node->parent() = parent;
  }

  return node;
}

Monitor::TreeNode*
Monitor::MonitorDataStorage::getEndpointNode(
  const std::string&           label,
  const InstanceKey&           key,
  const OpenDDS::DCPS::GUID_t& id,
  bool&                        create
)
{
  TreeNode* node   = 0;
  GuidToTreeMap::iterator location
    = this->guidToTreeMap_.find( id);
  if( location == this->guidToTreeMap_.end()) {
    // We are done if not creating a new node.
    if( !create) return 0;

    // We need to add a new endpoint.

    // Find the parent node, if any.  It is ok to not have a parent node
    // for cases of out-of-order udpates.  We handle that as the updates
    // are actually processed.
    TreeNode* parent = 0;
    InstanceToTreeMap::iterator parentLocation
      = this->instanceToTreeMap_.find( key);
    if( parentLocation != this->instanceToTreeMap_.end()) {
      parent = parentLocation->second.second;
    }

    // Node data.
    OpenDDS::DCPS::GuidConverter converter( id);
    QList<QVariant> list;
    list << QString( QObject::tr( label.c_str()))
         << QString( QObject::tr( std::string( converter).c_str()));
    node = new TreeNode( list, parent);
    if( parent) {
      parent->append( node);
    }

    // Install the new node.
    this->guidToTreeMap_[ id] = std::make_pair( node->row(), node);

  } else {
    node = location->second.second;
    create = false;
  }

  // If there have been some out-of-order reports, we may have been
  // created without a parent node.  We can now fill in that information
  // if we can.  If we created the node, we already know it has a
  // parent so this will be bypassed.
  if( !node->parent()) {
    // Need to search for the parent.
    TreeNode* parent = 0;
    InstanceToTreeMap::iterator parentLocation
      = this->instanceToTreeMap_.find( key);
    if( parentLocation != this->instanceToTreeMap_.end()) {
      parent = parentLocation->second.second;
    }

    // And install anything that is found.
    node->parent() = parent;
  }

  return node;
}

Monitor::TreeNode*
Monitor::MonitorDataStorage::getNode(
  const std::string&           label,
  const OpenDDS::DCPS::GUID_t& parentId,
  const OpenDDS::DCPS::GUID_t& id,
  bool&                        create
)
{
  TreeNode* node   = 0;
  GuidToTreeMap::iterator location = this->guidToTreeMap_.find( id);
  if( location == this->guidToTreeMap_.end()) {
    // We are done if not creating a new node.
    if( !create) return 0;

    // We need to add a new DomainParticipant.

    // Find the parent node, if any.  It is ok to not have a parent node
    // for cases of out-of-order udpates.  We handle that as the updates
    // are actually processed.
    TreeNode* parent = 0;
    GuidToTreeMap::iterator parentLocation
      = this->guidToTreeMap_.find( parentId);
    if( parentLocation != this->guidToTreeMap_.end()) {
      parent = parentLocation->second.second;
    }

    // Node data.
    OpenDDS::DCPS::GuidConverter converter( id);
    QList<QVariant> list;
    list << QString( QObject::tr( label.c_str()))
         << QString( QObject::tr( std::string( converter).c_str()));
    node = new TreeNode( list, parent);
    if( parent) {
      parent->append( node);
    }

    // Install the new node.
    this->guidToTreeMap_[ id] = std::make_pair( node->row(), node);

  } else {
    node = location->second.second;
    create = false;
  }

  // If there have been some out-of-order reports, we may have been
  // created without a parent node.  We can now fill in that information
  // if we can.  If we created the node, we already know it has a
  // parent so this will be bypassed.
  if( !node->parent()) {
    // Need to search for the parent.
    TreeNode* parent = 0;
    GuidToTreeMap::iterator parentLocation
      = this->guidToTreeMap_.find( parentId);
    if( parentLocation != this->guidToTreeMap_.end()) {
      parent = parentLocation->second.second;
    }

    // And install anything that is found.
    node->parent() = parent;
  }

  return node;
}

bool
Monitor::MonitorDataStorage::manageChildValue(
  TreeNode*      parent,
  TreeNode*&     node,
  const QString& label,
  const QString& value
)
{
  int row = parent->indexOf( 0, label);
  if( row == -1) {
    // New value, add a node.
    QList<QVariant> list;
    list << label << value;
    node = new TreeNode( list, parent);
    parent->append( node);
    return true;

  } else {
    // Value update.
    node = (*parent)[ row];
    node->setData( 1, value);
  }
  return false;
}

void
Monitor::MonitorDataStorage::manageTransportLink(
  TreeNode* node,
  int       transport_id,
  bool&     create
)
{
  // Start by finding the participant node.
  TreeNode* processNode = node->parent();
  if( processNode) {
    // And follow it to the actual process node.
    processNode = processNode->parent();

  } else {
    // Horribly corrupt model, something oughta been done about it!
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorDataStorage::manageTransportLink() - ")
      ACE_TEXT("unable to locate the ancestor process node!\n")
    ));
    return;
  }

  // Then finds its key in the maps.
  ProcessKey processKey;
  std::pair< bool, ProcessKey> pResult
    = this->findKey( this->processToTreeMap_, processNode);
  if( pResult.first) {
    processKey = pResult.second;

  } else {
    // Horribly corrupt model, something oughta been done about it!
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorDataStorage::manageTransportLink() - ")
      ACE_TEXT("unable to locate the process node in the maps!\n")
    ));
    return;
  }

  // Now we have enough information to build a TransportKey and find or
  // create a transport node for our Id value.
  TransportKey transportKey(
                 processKey.host,
                 processKey.pid,
                 transport_id
               );
  TreeNode* transportNode = this->getTransportNode( transportKey, create);
  if( !node) {
    return;
  }

  // Transport Id display.
  QString label( QObject::tr( "Transport"));
  int row = node->indexOf( 0, label);
  if( row == -1) {
    // New entry, add a reference to the actual transport node.
    QList<QVariant> list;
    list << label << QString( QObject::tr("<error>"));;
    TreeNode* idNode = new TreeNode( list, node);
    idNode->setColor( 1, QColor("#bfbfff"));
    node->append( idNode);
    transportNode->addValueRef( idNode);
    transportNode->setColor( 1, QColor("#bfffbf"));
    create = true;
  }
}

void
Monitor::MonitorDataStorage::manageTopicLink(
  TreeNode*                    node,
  const OpenDDS::DCPS::GUID_t& dp_id,
  const OpenDDS::DCPS::GUID_t& topic_id,
  bool&                        create
)
{
  // This gets a bit tedious.  Here are the cases:
  // 1) We currently have no reference, and found a Topic node to
  //    reference:
  //    - attach a reference to the topic node;
  // 2) We currently have no reference, and found a Name node to
  //    reference:
  //    - attach a reference to the name node;
  // 3) We currently have a Topic node referenced, and found the same
  //    topic node to reference:
  //    - nothing to do;
  // 4) We currently have a Topic node referenced, and found a different
  //    Topic node to reference:
  //    - detach previous reference and reattach to new topic node;
  // 5) We currently have a Topic node referenced, but were able to find
  //    a name node to reference:
  //    - detach previous reference and reattach to new name node;
  // 6) We currently have a Name node referenced, and found the same name
  //    node to reference:
  //    - nothing to do;
  // 7) We currently have a Name node referenced, and found a different
  //    name node to reference:
  //    - detach previous reference and reattach to new name node;
  // 8) We currently have a Name node referenced, and found a different
  //    Topic node to reference:
  //    - detach previous reference and reattach to new topic node.
  //
  // Note that cases (4), (7), and (8) indicate inconsistent data reports
  // and are error conditions.  We chose to not handle these cases.
  // Cases (3) and (6) require no action, so the only code paths we need
  // to consider are for cases (1), (2), and (5).

  // Find the actual topic.
  TreeNode* topicNode = this->getNode(
                          std::string( "Topic"),
                          dp_id,
                          topic_id,
                          create
                        );
  if( !topicNode) {
    return;
  }

  // Find the topic name to reference instead of the GUID value.
  TreeNode* nameNode = 0;
  QString nameLabel( QObject::tr( "Topic Name"));
  int row = topicNode->indexOf( 0, nameLabel);
  if( row != -1) {
    nameNode = (*topicNode)[ row];
  }

  // Check for an existing Topic entry.
  QString topicLabel( QObject::tr( "Topic"));
  int topicRow = node->indexOf( 0, topicLabel);
  if( nameNode && topicRow != -1) {
    // Case 5: We have a topic reference and found a name reference,
    //         remove the existing topic reference
    TreeNode* topicRef = (*node)[ topicRow];
    if( topicRef && topicRef->valueSource()) {
      topicRef->valueSource()->removeValueRef( topicRef);
    }
//  node->removeChildren( topicRow, 1); // DEVELOPMENT: don't delete to show stale data.
  }

  // The node to install.
  TreeNode* refNode = nameNode;
  if( !refNode) {
    refNode = topicNode;
  }

  // Check to see if we need to create a name entry.
  int nameRow  = node->indexOf( 0, nameLabel);
  if( nameRow == -1) {
    // New entry, add a reference to the topic or its name.
    QList<QVariant> list;
    list << topicLabel << QString( QObject::tr("<error>"));
    TreeNode* idNode = new TreeNode( list, node);
    idNode->setColor( 1, QColor("#bfbfff"));
    node->append( idNode);
    refNode->addValueRef( idNode);
    refNode->setColor( 1, QColor("#bfffbf"));
    create = true;
  }
}

void
Monitor::MonitorDataStorage::deleteProcessNode( TreeNode* node)
{
  std::pair< bool, ProcessToTreeMap::key_type> pResult
    = this->findKey( this->processToTreeMap_, node);
  if( pResult.first) {
    this->processToTreeMap_.erase( pResult.second);
  }

  // Remove the process from the host.
  TreeNode* hostNode = node->parent();
  hostNode->removeChildren( node->row(), 1);

  // Check and remove the host node if there are no pid nodes remaining
  // after the removal (no children).
  if( hostNode->size() == 0) {
    std::pair< bool, HostToTreeMap::key_type> hResult
      = this->findKey( this->hostToTreeMap_, hostNode);
    if( hResult.first) {
      this->hostToTreeMap_.erase( hResult.second);
    }
    delete hostNode;
  }

  delete node;

  // Notify the GUI to update.
  this->model_->changed();
}

void
Monitor::MonitorDataStorage::displayNvp(
  TreeNode*                    parent,
  const OpenDDS::DCPS::NVPSeq& data,
  bool                         layoutChanged,
  bool                         dataChanged
)
{
  // NAME / VALUE DATA
  int size = data.length();
  for( int index = 0; index < size; ++index) {
    QString name( data[ index].name);
    int row = parent->indexOf( 0, name);
    if( row == -1) {
      // This is new data, insert it.
      QList<QVariant> list;
      list << name;
      switch( data[ index].value._d()) {
        case OpenDDS::DCPS::INTEGER_TYPE:
          list << QString::number( data[ index].value.integer_value());
          break;

        case OpenDDS::DCPS::DOUBLE_TYPE:
          list << QString::number( data[ index].value.double_value());
          break;

        case OpenDDS::DCPS::STRING_TYPE:
          list << QString( data[ index].value.string_value());
          break;

        case OpenDDS::DCPS::STATISTICS_TYPE:
        case OpenDDS::DCPS::STRING_LIST_TYPE:
          list << QString( QObject::tr("<display unimplemented>"));
          break;
      }
      TreeNode* node = new TreeNode( list, parent);
      parent->append( node);
      layoutChanged = true;

    } else {
      // This is existing data, update the value.
      TreeNode* node = (*parent)[ row];
      switch( data[ index].value._d()) {
        case OpenDDS::DCPS::INTEGER_TYPE:
          node->setData( 1, QString::number( data[ index].value.integer_value()));
          break;

        case OpenDDS::DCPS::DOUBLE_TYPE:
          node->setData( 1, QString::number( data[ index].value.double_value()));
          break;

        case OpenDDS::DCPS::STRING_TYPE:
          node->setData( 1, QString( data[ index].value.string_value()));
          break;

        case OpenDDS::DCPS::STATISTICS_TYPE:
        case OpenDDS::DCPS::STRING_LIST_TYPE:
          break;
      }
      dataChanged = true;
    }
  }

  // Notify the GUI if we have changed the underlying model.
  if( layoutChanged) {
    /// @TODO: Check that we really do not need to do updated here.
    this->model_->changed();

  } else if( dataChanged) {
    this->model_->updated( parent, 1, (*parent)[ parent->size()-1], 1);
  }
}

Monitor::TreeNode*
Monitor::MonitorDataStorage::createQosNode(
  const std::string& label,
  TreeNode*          node,
  bool&              created
)
{
  TreeNode* qosNode = 0;
  QString qosLabel( QObject::tr( "Qos"));
  int qosRow = node->indexOf( 0, qosLabel);
  if( qosRow == -1) {
    // New entry, add a reference to the actual transport node.
    node->insertChildren( 0, 1, 2); // Always make Qos the first child.
    qosNode = (*node)[0];
    qosNode->setData( 0, qosLabel);
    qosNode->setData( 1, label.c_str());
    created = true;

  } else {
    qosNode = (*node)[ qosRow];
  }

  if( !qosNode) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorDataStorage::createQosNode() - ")
      ACE_TEXT("unable to find place for Qos in GUI.\n")
    ));
  }
  return qosNode;
}

void
Monitor::MonitorDataStorage::manageParticipantQos(
  TreeNode*                               parent,
  const DDS::ParticipantBuiltinTopicData& data,
  bool&                                   layoutChanged,
  bool&                                   dataChanged
)
{
  //  struct ParticipantBuiltinTopicData {
  //    BuiltinTopicKey_t key;
  //    UserDataQosPolicy user_data;
  //  };

  // Obtain the node to hold the Qos policy values.
  bool create = false;
  TreeNode* qosNode
    = this->createQosNode( std::string("DomainParticipantQos"), parent, create);
  layoutChanged |= create;

  // Install the policy values.

  //    BuiltinTopicKey_t key;
  QString builtinTopicKeyLabel
    = QString( QObject::tr("BuiltinTopicKey.value"));
  if( this->manageQosPolicy( qosNode, builtinTopicKeyLabel, data.key)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    UserDataQosPolicy user_data;
  QString userDataLabel
    = QString( QObject::tr("UserDataQosPolicy.user_data"));
  if( this->manageQosPolicy( qosNode, userDataLabel, data.user_data)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }
}

void
Monitor::MonitorDataStorage::manageTopicQos(
  TreeNode*                         parent,
  const DDS::TopicBuiltinTopicData& data,
  bool&                             layoutChanged,
  bool&                             dataChanged
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

  // Obtain the node to hold the Qos policy values.
  bool create = false;
  TreeNode* qosNode
    = this->createQosNode( std::string("TopicQos"), parent, create);
  layoutChanged |= create;

  // Install the policy values.

  //    BuiltinTopicKey_t key;
  QString builtinTopicKeyLabel
    = QString( QObject::tr("BuiltinTopicKey.value"));
  if( this->manageQosPolicy( qosNode, builtinTopicKeyLabel, data.key)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    string name;
  QString topicNameLabel
    = QString( QObject::tr("Topic name"));
  QString topicNameValue
    = QString( QObject::tr( static_cast<const char*>(data.name)));
  if( this->manageQosPolicy( qosNode, topicNameLabel, topicNameValue)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    string type_name;
  QString topicTypeLabel
    = QString( QObject::tr("Topic data type"));
  QString topicTypeValue
    = QString( QObject::tr( static_cast<const char*>(data.type_name)));
  if( this->manageQosPolicy( qosNode, topicTypeLabel, topicTypeValue)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DurabilityQosPolicy durability;
  QString durabilityLabel
    = QString( QObject::tr("DurabilityQosPolicy.durability"));
  if( this->manageQosPolicy( qosNode, durabilityLabel, data.durability)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DurabilityServiceQosPolicy durability_service;
  QString durabilityServiceLabel
    = QString( QObject::tr("DurabilityServiceQosPolicy.durability"));
  if( this->manageQosPolicy( qosNode, durabilityServiceLabel, data.durability_service)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DeadlineQosPolicy deadline;
  QString deadlineLabel
    = QString( QObject::tr("DeadlineQosPolicy.deadline"));
  if( this->manageQosPolicy( qosNode, deadlineLabel, data.deadline)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    LatencyBudgetQosPolicy latency_budget;
  QString latencyLabel
    = QString( QObject::tr("LatencyBudgetQosPolicy.latency_budget"));
  if( this->manageQosPolicy( qosNode, latencyLabel, data.latency_budget)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    LivelinessQosPolicy liveliness;
  QString livelinessLabel
    = QString( QObject::tr("LivelinessQosPolicy.liveliness"));
  if( this->manageQosPolicy( qosNode, livelinessLabel, data.liveliness)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    ReliabilityQosPolicy reliability;
  QString reliabilityLabel
    = QString( QObject::tr("ReliabilityQosPolicy.reliability"));
  if( this->manageQosPolicy( qosNode, reliabilityLabel, data.reliability)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    TransportPriorityQosPolicy transport_priority;
  QString priorityLabel
    = QString( QObject::tr("TransportPriorityQosPolicy.transport_priority"));
  if( this->manageQosPolicy( qosNode, priorityLabel, data.transport_priority)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    LifespanQosPolicy lifespan;
  QString lifespanLabel
    = QString( QObject::tr("LifespandQosPolicy.lifespan"));
  if( this->manageQosPolicy( qosNode, lifespanLabel, data.lifespan)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DestinationOrderQosPolicy destination_order;
  QString destinationOrderLabel
    = QString( QObject::tr("DestinationOrderQosPolicy.deadline"));
  if( this->manageQosPolicy( qosNode, destinationOrderLabel, data.destination_order)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    HistoryQosPolicy history;
  QString historyLabel
    = QString( QObject::tr("HistoryQosPolicy.history"));
  if( this->manageQosPolicy( qosNode, historyLabel, data.history)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    ResourceLimitsQosPolicy resource_limits;
  QString limitsLabel
    = QString( QObject::tr("ResourceLimitsQosPolicy.resource_limits"));
  if( this->manageQosPolicy( qosNode, limitsLabel, data.resource_limits)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    OwnershipQosPolicy ownership;
  QString ownershipLabel
    = QString( QObject::tr("OwnershipQosPolicy.ownership"));
  if( this->manageQosPolicy( qosNode, ownershipLabel, data.ownership)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    TopicDataQosPolicy topic_data;
  QString topicDataLabel
    = QString( QObject::tr("TopicDataQosPolicy.topic_data"));
  if( this->manageQosPolicy( qosNode, topicDataLabel, data.topic_data)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }
}

void
Monitor::MonitorDataStorage::managePublicationQos(
  TreeNode*                               parent,
  const DDS::PublicationBuiltinTopicData& data,
  bool&                                   layoutChanged,
  bool&                                   dataChanged
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

  /// @TODO: segregate the publisher and data writer policy values.

  // Obtain the node to hold the Qos policy values.
  bool create = false;
  TreeNode* qosNode
    = this->createQosNode( std::string("PublicationQos"), parent, create);
  layoutChanged |= create;

  // Install the policy values.

  //    BuiltinTopicKey_t key;
  QString builtinTopicKeyLabel
    = QString( QObject::tr("BuiltinTopicKey.value"));
  if( this->manageQosPolicy( qosNode, builtinTopicKeyLabel, data.key)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    string name;
  QString topicNameLabel
    = QString( QObject::tr("Topic name"));
  QString topicNameValue
    = QString( QObject::tr( static_cast<const char*>(data.topic_name)));
  if( this->manageQosPolicy( qosNode, topicNameLabel, topicNameValue)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    string type_name;
  QString topicTypeLabel
    = QString( QObject::tr("Topic data type"));
  QString topicTypeValue
    = QString( QObject::tr( static_cast<const char*>(data.type_name)));
  if( this->manageQosPolicy( qosNode, topicTypeLabel, topicTypeValue)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DurabilityQosPolicy durability;
  QString durabilityLabel
    = QString( QObject::tr("DurabilityQosPolicy.durability"));
  if( this->manageQosPolicy( qosNode, durabilityLabel, data.durability)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DurabilityServiceQosPolicy durability_service;
  QString durabilityServiceLabel
    = QString( QObject::tr("DurabilityServiceQosPolicy.durability"));
  if( this->manageQosPolicy( qosNode, durabilityServiceLabel, data.durability_service)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DeadlineQosPolicy deadline;
  QString deadlineLabel
    = QString( QObject::tr("DeadlineQosPolicy.deadline"));
  if( this->manageQosPolicy( qosNode, deadlineLabel, data.deadline)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    LatencyBudgetQosPolicy latency_budget;
  QString latencyLabel
    = QString( QObject::tr("LatencyBudgetQosPolicy.latency_budget"));
  if( this->manageQosPolicy( qosNode, latencyLabel, data.latency_budget)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    LivelinessQosPolicy liveliness;
  QString livelinessLabel
    = QString( QObject::tr("LivelinessQosPolicy.liveliness"));
  if( this->manageQosPolicy( qosNode, livelinessLabel, data.liveliness)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    ReliabilityQosPolicy reliability;
  QString reliabilityLabel
    = QString( QObject::tr("ReliabilityQosPolicy.reliability"));
  if( this->manageQosPolicy( qosNode, reliabilityLabel, data.reliability)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    LifespanQosPolicy lifespan;
  QString lifespanLabel
    = QString( QObject::tr("LifespandQosPolicy.lifespan"));
  if( this->manageQosPolicy( qosNode, lifespanLabel, data.lifespan)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    UserDataQosPolicy user_data;
  QString userDataLabel
    = QString( QObject::tr("UserDataQosPolicy.user_data"));
  if( this->manageQosPolicy( qosNode, userDataLabel, data.user_data)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    OwnershipQosPolicy ownership;
  QString ownershipDataLabel
    = QString( QObject::tr("OwnershipQosPolicy.ownership"));
  if( this->manageQosPolicy( qosNode, ownershipDataLabel, data.ownership)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    OwnershipStrengthQosPolicy ownership_strength;
  QString ownershipStrengthDataLabel
    = QString( QObject::tr("OwnershipStrengthQosPolicy.ownership_strength"));
  if( this->manageQosPolicy( qosNode,
                             ownershipStrengthDataLabel,
                             data.ownership_strength)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DestinationOrderQosPolicy destination_order;
  QString destinationOrderLabel
    = QString( QObject::tr("DestinationOrderQosPolicy.destination_order"));
  if( this->manageQosPolicy( qosNode, destinationOrderLabel, data.destination_order)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    PresentationQosPolicy presentation;
  QString presentationLabel
    = QString( QObject::tr("PresentationQosPolicy.presentation"));
  if( this->manageQosPolicy( qosNode, presentationLabel, data.presentation)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    PartitionQosPolicy partition;
  QString partitionLabel
    = QString( QObject::tr("PartitionQosPolicy.partition"));
  if( this->manageQosPolicy( qosNode, partitionLabel, data.partition)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    TopicDataQosPolicy topic_data;
  QString topicDataLabel
    = QString( QObject::tr("TopicDataQosPolicy.topic_data"));
  if( this->manageQosPolicy( qosNode, topicDataLabel, data.topic_data)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    GroupDataQosPolicy group_data;
  QString groupDataLabel
    = QString( QObject::tr("GroupDataQosPolicy.group_data"));
  if( this->manageQosPolicy( qosNode, groupDataLabel, data.group_data)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }
}

void
Monitor::MonitorDataStorage::manageSubscriptionQos(
  TreeNode*                                parent,
  const DDS::SubscriptionBuiltinTopicData& data,
  bool&                                    layoutChanged,
  bool&                                    dataChanged
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

  /// @TODO: segregate the publisher and data writer policy values.

  // Obtain the node to hold the Qos policy values.
  bool create = false;
  TreeNode* qosNode
    = this->createQosNode( std::string("SubscriptionQos"), parent, create);
  layoutChanged |= create;

  // Install the policy values.

  //    BuiltinTopicKey_t key;
  QString builtinTopicKeyLabel
    = QString( QObject::tr("BuiltinTopicKey.value"));
  if( this->manageQosPolicy( qosNode, builtinTopicKeyLabel, data.key)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    string name;
  QString topicNameLabel
    = QString( QObject::tr("Topic name"));
  QString topicNameValue
    = QString( QObject::tr( static_cast<const char*>(data.topic_name)));
  if( this->manageQosPolicy( qosNode, topicNameLabel, topicNameValue)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    string type_name;
  QString topicTypeLabel
    = QString( QObject::tr("Topic data type"));
  QString topicTypeValue
    = QString( QObject::tr( static_cast<const char*>(data.type_name)));
  if( this->manageQosPolicy( qosNode, topicTypeLabel, topicTypeValue)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DurabilityQosPolicy durability;
  QString durabilityLabel
    = QString( QObject::tr("DurabilityQosPolicy.durability"));
  if( this->manageQosPolicy( qosNode, durabilityLabel, data.durability)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DeadlineQosPolicy deadline;
  QString deadlineLabel
    = QString( QObject::tr("DeadlineQosPolicy.deadline"));
  if( this->manageQosPolicy( qosNode, deadlineLabel, data.deadline)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    LatencyBudgetQosPolicy latency_budget;
  QString latencyLabel
    = QString( QObject::tr("LatencyBudgetQosPolicy.latency_budget"));
  if( this->manageQosPolicy( qosNode, latencyLabel, data.latency_budget)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    LivelinessQosPolicy liveliness;
  QString livelinessLabel
    = QString( QObject::tr("LivelinessQosPolicy.liveliness"));
  if( this->manageQosPolicy( qosNode, livelinessLabel, data.liveliness)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    ReliabilityQosPolicy reliability;
  QString reliabilityLabel
    = QString( QObject::tr("ReliabilityQosPolicy.reliability"));
  if( this->manageQosPolicy( qosNode, reliabilityLabel, data.reliability)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    OwnershipQosPolicy ownership;
  QString ownershipDataLabel
    = QString( QObject::tr("OwnershipQosPolicy.ownership"));
  if( this->manageQosPolicy( qosNode, ownershipDataLabel, data.ownership)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    DestinationOrderQosPolicy destination_order;
  QString destinationOrderLabel
    = QString( QObject::tr("DestinationOrderQosPolicy.destination_order"));
  if( this->manageQosPolicy( qosNode, destinationOrderLabel, data.destination_order)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    UserDataQosPolicy user_data;
  QString userDataLabel
    = QString( QObject::tr("UserDataQosPolicy.user_data"));
  if( this->manageQosPolicy( qosNode, userDataLabel, data.user_data)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    TimeBasedFilterQosPolicy time_based_filter;
  QString timeBasedFilterLabel
    = QString( QObject::tr("TimeBasedFilterQosPolicy.time_based_filter"));
  if( this->manageQosPolicy( qosNode,
                             timeBasedFilterLabel,
                             data.time_based_filter)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    PresentationQosPolicy presentation;
  QString presentationLabel
    = QString( QObject::tr("PresentationQosPolicy.presentation"));
  if( this->manageQosPolicy( qosNode, presentationLabel, data.presentation)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    PartitionQosPolicy partition;
  QString partitionLabel
    = QString( QObject::tr("PartitionQosPolicy.partition"));
  if( this->manageQosPolicy( qosNode, partitionLabel, data.partition)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    TopicDataQosPolicy topic_data;
  QString topicDataLabel
    = QString( QObject::tr("TopicDataQosPolicy.topic_data"));
  if( this->manageQosPolicy( qosNode, topicDataLabel, data.topic_data)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }

  //    GroupDataQosPolicy group_data;
  QString groupDataLabel
    = QString( QObject::tr("GroupDataQosPolicy.group_data"));
  if( this->manageQosPolicy( qosNode, groupDataLabel, data.group_data)) {
    layoutChanged = true;

  } else {
    dataChanged = true;
  }
}

//testing -- there's a qos problem. after several update calls
//readbuiltintopics returns false which leads to qos not being populated. these
//statics ensure qos stays if it was once valid. this is a NOT a fix!!!
bool Monitor::MonitorDataStorage::b1 = false;
bool Monitor::MonitorDataStorage::b2 = false;
bool Monitor::MonitorDataStorage::b3 = false;
bool Monitor::MonitorDataStorage::b4 = false;

