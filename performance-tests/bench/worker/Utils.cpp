#include "Utils.h"

#include <string>
#include <map>
#include <vector>

#include "dds/DCPS/DCPS_Utils.h"

// QosProfile

class QosProfile {
public:
  QosProfile();

  const std::string& get_name() { return name_; }

  typedef std::pair<DDS::DataReaderQos, Builder::DataReaderQosMask> DataReaderQosInfo;
  void set_datareader_qos(const DataReaderQosInfo& info, const std::string& topic_filter = std::string(""));
  DataReaderQosInfo get_datareader_qos(const std::string& topic_name = std::string("")) const;

  typedef std::pair<DDS::DataWriterQos, Builder::DataWriterQosMask> DataWriterQosInfo;
  void set_datawriter_qos(const DataWriterQosInfo& info, const std::string& topic_filter = std::string(""));
  DataWriterQosInfo get_datawriter_qos(const std::string& topic_name = std::string("")) const;

  typedef std::pair<DDS::TopicQos, Builder::TopicQosMask> TopicQosInfo;
  void set_topic_qos(const TopicQosInfo& info, const std::string& topic_filter = std::string(""));
  TopicQosInfo get_topic_qos(const std::string& topic_name = std::string("")) const;

  typedef std::pair<DDS::SubscriberQos, Builder::SubscriberQosMask> SubscriberQosInfo;
  void set_subscriber_qos(const SubscriberQosInfo& info);
  SubscriberQosInfo get_subscriber_qos() const;

  typedef std::pair<DDS::PublisherQos, Builder::PublisherQosMask> PublisherQosInfo;
  void set_publisher_qos(const PublisherQosInfo& info);
  PublisherQosInfo get_publisher_qos() const;

  typedef std::pair<DDS::DomainParticipantQos, Builder::DomainParticipantQosMask> DomainParticipantQosInfo;
  void set_participant_qos(const DomainParticipantQosInfo& info);
  DomainParticipantQosInfo get_participant_qos() const;

protected:
  typedef std::vector<std::pair<std::string, DataReaderQosInfo> > DataReaderQosInfoVec;
  typedef std::vector<std::pair<std::string, DataWriterQosInfo> > DataWriterQosInfoVec;
  typedef std::vector<std::pair<std::string, TopicQosInfo> > TopicQosInfoVec;

  std::string name_;

  DDS::DataReaderQos default_datareader_qos_;
  Builder::DataReaderQosMask default_datareader_qos_mask_;
  DataReaderQosInfoVec datareader_qos_vec_;

  DDS::DataWriterQos default_datawriter_qos_;
  Builder::DataWriterQosMask default_datawriter_qos_mask_;
  DataWriterQosInfoVec datawriter_qos_vec_;

  DDS::TopicQos default_topic_qos_;
  Builder::TopicQosMask default_topic_qos_mask_;
  TopicQosInfoVec topic_qos_vec_;

  DDS::SubscriberQos default_subscriber_qos_;
  Builder::SubscriberQosMask default_subscriber_qos_mask_;

  DDS::PublisherQos default_publisher_qos_;
  Builder::PublisherQosMask default_publisher_qos_mask_;

  DDS::DomainParticipantQos default_participant_qos_;
  Builder::DomainParticipantQosMask default_participant_qos_mask_;
};

QosProfile::QosProfile() {
}

void QosProfile::set_datareader_qos(const QosProfile::DataReaderQosInfo& info, const std::string& topic_filter) {
  if (!topic_filter.empty()) {
    datareader_qos_vec_.push_back(std::make_pair(topic_filter, info));
  } else {
    default_datareader_qos_ = info.first;
    default_datareader_qos_mask_ = info.second;
  }
}

inline bool matchTopicName(const std::string& target, const std::string& cmp) {
  return (OpenDDS::DCPS::is_wildcard(cmp.c_str()) && ACE::wild_match(target.c_str(), cmp.c_str(), true, true))
    || target == cmp;
}

