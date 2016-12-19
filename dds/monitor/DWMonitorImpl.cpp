/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DWMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/DataWriterImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DWMonitorImpl::DWMonitorImpl(DataWriterImpl* dw,
              OpenDDS::DCPS::DataWriterReportDataWriter_ptr dw_writer)
  : dw_(dw),
    dw_writer_(DataWriterReportDataWriter::_duplicate(dw_writer))
{
}

DWMonitorImpl::~DWMonitorImpl()
{
}

void
DWMonitorImpl::report() {
  if (!CORBA::is_nil(this->dw_writer_.in())) {
    DataWriterReport report;
    report.dp_id = this->dw_->get_dp_id();
    DDS::Publisher_var pub = this->dw_->get_publisher();
    report.pub_handle = pub->get_instance_handle();
    report.dw_id   = this->dw_->get_publication_id();
    DDS::Topic_var topic = this->dw_->get_topic();
    report.topic_id = dynamic_cast<TopicImpl*>(topic.in())->get_id();
    DataWriterImpl::InstanceHandleVec instances;
    this->dw_->get_instance_handles(instances);
    CORBA::ULong length = 0;
    report.instances.length(static_cast<CORBA::ULong>(instances.size()));
    for (DataWriterImpl::InstanceHandleVec::iterator iter = instances.begin();
         iter != instances.end();
         ++iter) {
      report.instances[length++] = *iter;
    }
    DCPS::RepoIdSet readers;
    this->dw_->get_readers(readers);
    length = 0;
    report.associations.length(static_cast<CORBA::ULong>(readers.size()));
    for (DCPS::RepoIdSet::iterator iter = readers.begin();
         iter != readers.end();
         ++iter) {
      report.associations[length].dr_id = *iter;
      length++;
    }
    this->dw_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
