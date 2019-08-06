#include "ActionManager.h"

namespace Bench {

std::mutex ActionManager::s_mutex;
ActionManager::action_factory_map ActionManager::s_factory_map;

ActionManager::ActionManager(const Bench::ActionConfigSeq& configs, Bench::ActionReportSeq& reports, Builder::ReaderMap& reader_map, Builder::WriterMap& writer_map) {
  reports.length(configs.length());
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    auto action = create_action(configs[i].type.in());
    if (!action) {
      std::stringstream ss;
      ss << "Invalid action name '" << configs[i].type << "'" << std::flush;
      throw std::runtime_error(ss.str());
    }
    action->init(configs[i], reports[i], reader_map, writer_map);
    actions_.push_back(action);
  }
}

void ActionManager::start() {
  for (auto it = actions_.begin(); it != actions_.end(); ++it) {
    (*it)->start();
  }
}

void ActionManager::stop() {
  for (auto it = actions_.begin(); it != actions_.end(); ++it) {
    (*it)->stop();
  }
}

bool ActionManager::register_action_factory(const std::string& name, const ActionManager::action_factory& factory) {
  std::unique_lock<std::mutex> lock(s_mutex);
  bool result = false;

  auto it = s_factory_map.find(name);
  if (it == s_factory_map.end()) {
    s_factory_map[name] = factory;
    result = true;
  }
  return result;
}

std::shared_ptr<Action> ActionManager::create_action(const std::string& name) {
  std::unique_lock<std::mutex> lock(s_mutex);
  std::shared_ptr<Action> result;

  auto it = s_factory_map.find(name);
  if (it != s_factory_map.end()) {
    result = (it->second)();
  }
  return result;
}

ActionManager::Registration::Registration(const std::string& name, const ActionManager::action_factory& factory) {
  Builder::Log::log() << "Action registration created for name '" << name << "'" << std::endl;
  if (!register_action_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register action factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

}

