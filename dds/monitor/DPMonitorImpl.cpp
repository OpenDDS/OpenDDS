/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DPMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DPMonitorImpl::DPMonitorImpl(DomainParticipantImpl* dp,
              OpenDDS::DCPS::DomainParticipantReportDataWriter_ptr dp_writer)
  : dp_(dp),
    dp_writer_(DomainParticipantReportDataWriter::_duplicate(dp_writer))
{
  char host[256];
  ACE_OS::hostname(host, 256);
  hostname_ = host;
  pid_ = ACE_OS::getpid();
}

DPMonitorImpl::~DPMonitorImpl()
{
}

void
DPMonitorImpl::report() {
  if (!CORBA::is_nil(this->dp_writer_.in())) {
    DomainParticipantReport report;
    report.host       = this->hostname_.c_str();
    report.pid        = this->pid_;
    report.dp_id      = this->dp_->get_id();
    report.domain_id  = this->dp_->get_domain_id();
    DomainParticipantImpl::TopicIdVec topics;
    this->dp_->get_topic_ids(topics);
    CORBA::ULong length = 0;
    report.topics.length(static_cast<CORBA::ULong>(topics.size()));
    for (DomainParticipantImpl::TopicIdVec::iterator iter = topics.begin();
         iter != topics.end();
         ++iter) {
      report.topics[length++] = *iter;
    }
    this->dp_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
