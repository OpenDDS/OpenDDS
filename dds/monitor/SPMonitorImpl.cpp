/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SPMonitorImpl.h"
#include "MonitorFactoryImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


SPMonitorImpl::SPMonitorImpl(MonitorFactoryImpl* monitor_factory,
                             Service_Participant* /*sp*/)
    : monitor_factory_(monitor_factory)
{
  char host[256];
  ACE_OS::hostname(host, 256);
  hostname_ = host;
  pid_ = ACE_OS::getpid();
}

SPMonitorImpl::~SPMonitorImpl()
{
}

void
SPMonitorImpl::report()
{
  if (CORBA::is_nil(this->sp_writer_.in())) {
    this->sp_writer_ = this->monitor_factory_->get_sp_writer();
  }

  // If the SP writer is not available, it is too soon to report
  if (!CORBA::is_nil(this->sp_writer_.in())) {
    ServiceParticipantReport report;
    report.host = this->hostname_.c_str();
    report.pid  = this->pid_;
    DDS::DomainParticipantFactory_var pf = TheParticipantFactory;

    OpenDDS::DCPS::DomainParticipantFactoryImpl* pi = dynamic_cast<DomainParticipantFactoryImpl*>(pf.in());
    if (!pi) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) SPMonitorImpl::report():")
        ACE_TEXT(" failed to obtain DomainParticipantFactoryImpl.\n")));
      return;
    }

    const DomainParticipantFactoryImpl::DPMap& participants = pi->participants();
    CORBA::ULong length = 0;
    for (DomainParticipantFactoryImpl::DPMap::const_iterator mapIter = participants.begin();
         mapIter != participants.end();
         ++mapIter) {
      for (DomainParticipantFactoryImpl::DPSet::const_iterator iter = mapIter->second.begin();
           iter != mapIter->second.end();
           ++iter) {
        report.domain_participants.length(length+1);
        report.domain_participants[length] = (*iter)->get_id();
        length++;
      }
    }
    length = 0;
    // TODO: Redo the transport-related monitor publishing here...
    //const TransportFactory::ImplMap& transports =
    //  TransportFactory::instance()->get_transport_impl_map();
    //report.transports.length(static_cast<CORBA::ULong>(transports.size()));
    //for (TransportFactory::ImplMap::const_iterator mapIter = transports.begin();
    //     mapIter != transports.end();
    //     ++mapIter) {
    //  report.transports[length++] = mapIter->first;
    //}
    this->sp_writer_->write(report, DDS::HANDLE_NIL);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
