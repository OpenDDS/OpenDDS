/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DRMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


DRMonitorImpl::DRMonitorImpl(DataReaderImpl* dr,
              OpenDDS::DCPS::DataReaderReportDataWriter_ptr dr_writer)
  : dr_(dr),
    dr_writer_(DataReaderReportDataWriter::_duplicate(dr_writer))
{
}

DRMonitorImpl::~DRMonitorImpl()
{
}

void
DRMonitorImpl::report() {
  if (!CORBA::is_nil(this->dr_writer_.in())) {
    DataReaderReport report;
    report.dp_id = this->dr_->get_dp_id();
    DDS::Subscriber_var sub = this->dr_->get_subscriber();
    report.sub_handle = sub->get_instance_handle();
    report.dr_id   = this->dr_->get_subscription_id();
    report.topic_id = this->dr_->get_topic_id();
    DataReaderImpl::InstanceHandleVec instances;
    this->dr_->get_instance_handles(instances);
    CORBA::ULong length = 0;
    report.instances.length(static_cast<CORBA::ULong>(instances.size()));
    for (DataReaderImpl::InstanceHandleVec::iterator iter = instances.begin();
         iter != instances.end();
         ++iter) {
      report.instances[length++] = *iter;
    }
    DataReaderImpl::WriterStatePairVec writer_states;
    this->dr_->get_writer_states(writer_states);
    length = 0;
    report.associations.length(static_cast<CORBA::ULong>(writer_states.size()));
    for (DataReaderImpl::WriterStatePairVec::iterator iter = writer_states.begin();
         iter != writer_states.end();
         ++iter) {
      report.associations[length].dw_id = iter->first;
      report.associations[length].state = iter->second;
      length++;
    }
    this->dr_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
