/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TOPICDETAILS_H
#define OPENDDS_DCPS_TOPICDETAILS_H

#include "TopicCallbacks.h"
#include "GuidUtils.h"
#include "debug.h"
#include "Definitions.h"
#include "XTypes/TypeObject.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace DCPS {

    struct TopicDetails {

      TopicDetails()
        : topic_id_()
        , has_dcps_key_(false)
        , topic_callbacks_(0)
        , inconsistent_topic_count_(0)
        , assertion_count_(0)
      {}

      void init(const OPENDDS_STRING& name,
                const DCPS::RepoId& topic_id) {
        name_ = name;
        topic_id_ = topic_id;
      }

      void set_local(const OPENDDS_STRING& data_type_name,
                     const DDS::TopicQos& qos,
                     bool has_dcps_key,
                     TopicCallbacks* topic_callbacks)
      {
        OPENDDS_ASSERT(topic_callbacks != 0);

        local_data_type_name_ = data_type_name;
        local_qos_ = qos;
        has_dcps_key_ = has_dcps_key;
        topic_callbacks_ = topic_callbacks;
        ++assertion_count_;
      }

      void unset_local()
      {
        if (--assertion_count_ == 0) {
          OPENDDS_ASSERT(local_publications_.empty());
          OPENDDS_ASSERT(local_subscriptions_.empty());
          topic_callbacks_ = 0;
        }
      }

      void update(const DDS::TopicQos& qos)
      {
        local_qos_ = qos;
      }

      void add_local_publication(const DCPS::RepoId& guid)
      {
        local_publications_.insert(guid);
      }

      void remove_local_publication(const DCPS::RepoId& guid)
      {
        local_publications_.erase(guid);
      }

      const RepoIdSet& local_publications() const
      {
        return local_publications_;
      }

      void add_local_subscription(const DCPS::RepoId& guid)
      {
        local_subscriptions_.insert(guid);
      }

      void remove_local_subscription(const DCPS::RepoId& guid)
      {
        local_subscriptions_.erase(guid);
      }

      const RepoIdSet& local_subscriptions() const
      {
        return local_subscriptions_;
      }

      void add_discovered_publication(const DCPS::RepoId& guid)
      {
        discovered_publications_.insert(guid);
      }

      void remove_discovered_publication(const DCPS::RepoId& guid)
      {
        discovered_publications_.erase(guid);
      }

      const RepoIdSet& discovered_publications() const
      {
        return discovered_publications_;
      }

      void add_discovered_subscription(const DCPS::RepoId& guid)
      {
        discovered_subscriptions_.insert(guid);
      }

      void remove_discovered_subscription(const DCPS::RepoId& guid)
      {
        discovered_subscriptions_.erase(guid);
      }

      const RepoIdSet& discovered_subscriptions() const
      {
        return discovered_subscriptions_;
      }

      void increment_inconsistent()
      {
        ++inconsistent_topic_count_;
        topic_callbacks_->inconsistent_topic(inconsistent_topic_count_);
      }

      const OPENDDS_STRING local_data_type_name() const { return local_data_type_name_; }
      const DDS::TopicQos local_qos() const { return local_qos_; }
      const DCPS::RepoId& topic_id() const { return topic_id_; }
      bool has_dcps_key() const { return has_dcps_key_; }
      bool local_is_set() const { return topic_callbacks_; }

      bool is_dead() const
      {
        return topic_callbacks_ == 0 &&
          local_publications_.empty() &&
          local_subscriptions_.empty() &&
          discovered_publications_.empty() &&
          discovered_subscriptions_.empty();
      }

    private:
      OPENDDS_STRING name_;
      OPENDDS_STRING local_data_type_name_;
      DDS::TopicQos local_qos_;
      DCPS::RepoId topic_id_;
      bool has_dcps_key_;
      TopicCallbacks* topic_callbacks_;

      RepoIdSet local_publications_;
      RepoIdSet local_subscriptions_;
      RepoIdSet discovered_publications_;
      RepoIdSet discovered_subscriptions_;
      int inconsistent_topic_count_;
      int assertion_count_;
    };

  } // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TOPICDETAILS_H  */
