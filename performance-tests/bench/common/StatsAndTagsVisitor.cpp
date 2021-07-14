#include "StatsAndTagsVisitor.h"

namespace Bench {

StatsAndTagsVisitor::StatsAndTagsVisitor(std::unordered_set<std::string>& stat_names,
                                         std::unordered_set<std::string>& tag_names)
  : stat_names_(stat_names)
  , tag_names_(tag_names)
{
}

void StatsAndTagsVisitor::on_node_controller_report(const ReportVisitorContext& context)
{
  for (unsigned int property_index = 0; property_index < context.nc_report_->properties.length(); ++property_index) {
    const std::string name = context.nc_report_->properties[property_index].name.in();
    auto pos = name.rfind("_var_x_sample_count");
    if (pos != std::string::npos) {
      stat_names_.insert(name.substr(0, name.length() - 19));
    }
  }
}

void StatsAndTagsVisitor::on_datareader_report(const ReportVisitorContext& context)
{
  for (unsigned int tag_index = 0; tag_index < context.datareader_report_->tags.length(); ++tag_index) {
    tag_names_.insert(context.datareader_report_->tags[tag_index].in());
  }

  for (unsigned int property_index = 0; property_index < context.datareader_report_->properties.length(); ++property_index) {
    const std::string name = context.datareader_report_->properties[property_index].name.in();
    auto pos = name.rfind("_var_x_sample_count");
    if (pos != std::string::npos) {
      stat_names_.insert(name.substr(0, name.length() - 19));
    }
  }
}

void StatsAndTagsVisitor::on_datawriter_report(const ReportVisitorContext& context)
{
  for (unsigned int tag_index = 0; tag_index < context.datawriter_report_->tags.length(); ++tag_index) {
    tag_names_.insert(context.datawriter_report_->tags[tag_index].in());
  }

  for (unsigned int property_index = 0; property_index < context.datawriter_report_->properties.length(); ++property_index) {
    const std::string name = context.datawriter_report_->properties[property_index].name.in();
    auto pos = name.rfind("_var_x_sample_count");
    if (pos != std::string::npos) {
      stat_names_.insert(name.substr(0, name.length() - 19));
    }
  }
}

} // namespace Bench
