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

