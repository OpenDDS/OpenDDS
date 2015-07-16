/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "DataLinkCleanupTask.h"

#include "dds/DCPS/transport/framework/EntryExit.h"

OpenDDS::DCPS::DataLinkCleanupTask::DataLinkCleanupTask()
{
  DBG_ENTRY_LVL("DataLinkCleanupTask", "DataLinkCleanupTask",6);
}

OpenDDS::DCPS::DataLinkCleanupTask::~DataLinkCleanupTask()
{
  DBG_ENTRY_LVL("DataLinkCleanupTask", "~DataLinkCleanupTask",6);
}

void
OpenDDS::DCPS::DataLinkCleanupTask::execute(DataLink_rch& dl)
{
  DBG_ENTRY_LVL("DataLinkCleanupTask", "execute",6);

  // Assumes that the DataLink is safe for now.
  // ciju: I don't believe there are any thread issues here. If any
  // the risk seems minimal.
  // Not sure about the above statement. Associations could change while
  // the Id sequence is being created. That could be trouble.

  // Yan: It should be safe to remove now as we made the changes to take a
  // snapshot of associations in datalink when reconnect fails and removes
  // associations based on the image. It won't remove the new assiciations
  // added to the datalink.
  dl->clear_associations();
}
