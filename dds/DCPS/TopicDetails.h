/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DDS_DCPS_TOPICDETAILS_H
#define OPENDDS_DDS_DCPS_TOPICDETAILS_H

#include "dds/DCPS/TopicCallbacks.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/debug.h"
#include "dds/DCPS/Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace DCPS {

    struct TopicDetails {

      struct RemoteTopic {
        RemoteTopic() : data_type_name_(), inconsistent_(false), endpoints_() {}
        OPENDDS_STRING data_type_name_;
        bool inconsistent_;
        RepoIdSet endpoints_;
      };
      typedef OPENDDS_MAP_CMP(DCPS::RepoId, RemoteTopic, DCPS::GUID_tKeyLessThan) RemoteTopicMap;

      TopicDetails()
        : topic_id_()
        , has_dcps_key_(false)
        , topic_callbacks_(0)
        , inconsistent_topic_count_(0)
      {}

      void init(const OPENDDS_STRING& name,
                const DCPS::RepoId& topic_id) {
        name_ = name;
        topic_id_ = topic_id;
      }

      void set_local(const OPENDDS_STRING& data_type_name,
                     const DDS::TopicQos& qos,
                     bool has_dcps_key,
                     TopicCallbacks* topic_callbacks) {
        OPENDDS_ASSERT(topic_callbacks != 0);

        local_data_type_name_ = data_type_name;
        local_qos_ = qos;
        has_dcps_key_ = has_dcps_key;
        topic_callbacks_ = topic_callbacks;

        endpoints_.clear();
        inconsistent_topic_count_ = 0;

        for (RemoteTopicMap::iterator pos = remote_topics_.begin(), limit = remote_topics_.end();
             pos != limit; ++pos) {
          RemoteTopic& remote = pos->second;
          remote.inconsistent_ = !remote.data_type_name_.empty() && local_data_type_name_ != remote.data_type_name_;
          if (!remote.inconsistent_) {
            remote.data_type_name_.clear();
          }
          if (remote.inconsistent_) {
            ++inconsistent_topic_count_;
            if (DCPS::DCPS_debug_level) {
              ACE_DEBUG((LM_WARNING,
                         ACE_TEXT("(%P|%t) TopicDetails::set_local - WARNING ")
                         ACE_TEXT("topic %C with data type %C doesn't match discovered data type %C\n"),
                         name_.c_str(),
                         local_data_type_name_.c_str(),
                         remote.data_type_name_.c_str()));
            }
          } else {
            endpoints_.insert(remote.endpoints_.begin(), remote.endpoints_.end());
          }
        }

        if (inconsistent_topic_count_ != 0) {
          topic_callbacks_->inconsistent_topic(inconsistent_topic_count_);
        }
      }

      void unset_local() {
        topic_callbacks_ = 0;

        for (RemoteTopicMap::iterator pos = remote_topics_.begin(), limit = remote_topics_.end();
             pos != limit; ++pos) {
          RemoteTopic& remote = pos->second;
          if (remote.data_type_name_.empty()) {
            remote.data_type_name_ = local_data_type_name_;
          }
        }
      }

      void update(const DDS::TopicQos& qos) {
        local_qos_ = qos;
      }

      // Local
      void add_pub_sub(const DCPS::RepoId& guid) {
        endpoints_.insert(guid);
      }

      // Remote
      void add_pub_sub(const DCPS::RepoId& guid,
                       const OPENDDS_STRING& type_name) {
        // This function can be called before the local side of the
        // topic is initialized.  If this happens, the topic will
        // always be inconsistent meaning the inconsistent count
        // will be incremented and the guid will not be in the
        // endpoints.

        RepoId participant_id = guid;
        participant_id.entityId = ENTITYID_PARTICIPANT;

        endpoints_.insert(guid);

        RemoteTopicMap::iterator remote_topic_iter = remote_topics_.find(participant_id);
        bool inconsistent_before;
        if (remote_topic_iter == remote_topics_.end()) {
          // Insert.
          remote_topic_iter = remote_topics_.insert(std::make_pair(participant_id, RemoteTopic())).first;
          inconsistent_before = false;
        } else {
          inconsistent_before = remote_topic_iter->second.inconsistent_;
        }

        // Initialize.
        RemoteTopic& remote = remote_topic_iter->second;
        remote.data_type_name_ = type_name;
        remote.inconsistent_ = topic_callbacks_ && local_data_type_name_ != remote.data_type_name_;
        if (topic_callbacks_ && !remote.inconsistent_) {
          remote.data_type_name_.clear();
        }
        remote.endpoints_.insert(guid);

        if (!inconsistent_before && remote.inconsistent_) {
          ++inconsistent_topic_count_;
          if (DCPS::DCPS_debug_level) {
            ACE_DEBUG((LM_WARNING,
                       ACE_TEXT("(%P|%t) TopicDetails::add_pub_sub - WARNING ")
                       ACE_TEXT("topic %C with data type %C now does not match discovered data type %C\n"),
                       name_.c_str(),
                       local_data_type_name_.c_str(),
                       remote.data_type_name_.c_str()));
          }
          if (topic_callbacks_) {
            topic_callbacks_->inconsistent_topic(inconsistent_topic_count_);
          }
        } else if (inconsistent_before && !remote.inconsistent_) {
          --inconsistent_topic_count_;
          if (DCPS::DCPS_debug_level) {
            ACE_DEBUG((LM_WARNING,
                       ACE_TEXT("(%P|%t) TopicDetails::add_pub_sub - WARNING ")
                       ACE_TEXT("topic %C with data type %C now matches discovered data type %C\n"),
                       name_.c_str(),
                       local_data_type_name_.c_str(),
                       remote.data_type_name_.c_str()));
          }
          if (topic_callbacks_) {
            topic_callbacks_->inconsistent_topic(inconsistent_topic_count_);
          }
        }

        if (remote.inconsistent_) {
          endpoints_.erase(guid);
        }
      }

      // Local and remote
      void remove_pub_sub(const DCPS::RepoId& guid) {
        endpoints_.erase(guid);

        RepoId participant_id = guid;
        participant_id.entityId = ENTITYID_PARTICIPANT;

        RemoteTopicMap::iterator remote_topic_iter = remote_topics_.find(participant_id);

        if (remote_topic_iter == remote_topics_.end()) {
          // It was local.
          return;
        }

        RemoteTopic& remote = remote_topic_iter->second;
        remote.endpoints_.erase(guid);

        if (remote.inconsistent_) {
          --inconsistent_topic_count_;
          if (topic_callbacks_) {
            topic_callbacks_->inconsistent_topic(inconsistent_topic_count_);
          }
        }

        if (remote.endpoints_.empty()) {
          remote_topics_.erase(remote_topic_iter);
        }
      }

      const OPENDDS_STRING local_data_type_name() const { return local_data_type_name_; }
      const DDS::TopicQos local_qos() const { return local_qos_; }
      const DCPS::RepoId& topic_id() const { return topic_id_; }
      bool has_dcps_key() const { return has_dcps_key_; }
      bool local_is_set() const { return topic_callbacks_; }
      const RepoIdSet& endpoints() const { return endpoints_; }

      bool is_dead() const {
        return topic_callbacks_ == 0 && remote_topics_.empty();
      }

    private:
      OPENDDS_STRING name_;
      OPENDDS_STRING local_data_type_name_;
      DDS::TopicQos local_qos_;
      DCPS::RepoId topic_id_;
      bool has_dcps_key_;
      TopicCallbacks* topic_callbacks_;
      RepoIdSet endpoints_;

      RemoteTopicMap remote_topics_;
      int inconsistent_topic_count_;
    };

  } // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TOPICDETAILS_H  */
