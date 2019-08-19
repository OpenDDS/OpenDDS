#include "TransportInstance.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"

namespace Builder {

TransportInstance::TransportInstance(const TransportInstanceConfig& config)
  : name_(config.name.in())
{
  Log::log() << "Creating instance '" << name_ << "' of type " << config.type << std::endl;
  config_ = OpenDDS::DCPS::TransportRegistry::instance()->get_config(config.name.in());
  if (!config_) {
    config_ = OpenDDS::DCPS::TransportRegistry::instance()->create_config(config.name.in());
    inst_ = OpenDDS::DCPS::TransportRegistry::instance()->get_inst(config.name.in());
    if (!inst_) {
      if (std::string(config.type.in()) == "rtps_udp") {
        inst_ = OpenDDS::DCPS::TransportRegistry::instance()->create_inst(config.name.in(), "rtps_udp");
        Log::log() << "Creating instance config '" << name_ << "' of type " << config.type << std::endl;
      } else {
        throw std::runtime_error("unsupported transport instance type");
      }
    }
    config_->instances_.push_back(inst_);
    OpenDDS::DCPS::TransportRegistry::instance()->domain_default_config(config.domain, config_);
  } else {
    inst_ = OpenDDS::DCPS::TransportRegistry::instance()->get_inst(config.name.in());
    if (inst_ && std::string(config.type.in()) == inst_->transport_type_) {
      Log::log() << "Using existing instance '" << name_ << "' of type " << inst_->transport_type_ << std::endl;
    } else {
      throw std::runtime_error("mismatched transport instance type");
    }
  }
}

TransportInstance::~TransportInstance() {
  auto it = config_->instances_.begin();
  while (it != config_->instances_.end()) {
    if (*it == inst_) {
      it = config_->instances_.erase(it);
    } else {
      ++it;
    }
  }

  OpenDDS::DCPS::TransportRegistry::instance()->remove_inst(inst_);
  inst_.reset();
  OpenDDS::DCPS::TransportRegistry::instance()->remove_config(config_);
  config_.reset();
}

}

