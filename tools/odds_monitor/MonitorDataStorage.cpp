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
  this->publisherIdGenerator_ = new RepoIdGenerator( 0, 0, OpenDDS::DCPS::KIND_PUBLISHER);
  this->subscriberIdGenerator_ = new RepoIdGenerator( 0, 0, OpenDDS::DCPS::KIND_SUBSCRIBER);
  this->transportIdGenerator_ = new RepoIdGenerator( 0, 0, OpenDDS::DCPS::KIND_USER);
}

void
Monitor::MonitorDataStorage::clear()
{
  this->guidToTreeMap_.clear();
  this->hostToTreeMap_.clear();
  this->processToTreeMap_.clear();
  delete this->publisherIdGenerator_;
  delete this->subscriberIdGenerator_;
  delete this->transportIdGenerator_;
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
  if( this->host < rhs.host)       return true;
  else if( this->host == rhs.host) return false;
  else                             return this->pid < rhs.pid;
}

