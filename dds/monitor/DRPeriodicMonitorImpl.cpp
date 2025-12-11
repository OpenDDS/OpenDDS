/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DRPeriodicMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {


DRPeriodicMonitorImpl::DRPeriodicMonitorImpl(DCPS::DataReaderImpl* dr,
                                             DataReaderPeriodicReportDataWriter_ptr dr_per_writer)
  : dr_(dr),
    dr_per_writer_(DataReaderPeriodicReportDataWriter::_duplicate(dr_per_writer))
{
}

DRPeriodicMonitorImpl::~DRPeriodicMonitorImpl()
{
}

void
DRPeriodicMonitorImpl::report() {
  if (!CORBA::is_nil(this->dr_per_writer_.in())) {
    DataReaderPeriodicReport report;
    report.dr_id   = dr_->get_guid();
    //report.associations = dr_->
    this->dr_per_writer_->write(report, DDS::HANDLE_NIL);
  }
}


}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
