#ifndef CONNECTION_RECORD_LOGGER_H
#define CONNECTION_RECORD_LOGGER_H

#include "dds/DdsDcpsDomainC.h"

namespace OpenDDS {
namespace Test {

void install_connection_record_logger(DDS::DomainParticipant_var participant);

} // Test
} // OpenDDS

#endif /* CONNECTION_RECORD_LOGGER_H */
