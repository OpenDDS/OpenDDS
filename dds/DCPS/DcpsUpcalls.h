#ifndef OPENDDS_DCPS_DCPS_UPCALLS_H
#define OPENDDS_DCPS_DCPS_UPCALLS_H

#include "TimeTypes.h"
#include "DataReaderCallbacks.h"
#include "DataWriterCallbacks.h"
#include "ConditionVariable.h"
#include "ReactorTask.h"
#include "debug.h"
#include "PoolAllocator.h"
#include "dcps_export.h"

#include <dds/DdsDcpsGuidC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#include <ace/Task.h>
#include <ace/Thread_Mutex.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export DcpsUpcalls : public ACE_Task_Base {
public:
  DcpsUpcalls(DataReaderCallbacks_rch drr,
              const GUID_t& reader,
              const WriterAssociation& wa,
              bool active,
              DataWriterCallbacks_rch dwr);

  int svc();

  void writer_done();

private:
  DataReaderCallbacks_wrch drr_;
  const GUID_t& reader_;
  const WriterAssociation& wa_;
  bool active_;
  DataWriterCallbacks_wrch dwr_;
  bool reader_done_, writer_done_;
  ACE_Thread_Mutex mtx_;
  ConditionVariable<ACE_Thread_Mutex> cnd_;
};

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_DCPS_UPCALLS_H
