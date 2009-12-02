/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MonitorData.h"
#include "Options.h"
#include "MonitorTask.h"
#include "MonitorDataModel.h"
#include "MonitorDataStorage.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/monitor/monitorC.h"

#include <sstream>    // For stub
#include "TreeNode.h" // For stub?

Monitor::MonitorData::MonitorData( const Options& options, MonitorDataModel* model)
 : enabled_( true),
   options_( options),
   model_( model),
   dataSource_( 0),
   storage_( new MonitorDataStorage())
{
  this->dataSource_ = new MonitorTask( this, this->options_);
  this->dataSource_->start();
}

Monitor::MonitorData::~MonitorData()
{
  this->disable();
  delete this->dataSource_;
  delete this->storage_;
}

void
Monitor::MonitorData::disable()
{
  this->enabled_ = false;
}

void
Monitor::MonitorData::getIorList( QList<QString>& iorList)
{
  for( MonitorTask::IorKeyMap::const_iterator
       location = this->dataSource_->iorKeyMap().begin();
       location != this->dataSource_->iorKeyMap().end();
       ++location) {
    iorList.append( QString( location->first.c_str()));
  }
}

bool
Monitor::MonitorData::setRepoIor( const QString& ior)
{
  if( !this->enabled_) {
    return false;
  }

  // Return successfully if the requested repository is already the
  // active repository.
  if( this->storage_->activeIor() == ior.toStdString()) {
    return true;
  }

  // Set the repository IOR.
  MonitorTask::RepoKey key
    = this->dataSource_->setRepoIor( ior.toStdString());

  // Clear existing data.
  this->clearData();

  // Switch to the new repository.
  if( this->dataSource_->setActiveRepo( key)) {
    this->storage_->activeIor() = ior.toStdString();
this->stubmodelchange();
    return true;

  } else {
    return false;
  }
}

bool
Monitor::MonitorData::removeRepo( const QString& ior)
{
  if( !this->enabled_) {
    return false;
  }

  // Check if this is the currently monitored repository, and clear its
  // data if it is in the view.
  if( this->storage_->activeIor() == ior.toStdString()) {
    this->clearData();
  }

  // Look up the index for this IOR and remove it from the service.
  return true;
}

bool
Monitor::MonitorData::clearData()
{
  if( !this->enabled_) {
    return false;
  }

  // Copy out the previous header settings.
  QList< QVariant> list;
  int cols = this->model_->columnCount();
  for( int index = 0; index < cols; ++index) {
     QString value
       = this->model_->headerData( index, Qt::Horizontal).toString();
     list << value;
  }

  // Clear the mappings.
  this->storage_->clear();

  // Create a new tree to install fresh data into.
  TreeNode* root = new TreeNode( list);

  // Install the empty tree into the data model.
  this->model_->newRoot( root);

  // Clear the active IOR field.
  this->storage_->activeIor() = std::string();

  return true;
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::ServiceParticipantReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::DomainParticipantReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::TopicReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::PublisherReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::SubscriberReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::DataWriterReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::DataWriterPeriodicReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::DataReaderReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::DataReaderPeriodicReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::update(
  const OpenDDS::DCPS::TransportReport& /* data */,
  bool  /* remove */
)
{
}

void
Monitor::MonitorData::stubmodelchange()
{
  if( !this->enabled_) {
    return;
  }

  // Copy out the previous header settings.
  QList< QVariant> list;
  int cols = this->model_->columnCount();
  for( int index = 0; index < cols; ++index) {
     QString value
       = this->model_->headerData( index, Qt::Horizontal).toString();
     list << value;
  }
  TreeNode* root = new TreeNode( list);

  static int which = 0;
  TreeNode* parent = root;
  if( ++which%2) {
    for( int row = 0; row < 4; ++row) {
      std::stringstream buffer1;
      buffer1 << "SomeProperty at " << row << std::ends;
      QString element(buffer1.str().c_str());

      std::stringstream buffer2;
      buffer2 << "some value with which = " << which << " at " << row << std::ends;
      QString value(buffer2.str().c_str());

      QList<QVariant> data;
      data << element << value;

      TreeNode* node = new TreeNode( data, parent);
      parent->append( node);
    }

  } else {
    for( int row = 0; row < 4; ++row) {
      std::stringstream buffer1;
      buffer1 << "AnotherProperty at " << row << std::ends;
      QString element(buffer1.str().c_str());

      std::stringstream buffer2;
      buffer2 << "another value with which = " << which << " at " << row << std::ends;
      QString value(buffer2.str().c_str());

      QList<QVariant> data;
      data << element << value;

      TreeNode* node = new TreeNode( data, parent);
      parent->append( node);
    }
  }

  this->model_->newRoot( root);
}

