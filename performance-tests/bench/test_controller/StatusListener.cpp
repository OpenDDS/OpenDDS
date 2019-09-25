#include "StatusListener.h"

using namespace Bench::NodeController;

std::ostream& operator<<(std::ostream& os, const Status& node)
{
  os << node.node_id;
  if (node.name.in()[0]) {
    os << " \"" << node.name.in() << '\"';
  }
  return os;
}

void StatusListener::on_data_available(DDS::DataReader_ptr reader)
{
  std::lock_guard<std::mutex> lock(nodes_mutex_);

  StatusDataReader_var status_reader_impl = StatusDataReader::_narrow(reader);
  if (!status_reader_impl) {
    std::cerr << "Error: StatusListener: narrow status reader failed" << std::endl;
    return;
  }

  StatusSeq statuses;
  DDS::SampleInfoSeq info;
  DDS::ReturnCode_t rc = status_reader_impl->take(
    statuses, info,
    DDS::LENGTH_UNLIMITED,
    DDS::ANY_SAMPLE_STATE,
    DDS::ANY_VIEW_STATE,
    DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    std::cerr << "Error: StatusListener: Take failed" << std::endl;
    return;
  }

  for (size_t i = 0; i < statuses.length(); i++) {
    auto& node = statuses[i];
    Nodes::iterator iter = nodes_.find(node.node_id);
    bool already_discovered = iter != nodes_.end();
    if (info[i].valid_data) {
      if (in_discovery_period_) {
        std::cout << "Discovered Node " << node << std::endl;
        nodes_[node.node_id] = node;
      } else if (!already_discovered) {
        std::cerr << "Warning: Discovered node " << node << " outside discovery period" << std::endl;
      } else {
        nodes_[node.node_id] = node;
      }
    } else if (info[i].instance_state & DDS::NOT_ALIVE_INSTANCE_STATE && already_discovered) {
      nodes_.erase(iter);
    }
  }
}

Nodes StatusListener::get_available_nodes()
{
  std::lock_guard<std::mutex> lock(nodes_mutex_);
  in_discovery_period_ = false;
  Nodes rv;
  for (auto i : nodes_) {
    if (i.second.state == AVAILABLE) {
      rv[i.first] = i.second;
    } else {
      std::cerr << "Warning: Node " << i.second << " is not available and will not be used" << std::endl;
    }
  }
  return rv;
}
