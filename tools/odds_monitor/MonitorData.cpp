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

#include "dds/DCPS/Service_Participant.h"

Monitor::MonitorData::MonitorData( const Options& options, MonitorDataModel* model)
 : enabled_( true),
   options_( options),
   model_( model),
   dataSource_( 0)
{
  this->dataSource_ = new MonitorTask( this, this->options_);
  this->dataSource_->start();
}

Monitor::MonitorData::~MonitorData()
{
  this->disable();
  delete this->dataSource_;
}

void
Monitor::MonitorData::disable()
{
  this->enabled_ = false;
}

void
Monitor::MonitorData::setRepoIor( const QString& ior)
{
  // Delegate to the data source.
  this->dataSource_->setRepoIor( ior.toStdString());
}