QosProfile::DataReaderQosInfo QosProfile::get_datareader_qos(const std::string& topic_name) const {
  if (!topic_name.empty()) {
    for (auto it = datareader_qos_vec_.begin(); it != datareader_qos_vec_.end(); ++it) {
      if (matchTopicName(topic_name, it->first)) {
        return std::make_pair(it->second.first, it->second.second);
      }
    }
  }
  return std::make_pair(default_datareader_qos_, default_datareader_qos_mask_);
}

void QosProfile::set_datawriter_qos(const QosProfile::DataWriterQosInfo& info, const std::string& topic_filter) {
  if (!topic_filter.empty()) {
    datawriter_qos_vec_.push_back(std::make_pair(topic_filter, info));
  } else {
    default_datawriter_qos_ = info.first;
    default_datawriter_qos_mask_ = info.second;
  }
}

QosProfile::DataWriterQosInfo QosProfile::get_datawriter_qos(const std::string& topic_name) const {
  if (!topic_name.empty()) {
    for (auto it = datawriter_qos_vec_.begin(); it != datawriter_qos_vec_.end(); ++it) {
      if (matchTopicName(topic_name, it->first)) {
        return std::make_pair(it->second.first, it->second.second);
      }
    }
  }
  return std::make_pair(default_datawriter_qos_, default_datawriter_qos_mask_);
}

void QosProfile::set_topic_qos(const QosProfile::TopicQosInfo& info, const std::string& topic_filter) {
  if (!topic_filter.empty()) {
    topic_qos_vec_.push_back(std::make_pair(topic_filter, info));
  } else {
    default_topic_qos_ = info.first;
    default_topic_qos_mask_ = info.second;
  }
}

QosProfile::TopicQosInfo QosProfile::get_topic_qos(const std::string& topic_name) const {
  if (!topic_name.empty()) {
    for (auto it = topic_qos_vec_.begin(); it != topic_qos_vec_.end(); ++it) {
      if (matchTopicName(topic_name, it->first)) {
        return std::make_pair(it->second.first, it->second.second);
      }
    }
  }
  return std::make_pair(default_topic_qos_, default_topic_qos_mask_);
}

QosProfile::SubscriberQosInfo QosProfile::get_subscriber_qos() const {
  return std::make_pair(default_subscriber_qos_, default_subscriber_qos_mask_);
}

QosProfile::PublisherQosInfo QosProfile::get_publisher_qos() const {
  return std::make_pair(default_publisher_qos_, default_publisher_qos_mask_);
}

QosProfile::DomainParticipantQosInfo QosProfile::get_participant_qos() const {
  return std::make_pair(default_participant_qos_, default_participant_qos_mask_);
}

// QosLibrary

class QosLibrary {
public:
  QosLibrary();

  const std::string& get_name() { return name_; }

  bool set_profile(const std::string& name, const QosProfile& profile);
  const QosProfile& get_profile(const std::string& name) const;

protected:
  typedef std::map<std::string, QosProfile> ProfileMap;

  std::string name_;
  QosProfile default_profile_;
  ProfileMap profile_map_;
};

QosLibrary::QosLibrary() {
}

bool QosLibrary::set_profile(const std::string& name, const QosProfile& profile) {
  auto it = profile_map_.find(name);
  if (it != profile_map_.end()) {
    return false;
  }
  profile_map_[name] = profile;
  return true;
}

const QosProfile& QosLibrary::get_profile(const std::string& name) const {
  auto it = profile_map_.find(name);
  if (it != profile_map_.end()) {
    return it->second;
  } else {
    return default_profile_;
  }
}

namespace {
static Builder::PropertySeq properties_;
}

void set_global_properties(Builder::PropertySeq& properties) {
  properties_ = properties;
}

const Builder::PropertySeq& get_global_properties() {
  return properties_;
}
