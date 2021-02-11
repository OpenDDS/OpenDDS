#pragma once

#include "BenchC.h"

#include "Action.h"

namespace Bench {

class ActionManager {
public:
  explicit ActionManager(const ActionConfigSeq& configs, ActionReportSeq& reports,
    Builder::ReaderMap& reader_map, Builder::WriterMap& writer_map, Builder::ContentFilteredTopicMap& cft_map);

  void action_start();
  void test_start();
  void test_stop();
  void action_stop();

  using action_factory = std::function<std::shared_ptr<Action>()>;
  using action_factory_map = std::map<std::string, action_factory>;

  static bool register_action_factory(const std::string& name, const action_factory& factory);
  static std::shared_ptr<Action> create_action(const std::string& name);

  class Registration {
  public:
    Registration(const std::string& name, const action_factory& factory);
  };

protected:
  static std::mutex s_mutex;
  static action_factory_map s_factory_map;

  std::vector<std::shared_ptr<Action>> actions_;
};

}
