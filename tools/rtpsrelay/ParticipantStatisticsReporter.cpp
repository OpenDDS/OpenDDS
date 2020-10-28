#include "ParticipantStatisticsReporter.h"

namespace RtpsRelay {

const Config* ParticipantStatisticsReporter::config;
ParticipantStatisticsDataWriter_var ParticipantStatisticsReporter::writer;
CORBA::String_var ParticipantStatisticsReporter::topic_name;

}
