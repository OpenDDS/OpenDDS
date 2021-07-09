#include "StatBlockRawFormatter.h"

#include <PropertyStatBlock.h>

namespace Bench {

int StatBlockRawFormatter::format(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
{
  std::map<std::string, std::vector<Bench::SimpleStatBlock> > consolidated_stat_vec_map;

  for (unsigned int node_index = 0; node_index < report.node_reports.length(); ++node_index) {
    const Bench::TestController::NodeReport& nc_report = report.node_reports[node_index];

    for (auto it = parse_parameters.stats.begin(); it != parse_parameters.stats.end(); ++it) {
      Bench::ConstPropertyStatBlock cpsb(nc_report.properties, *it);
      if (cpsb) {
        consolidated_stat_vec_map[*it].push_back(cpsb.to_simple_stat_block());
      }
    }

    for (unsigned int worker_index = 0; worker_index < nc_report.worker_reports.length(); ++worker_index) {
      const Bench::WorkerReport& worker_report = nc_report.worker_reports[worker_index];

      for (unsigned int participant_index = 0; participant_index < worker_report.process_report.participants.length(); ++participant_index) {
        const Builder::ParticipantReport& participant_report = worker_report.process_report.participants[participant_index];

        for (unsigned int subscriber_index = 0; subscriber_index < participant_report.subscribers.length(); ++subscriber_index) {
          const Builder::SubscriberReport& subscriber_report = participant_report.subscribers[subscriber_index];

          for (unsigned int datareader_index = 0; datareader_index < subscriber_report.datareaders.length(); ++datareader_index) {
            const Builder::DataReaderReport& datareader_report = subscriber_report.datareaders[datareader_index];

            // Check to see if tags match (empty tags always match)
            if (!parse_parameters.tags.empty()) {
              bool found = false;
              for (unsigned int tags_index = 0; tags_index < datareader_report.tags.length(); ++tags_index) {
                const std::string tag(datareader_report.tags[tags_index]);
                if (parse_parameters.tags.find(tag) != parse_parameters.tags.end()) {
                  found = true;
                  break;
                }
              }
              if (!found) {
                continue;
              }
            }

            // Consolidate Stats
            for (auto it = parse_parameters.stats.begin(); it != parse_parameters.stats.end(); ++it) {
              Bench::ConstPropertyStatBlock cpsb(datareader_report.properties, *it);
              if (cpsb) {
                consolidated_stat_vec_map[*it].push_back(cpsb.to_simple_stat_block());
              }
            }

          }

        }

        for (unsigned int publisher_index = 0; publisher_index < participant_report.publishers.length(); ++publisher_index) {
          const Builder::PublisherReport& publisher_report = participant_report.publishers[publisher_index];

          for (unsigned int datawriter_index = 0; datawriter_index < publisher_report.datawriters.length(); ++datawriter_index) {
            const Builder::DataWriterReport& datawriter_report = publisher_report.datawriters[datawriter_index];

            // Check to see if tags match (empty tags always match)
            if (!parse_parameters.tags.empty()) {
              bool found = false;
              for (unsigned int tags_index = 0; tags_index < datawriter_report.tags.length(); ++tags_index) {
                const std::string tag(datawriter_report.tags[tags_index]);
                if (parse_parameters.tags.find(tag) != parse_parameters.tags.end()) {
                  found = true;
                  break;
                }
              }
              if (!found) {
                continue;
              }
            }

            // Consolidate Stats
            for (auto it = parse_parameters.stats.begin(); it != parse_parameters.stats.end(); ++it) {
              Bench::ConstPropertyStatBlock cpsb(datawriter_report.properties, *it);
              if (cpsb) {
                consolidated_stat_vec_map[*it].push_back(cpsb.to_simple_stat_block());
              }
            }

          }

        }

      }

    }

  }

  for (auto it = parse_parameters.stats.begin(); it != parse_parameters.stats.end(); ++it) {
    consolidate(consolidated_stat_vec_map[*it]).pretty_print(output_stream, *it);
  }

  return EXIT_SUCCESS;
}

} // namespace Bench
