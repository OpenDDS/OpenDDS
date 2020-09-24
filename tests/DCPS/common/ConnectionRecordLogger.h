#ifndef CONNECTION_RECORD_LOGGER_H
#define CONNECTION_RECORD_LOGGER_H

#include "dds/DdsDcpsDomainC.h"
#include "common_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Test {

void Common_Export install_connection_record_logger(DDS::DomainParticipant_var participant);

} // Test
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* CONNECTION_RECORD_LOGGER_H */
