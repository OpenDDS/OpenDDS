#include "WriteAction.h"

#include "MemFunHandler.h"

namespace {

uint32_t one_at_a_time_hash(const uint8_t* key, size_t length) {
  size_t i = 0;
  uint32_t hash = 0;
  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

}

namespace Stool {

WriteAction::WriteAction(ACE_Proactor& proactor) : proactor_(proactor), started_(false), stopped_(false), write_period_(1, 0) {
}

bool WriteAction::init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers, Builder::WriterMap& writers) {

  std::unique_lock<std::mutex> lock(mutex_);
  Action::init(config, report, readers, writers);

  if (writers_by_index_.empty()) {
    std::stringstream ss;
    ss << "WriteAction '" << config.name << "' is missing a writer" << std::flush;
    throw std::runtime_error(ss.str());
  }

  data_dw_ = DataDataWriter::_narrow(writers_by_index_[0]->get_dds_datawriter());
  if (!data_dw_) {
    std::stringstream ss;
    ss << "WriteAction '" << config.name << "' is missing a valid Stool::Data datawriter" << std::flush;
    throw std::runtime_error(ss.str());
  }

  std::string name(config.name.in());
  data_.key = one_at_a_time_hash(reinterpret_cast<const uint8_t*>(name.data()), name.size());

  size_t data_buffer_bytes = 256;
  auto data_buffer_bytes_prop = get_property(config.params, "data_buffer_bytes", Builder::PVK_ULL);
  if (data_buffer_bytes_prop) {
    data_buffer_bytes = data_buffer_bytes_prop->value.ull_prop();
  }
  data_.buffer.length(data_buffer_bytes);

  auto write_period_prop = get_property(config.params, "write_period", Builder::PVK_TIME);
  if (write_period_prop) {
    write_period_ = ACE_Time_Value(write_period_prop->value.time_prop().sec, write_period_prop->value.time_prop().nsec / 1e3);
  }

  handler_.reset(new MemFunHandler<WriteAction>(&WriteAction::do_write, *this));

  return true;
}

void WriteAction::start() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!started_) {
    instance_ = data_dw_->register_instance(data_);
    started_ = true;
    proactor_.schedule_timer(*handler_, nullptr, write_period_);
  }
}

void WriteAction::stop() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    stopped_ = true;
    proactor_.cancel_timer(*handler_);
    data_dw_->unregister_instance(data_, instance_);
  }
}

void WriteAction::do_write() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    data_.created.time = Builder::get_time();
    DDS::ReturnCode_t result = data_dw_->write(data_, 0);
    if (result != DDS::RETCODE_OK) {
      std::cout << "Error during WriteAction::do_write()'s call to datawriter::write()" << std::endl;
    }
    proactor_.schedule_timer(*handler_, nullptr, write_period_);
  }
}

}

