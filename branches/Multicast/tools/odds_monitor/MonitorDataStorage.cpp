/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MonitorDataStorage.h"
#include "TreeNode.h"

#include "dds/DCPS/RepoIdGenerator.h"

#include <sstream>

Monitor::MonitorDataStorage::MonitorDataStorage()
 : publisherIdGenerator_(
     new RepoIdGenerator( 0, 0, OpenDDS::DCPS::KIND_PUBLISHER)
   ),
   subscriberIdGenerator_(
     new RepoIdGenerator( 0, 0, OpenDDS::DCPS::KIND_SUBSCRIBER)
   ),
   transportIdGenerator_(
     new RepoIdGenerator( 0, 0, OpenDDS::DCPS::KIND_USER)
   )
{
}

Monitor::MonitorDataStorage::~MonitorDataStorage()
{
  delete this->publisherIdGenerator_;
  delete this->subscriberIdGenerator_;
  delete this->transportIdGenerator_;
}

void
Monitor::MonitorDataStorage::clear()
{
  this->guidToTreeMap_.clear();
}

