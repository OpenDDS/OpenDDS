/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MonitorDataStorage.h"

#include <sstream>

Monitor::MonitorDataStorage::MonitorDataStorage( MonitorData* model)
 : model_( model),
   publisherIdGenerator_( 0),
   subscriberIdGenerator_( 0),
   transportIdGenerator_( 0)
{
  this->reset();
}

Monitor::MonitorDataStorage::~MonitorDataStorage()
{
  this->clear();
}

void
Monitor::MonitorDataStorage::reset()
{
  this->clear();

  // Establish the generators.
  this->publisherIdGenerator_ = new RepoIdGenerator( 0, 0, OpenDDS::DCPS::KIND_PUBLISHER);
  this->subscriberIdGenerator_ = new RepoIdGenerator( 0, 0, OpenDDS::DCPS::KIND_SUBSCRIBER);
  this->transportIdGenerator_ = new RepoIdGenerator( 0, 0, OpenDDS::DCPS::KIND_USER);
}

void
Monitor::MonitorDataStorage::clear()
{
  // Empty the maps.
  this->guidToTreeMap_.clear();
  this->hostToTreeMap_.clear();
  this->processToTreeMap_.clear();
  this->transportToGuidMap_.clear();

  // Remove the generators.
  delete this->publisherIdGenerator_;
  delete this->subscriberIdGenerator_;
  delete this->transportIdGenerator_;
}

void
Monitor::MonitorDataStorage::cleanMaps( TreeNode* node)
{
  // Remove all our children from the maps first.
  for( int child = 0; child < node->size(); ++child) {
    this->cleanMaps( (*node)[ child]);
  }

  // Now remove ourselves from all the maps.
  this->removeNode( this->guidToTreeMap_, node);
  this->removeNode( this->hostToTreeMap_, node);
  this->removeNode( this->processToTreeMap_, node);
}

template< class MapType>
void
Monitor::MonitorDataStorage::removeNode( MapType& map, TreeNode* node)
{
  // This search is predicated on a node only being present once in any
  // tree.
  // Need to build a reverse index to do this efficiently.
  for( typename MapType::iterator current = map.begin();
       current != map.end();
       ++current) {
    if( node == current->second.second) {
      map.erase( current);
      break;
    }
  }
}

void
Monitor::MonitorDataStorage::displayNvp(
  TreeNode*                    node,
  const OpenDDS::DCPS::NVPSeq& data,
  bool                         layoutChanged,
  bool                         dataChanged
)
{
  // NAME / VALUE DATA
  int size = data.length();
  for( int index = 0; index < size; ++index) {
    QString name( data[ index].name);
    int row = node->indexOf( 1, name);
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
          list << QString( "<display unimplemented>");
          break;
      }
      TreeNode* node = new TreeNode( list, node);
      node->append( node);
      layoutChanged = true;

    } else {
      // This is existing data, update the value.
      TreeNode* node = (*node)[ row];
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
    this->model_->updated( node, 1, (*node)[ node->size()-1], 1);
  }
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

