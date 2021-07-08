#include "SummaryRawFormatter.h"

#include <PropertyStatBlock.h>
#include <util.h>

int SummaryRawFormatter::format(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
{
  std::unordered_set<std::string> stats;
  std::unordered_set<std::string> tags;

  Bench::gather_stats_and_tags(report, stats, tags);

  if (!parse_parameters.tags.empty()) {
    tags = parse_parameters.tags;
  }

  if (!parse_parameters.stats.empty()) {
    stats = parse_parameters.stats;
  }

  typedef std::map<std::string, std::vector<Bench::SimpleStatBlock> > stat_vec_map;

  stat_vec_map untagged_stat_vecs;
  std::map<std::string, stat_vec_map> tagged_stat_vecs;

  for (unsigned int node_index = 0; node_index < report.node_reports.length(); ++node_index) {
    const Bench::TestController::NodeReport& nc_report = report.node_reports[node_index];

    for (auto it = stats.begin(); it != stats.end(); ++it) {
      Bench::ConstPropertyStatBlock cpsb(nc_report.properties, *it);
      if (cpsb) {
        untagged_stat_vecs[*it].push_back(cpsb.to_simple_stat_block());
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

            // Consolidate Stats
            for (auto it = stats.begin(); it != stats.end(); ++it) {
              Bench::ConstPropertyStatBlock cpsb(datareader_report.properties, *it);
              if (cpsb) {
                const auto& ssb = cpsb.to_simple_stat_block();
                untagged_stat_vecs[*it].push_back(ssb);
                for (unsigned int tags_index = 0; tags_index < datareader_report.tags.length(); ++tags_index) {
                  const std::string tag(datareader_report.tags[tags_index]);
                  if (tags.find(tag) != tags.end()) {
                    tagged_stat_vecs[tag][*it].push_back(ssb);
                  }
                }
              }
            }

          }

        }

        for (unsigned int publisher_index = 0; publisher_index < participant_report.publishers.length(); ++publisher_index) {
          const Builder::PublisherReport& publisher_report = participant_report.publishers[publisher_index];

          for (unsigned int datawriter_index = 0; datawriter_index < publisher_report.datawriters.length(); ++datawriter_index) {
            const Builder::DataWriterReport& datawriter_report = publisher_report.datawriters[datawriter_index];

            // Consolidate Stats
            for (auto it = stats.begin(); it != stats.end(); ++it) {
              Bench::ConstPropertyStatBlock cpsb(datawriter_report.properties, *it);
              if (cpsb) {
                const auto& ssb = cpsb.to_simple_stat_block();
                untagged_stat_vecs[*it].push_back(ssb);
                for (unsigned int tags_index = 0; tags_index < datawriter_report.tags.length(); ++tags_index) {
                  const std::string tag(datawriter_report.tags[tags_index]);
                  if (tags.find(tag) != tags.end()) {
                    tagged_stat_vecs[tag][*it].push_back(ssb);
                  }
                }
              }
            }

          }

        }

      }

    }

  }

  for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
    auto stat_pos = untagged_stat_vecs.find(*stat_it);
    if (stat_pos != untagged_stat_vecs.end()) {
      output_stream << std::endl;
      consolidate(stat_pos->second).pretty_print(output_stream, *stat_it);
    }
  }
  for (auto tags_it = tags.begin(); tags_it != tags.end(); ++tags_it) {
    auto tag_pos = tagged_stat_vecs.find(*tags_it);
    if (tag_pos != tagged_stat_vecs.end()) {
      output_stream << std::endl;
      output_stream << "---=== Stats For Tag '" << *tags_it << "' ===---" << std::endl;
      for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
        auto stat_pos = tag_pos->second.find(*stat_it);
        if (stat_pos != tag_pos->second.end()) {
          output_stream << std::endl;
          consolidate(stat_pos->second).pretty_print(output_stream, *stat_it);
        }
      }

    }
  }
  return EXIT_SUCCESS;
}
