#include "ParticipantStatisticsReporter.h"

namespace RtpsRelay {

const Config* ParticipantStatisticsReporter::config;
ParticipantStatisticsDataWriter_var ParticipantStatisticsReporter::writer;
const char* ParticipantStatisticsReporter::topic_name;

}
