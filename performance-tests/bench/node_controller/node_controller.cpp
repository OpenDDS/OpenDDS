#include <vector>
#include <string>
#include <iostream>
#include <memory>

#include "ace/Process_Manager.h"
#include "ace/OS_NS_stdlib.h"

ACE_Process_Manager* process_manager = ACE_Process_Manager::instance();
std::string dds_root;

/**
 * Common Group Interface
 */
class Group {
public:
  typedef std::shared_ptr<Group> Ptr;

  virtual ~Group()
  {
  }

  virtual void
  dump(size_t indent = 0)
  {
    indent++;
  }

  virtual bool
  run()
  {
    return true;
  }

  virtual bool
  failed()
  {
    return true;
  }
};

/**
 * A group of workers that all have the same config.
 */
class WorkerGroup : public Group {
public:
  typedef std::shared_ptr<WorkerGroup> Ptr;

  std::string config_;
  size_t count_ = 1;

  std::vector<pid_t> pids_;

  virtual ~WorkerGroup()
  {
  }

  virtual void
  dump(size_t indent = 0)
  {
    std::cout << std::string(indent, ' ') << config_ << " x " << count_ << std::endl;
  }

  virtual bool
  run()
  {
    ACE_Process_Options proc_opts;
    const std::string config = dds_root + config_;
    proc_opts.command_line("%s/performance-tests/bench/worker/worker %s true", dds_root.c_str(), config.c_str());
    for (size_t i = 0; i < count_; i++) {
      pid_t pid = process_manager->spawn(proc_opts);
      if (pid == ACE_INVALID_PID) {
        return true;
      }
      pids_.push_back(pid);
    }
    return false;
  }

  virtual bool
  failed()
  {
    ACE_exitcode status;
    for (auto pid : pids_) {
      if (process_manager->wait(pid, &status) == ACE_INVALID_PID || status != 0) {
        return true;
      }
    }
    return false;
  }
};

/**
 * Group that contatins other groups
 */
class SuperGroup : public Group {
public:
  typedef std::shared_ptr<SuperGroup> Ptr;

  size_t count_ = 1;

  std::vector<Group::Ptr> groups_;

  virtual ~SuperGroup()
  {
  }

  WorkerGroup::Ptr
  create_worker_group(const std::string& config, size_t count = 1)
  {
    WorkerGroup::Ptr group(new WorkerGroup);
    group->config_ = config;
    group->count_ = count;
    groups_.push_back(group);
    return group;
  }

  SuperGroup::Ptr
  create_super_group(size_t count = 1)
  {
    SuperGroup::Ptr group(new SuperGroup);
    group->count_ = count;
    groups_.push_back(group);
    return group;
  }

  virtual void
  dump(size_t indent = 0)
  {
    std::string indent_str(indent, ' ');
    indent += 2;
    std::cout << indent_str << '{' << std::endl;
    for (auto group : groups_) {
      group->dump(indent);
    }
    std::cout << indent_str << "} x " << count_ << std::endl;
  }

  virtual bool
  run()
  {
    for (size_t i = 0; i < count_; i++) {
      for (auto group : groups_) {
        if (group->run()) {
          return true;
        }
      }
    }
    return false;
  }

  virtual bool
  failed()
  {
    for (auto group : groups_) {
      if (group->failed()) {
        return true;
      }
    }
    return false;
  }
};

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  ACE_UNUSED_ARG(argc);
  ACE_UNUSED_ARG(argv);

  dds_root = ACE_OS::getenv("DDS_ROOT");
  if (dds_root.empty()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("DDS_ROOT isn't defined\n")));
    return 1;
  }
  SuperGroup root;

  std::string configs = "/performance-tests/bench/worker/configs";
  SuperGroup::Ptr sites = root.create_super_group(10);
  sites->create_worker_group(configs + "/jfti_sim_daemon_config.json");
  sites->create_worker_group(configs + "/jfti_sim_worker_config.json");
  root.create_worker_group(configs + "/jfti_sim_master_config.json");

  if (!std::string(ACE_OS::getenv("DUMP_NODE_CONFIG")).empty()) {
    root.dump();
  }
  if (root.run()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to run a worker\n")));
    return 1;
  }
  if (root.failed()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("A worker failed\n")));
    return 1;
  }
  return 0;
}
