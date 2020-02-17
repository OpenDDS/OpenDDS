#include "ForwardAction.h"

#include "MemFunHandler.h"

namespace {

const ACE_Time_Value ZERO(0, 0);

}

namespace Bench {

ForwardAction::ForwardAction(ACE_Proactor& proactor) : proactor_(proactor), started_(false), stopped_(false), prevent_copy_(false), force_copy_(false), copy_threshold_(0), queue_first_(0), queue_last_(0) {
}

bool ForwardAction::init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers, Builder::WriterMap& writers) {

  std::unique_lock<std::mutex> lock(mutex_);
  Action::init(config, report, readers, writers);

  if (writers_by_index_.empty()) {
    std::stringstream ss;
    ss << "ForwardAction '" << config.name << "' is missing a writer" << std::flush;
    throw std::runtime_error(ss.str());
  }

  if (readers_by_index_.empty()) {
    std::stringstream ss;
    ss << "ForwardAction '" << config.name << "' is missing a reader" << std::flush;
    throw std::runtime_error(ss.str());
  }

  for (auto it = writers_by_index_.begin(); it != writers_by_index_.end(); ++it) {
    DataDataWriter_var data_dw = DataDataWriter::_narrow((*it)->get_dds_datawriter());
    if (!data_dw) {
      std::stringstream ss;
      ss << "ForwardAction::init() - writer '" << config.name << "' is not a Bench::Data datawriter" << std::flush;
      throw std::runtime_error(ss.str());
    }
    data_dws_.push_back(data_dw);
  }

  for (auto it = readers_by_index_.begin(); it != readers_by_index_.end(); ++it) {
    WorkerDataReaderListener* wdrl = dynamic_cast<WorkerDataReaderListener*>((*it)->get_dds_datareaderlistener().in());
    if (!wdrl) {
      std::stringstream ss;
      ss << "ForwardAction::init() - reader '" << config.name << "' does not have a WorkerDataReaderListener (\"bench_drl\") listener" << std::flush;
      throw std::runtime_error(ss.str());
    }
    registrations_.push_back(std::make_shared<Registration>(*this, wdrl));
  }

  auto force_copy_prop = get_property(config.params, "force_copy", Builder::PVK_ULL);
  if (force_copy_prop) {
    force_copy_ = (force_copy_prop->value.ull_prop() != 0);
  }

  auto prevent_copy_prop = get_property(config.params, "prevent_copy", Builder::PVK_ULL);
  if (prevent_copy_prop) {
    prevent_copy_ = (prevent_copy_prop->value.ull_prop() != 0);
  }

  if (force_copy_ && prevent_copy_) {
    std::stringstream ss;
    ss << "ForwardAction::init() -  force_copy and prevent_copy cannot both be set true" << std::flush;
    throw std::runtime_error(ss.str());
  }

  auto copy_threshold_prop = get_property(config.params, "copy_threshold", Builder::PVK_ULL);
  if (copy_threshold_prop) {
    copy_threshold_ = copy_threshold_prop->value.ull_prop();
  }

  size_t queue_size = data_dws_.size() + 1;
  auto queue_size_prop = get_property(config.params, "queue_size", Builder::PVK_ULL);
  if (queue_size_prop) {
    queue_size = queue_size_prop->value.ull_prop();
  }

  // because of the way we're doing indices, vector size should be one more than effective queue size
  // hence 0 and 1 are not valid vector sizes and will not work
  data_queue_.resize(queue_size > 0 ? queue_size + 1 : 2);

  handler_.reset(new MemFunHandler<ForwardAction>(&ForwardAction::do_writes, *this));

  return true;
}

void ForwardAction::start() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!started_) {
    started_ = true;
  }
}

void ForwardAction::stop() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    stopped_ = true;
    queue_not_full_.notify_all();
  }
}

void ForwardAction::on_data(const Data& data) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    bool use_queue = (force_copy_ || (data_dws_.size() > copy_threshold_ && !prevent_copy_));
    if (use_queue) {
      bool queue_full = (((queue_last_ + 1) % data_queue_.size()) == queue_first_);
      while (queue_full && !stopped_) {
        queue_not_full_.wait(lock);
        queue_full = (((queue_last_ + 1) % data_queue_.size()) == queue_first_);
      }
      data_queue_[queue_last_] = data;
      queue_last_ = (queue_last_ + 1) % data_queue_.size();
      proactor_.schedule_timer(*handler_, nullptr, ZERO);
    } else {
      for (auto it = data_dws_.begin(); it != data_dws_.end(); ++it) {

        // Cache previous values of things we're going to tweak
        Builder::TimeStamp old_sent_time = data.sent_time;
        CORBA::ULong old_hop_count = data.hop_count;

        // Temporarily break const promises to modify data for resending
        Data& dangerous_data = const_cast<Data&>(data);
        dangerous_data.sent_time = Builder::get_time();
        dangerous_data.hop_count = old_hop_count + 1;

        (*it)->write(data, 0);

        // Set it back in case anyone else really needed us to keep our promises
        dangerous_data.sent_time = old_sent_time;
        dangerous_data.hop_count = old_hop_count;
      }
    }
  }
}

void ForwardAction::do_writes() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (queue_first_ != queue_last_) {
    Data& data = data_queue_[queue_first_];
    data.hop_count += 1;
    for (auto it = data_dws_.begin(); it != data_dws_.end(); ++it) {
      data.sent_time = Builder::get_time();
      (*it)->write(data, 0);
    }
    queue_first_ = (queue_first_ + 1) % data_queue_.size();
  }
  queue_not_full_.notify_all();
}

}

