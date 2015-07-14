#ifndef DDS_HAS_MINIMUM_BIT

#include "StaticDiscovery.h"
#include "dds/DCPS/debug.h"
#include "dds/DCPS/ConfigUtils.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

namespace OpenDDS {
  namespace DCPS {

    void
    StaticEndpointManager::assign_publication_key(DCPS::RepoId& rid,
                                                  const RepoId& /*topicId*/,
                                                  const DDS::DataWriterQos& qos)
    {
      if (qos.user_data.value.length() != 3) {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_publication_key: no user data to identify writer\n")));
        return;
      }

      rid.entityId.entityKey[0] = qos.user_data.value[0];
      rid.entityId.entityKey[1] = qos.user_data.value[1];
      rid.entityId.entityKey[2] = qos.user_data.value[2];
      rid.entityId.entityKind = DCPS::ENTITYKIND_USER_WRITER_WITH_KEY;

      EndpointRegistry::WriterMapType::const_iterator pos = registry_.writer_map.find(rid);
      if (pos == registry_.writer_map.end()) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: StaticEndpointManager::assign_publication_key: unknown writer: %s\n"), LogGuid(rid).c_str()));
        return;
      }

      DDS::DataWriterQos qos2(qos);
      // Qos in registry will not have the user data so overwrite.
      qos2.user_data = pos->second.qos.user_data;

      DDS::DataWriterQos qos3(pos->second.qos);

      if (qos2 != qos3) {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_publication_key: dynamic and static QoS differ\n")));
      }
    }

    void
    StaticEndpointManager::assign_subscription_key(DCPS::RepoId& rid,
                                                   const RepoId& /*topicId*/,
                                                   const DDS::DataReaderQos& qos)
    {
      if (qos.user_data.value.length() != 3) {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_subscription_key: no user data to identify reader\n")));
        return;
      }

      rid.entityId.entityKey[0] = qos.user_data.value[0];
      rid.entityId.entityKey[1] = qos.user_data.value[1];
      rid.entityId.entityKey[2] = qos.user_data.value[2];
      rid.entityId.entityKind = DCPS::ENTITYKIND_USER_READER_WITH_KEY;

      EndpointRegistry::ReaderMapType::const_iterator pos = registry_.reader_map.find(rid);
      if (pos == registry_.reader_map.end()) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_subscription_key: unknown reader: %s\n"), LogGuid(rid).c_str()));
        return;
      }

      DDS::DataReaderQos qos2(qos);
      // Qos in registry will not have the user data so overwrite.
      qos2.user_data = pos->second.qos.user_data;

      if (qos2 != pos->second.qos) {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_subscription_key: dynamic and static QoS differ\n")));
      }
    }

    bool
    StaticEndpointManager::update_topic_qos(const DCPS::RepoId& /*topicId*/,
                                            const DDS::TopicQos& /*qos*/,
                                            OPENDDS_STRING& /*name*/)
    {
      ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) StaticEndpointManager::update_topic_qos - ")
                 ACE_TEXT("Not allowed\n")));
      return false;
    }

    bool
    StaticEndpointManager::update_publication_qos(const DCPS::RepoId& /*publicationId*/,
                                                  const DDS::DataWriterQos& /*qos*/,
                                                  const DDS::PublisherQos& /*publisherQos*/)
    {
      ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) StaticEndpointManager::update_publication_qos - ")
                 ACE_TEXT("Not allowed\n")));
      return false;
    }

    bool
    StaticEndpointManager::update_subscription_qos(const DCPS::RepoId& /*subscriptionId*/,
                                                   const DDS::DataReaderQos& /*qos*/,
                                                   const DDS::SubscriberQos& /*subscriberQos*/)
    {
      ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) StaticEndpointManager::update_subscription_qos - ")
                 ACE_TEXT("Not allowed\n")));
      return false;
    }

    bool
    StaticEndpointManager::update_subscription_params(const DCPS::RepoId& /*subId*/,
                                                      const DDS::StringSeq& /*params*/)
    {
      ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) StaticEndpointManager::update_subscription_qos - ")
                 ACE_TEXT("Not allowed\n")));
      return false;
    }

    void
    StaticEndpointManager::association_complete(const DCPS::RepoId& /*localId*/,
                                                const DCPS::RepoId& /*remoteId*/)
    {
      // Do nothing.
    }

    bool
    StaticEndpointManager::disassociate(const StaticDiscoveredParticipantData& /*pdata*/)
    {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::disassociate TODO\n")));
      // TODO
      return false;
    }

    DDS::ReturnCode_t
    StaticEndpointManager::write_publication_data(const DCPS::RepoId& writerid,
                                                  LocalPublication& pub,
                                                  const DCPS::RepoId& /*reader*/)
    {
      /*
        Find all matching remote readers.
        If the reader is best effort, then associate immediately.
        If the reader is reliable (we are reliable by implication), register with the transport to receive notification that the remote reader is up.
       */

      TopicNameMap::const_iterator topic_pos = topic_names_.find(pub.topic_id_);
      if (topic_pos == topic_names_.end()) {
        return DDS::RETCODE_ERROR;
      }

      const OPENDDS_STRING topic_name = topic_pos->second;

      for (EndpointRegistry::ReaderMapType::const_iterator pos = registry_.reader_map.begin(), limit = registry_.reader_map.end();
           pos != limit;
           ++pos) {
        const RepoId& readerid = pos->first;

        // pos represents a remote reader.  Try to match.
        if (!GuidPrefixEqual()(readerid.guidPrefix, writerid.guidPrefix) &&
            pos->second.topic_name == topic_name) {
          // Different participants, same topic.
          DCPS::IncompatibleQosStatus writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
          DCPS::IncompatibleQosStatus readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};

          if (DCPS::compatibleQOS(&writerStatus, &readerStatus, pub.trans_info_, pos->second.trans_info, &pub.qos_, &pos->second.qos, &pub.publisher_qos_, &pos->second.subscriber_qos)) {
            switch (pos->second.qos.reliability.kind) {
            case DDS::BEST_EFFORT_RELIABILITY_QOS:
              {
                const DCPS::ReaderAssociation ra =
                  {pos->second.trans_info, readerid, pos->second.subscriber_qos, pos->second.qos,
                   "", "",
                   0};
                pub.publication_->add_association(readerid, ra, true);
                pub.publication_->association_complete(readerid);
              }
              break;
            case DDS::RELIABLE_RELIABILITY_QOS:
              pub.publication_->register_for_reader(participant_id_, writerid, readerid, pos->second.trans_info, this);
              break;
            }

          }
        }
      }

      return DDS::RETCODE_OK;
    }

    DDS::ReturnCode_t
    StaticEndpointManager::remove_publication_i(const RepoId& /*publicationId*/)
    {
      return DDS::RETCODE_OK;
    }

    DDS::ReturnCode_t
    StaticEndpointManager::write_subscription_data(const DCPS::RepoId& readerid,
                                                   LocalSubscription& sub,
                                                   const DCPS::RepoId& /*reader*/)
    {
      /*
        Find all matching remote writers.
        If we (the reader) is best effort, then associate immediately.
        If we (the reader) are reliable, then register with the transport to receive notification that the remote writer is up.
       */

      TopicNameMap::const_iterator topic_pos = topic_names_.find(sub.topic_id_);
      if (topic_pos == topic_names_.end()) {
        return DDS::RETCODE_ERROR;
      }

      const OPENDDS_STRING topic_name = topic_pos->second;

      for (EndpointRegistry::WriterMapType::const_iterator pos = registry_.writer_map.begin(), limit = registry_.writer_map.end();
           pos != limit;
           ++pos) {
        const RepoId& writerid = pos->first;

        // pos represents a remote writer.  Try to match.
        if (!GuidPrefixEqual()(writerid.guidPrefix, readerid.guidPrefix) &&
            pos->second.topic_name == topic_name) {
          // Different participants, same topic.
          DCPS::IncompatibleQosStatus writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
          DCPS::IncompatibleQosStatus readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};

          if (DCPS::compatibleQOS(&writerStatus, &readerStatus, pos->second.trans_info, sub.trans_info_, &pos->second.qos, &sub.qos_, &pos->second.publisher_qos, &sub.subscriber_qos_)) {
            switch (sub.qos_.reliability.kind) {
            case DDS::BEST_EFFORT_RELIABILITY_QOS:
              {
                const DCPS::WriterAssociation wa =
                  {pos->second.trans_info, writerid, pos->second.publisher_qos, pos->second.qos};

                sub.subscription_->add_association(writerid, wa, false);
              }
              break;
            case DDS::RELIABLE_RELIABILITY_QOS:
              sub.subscription_->register_for_writer(participant_id_, readerid, writerid, pos->second.trans_info, this);
              break;
            }
          }
        }
      }

      return DDS::RETCODE_OK;
    }

    DDS::ReturnCode_t
    StaticEndpointManager::remove_subscription_i(const RepoId& /*subscriptionId*/)
    {
      return DDS::RETCODE_OK;
    }

    bool
    StaticEndpointManager::shutting_down() const
    {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::shutting_down TODO\n")));
      // TODO
      return false;
    }

    void
    StaticEndpointManager::populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& /*tls*/,
                                                               DiscoveredSubscriptionIter& /*iter*/,
                                                               const RepoId& /*reader*/)
    {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::populate_transport_locator_sequence TODO\n")));
      // TODO
    }

    void
    StaticEndpointManager::populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& /*tls*/,
                                                               DiscoveredPublicationIter& /*iter*/,
                                                               const RepoId& /*reader*/)
    {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::populate_transport_locator_sequence TODO\n")));
      // TODO
    }

    bool
    StaticEndpointManager::defer_writer(const RepoId& /*writer*/,
                                        const RepoId& /*writer_participant*/)
    {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::defer_writer TODO\n")));
      // TODO
      return false;
    }

    bool
    StaticEndpointManager::defer_reader(const RepoId& /*writer*/,
                                        const RepoId& /*writer_participant*/)
    {
      // TODO
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::defer_reader TODO\n")));
      return false;
    }

    void
    StaticEndpointManager::reader_exists(const RepoId& readerid, const RepoId& writerid)
    {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      LocalPublicationMap::const_iterator lp_pos = local_publications_.find(writerid);
      EndpointRegistry::ReaderMapType::const_iterator reader_pos = registry_.reader_map.find(readerid);
      if (lp_pos != local_publications_.end() &&
          reader_pos != registry_.reader_map.end()) {
        DCPS::DataWriterCallbacks* dwr = lp_pos->second.publication_;
        const DCPS::ReaderAssociation ra =
          {reader_pos->second.trans_info, readerid, reader_pos->second.subscriber_qos, reader_pos->second.qos,
           "", "",
           0};

        dwr->add_association(readerid, ra, true);
        dwr->association_complete(readerid);
      }
    }

    void
    StaticEndpointManager::writer_exists(const RepoId& writerid, const RepoId& readerid)
    {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      LocalSubscriptionMap::const_iterator ls_pos = local_subscriptions_.find(readerid);
      EndpointRegistry::WriterMapType::const_iterator writer_pos = registry_.writer_map.find(writerid);
      if (ls_pos != local_subscriptions_.end() &&
          writer_pos != registry_.writer_map.end()) {
        DCPS::DataReaderCallbacks* drr = ls_pos->second.subscription_;
        const DCPS::WriterAssociation wa =
          {writer_pos->second.trans_info, writerid, writer_pos->second.publisher_qos, writer_pos->second.qos};

        drr->add_association(writerid, wa, false);
      }
    }

    StaticDiscovery::StaticDiscovery(const RepoKey& key)
      : PeerDiscovery<StaticParticipant>(key)
    { }

    namespace {
      unsigned char hextobyte(unsigned char c)
      {
        if (c >= '0' && c <= '9') {
          return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
          return c - 'a';
        }
        if (c >= 'A' && c <= 'F') {
          return c - 'A';
        }
        return c;
      }

      unsigned char
      fromhex(const std::string& x,
              size_t idx)
      {
        return (hextobyte(x[idx * 2]) << 4) | (hextobyte(x[idx * 2 + 1]));
      }

      EntityId_t
      build_id(const unsigned char* entity_key,
               const unsigned char entity_kind)
      {
        EntityId_t retval;
        retval.entityKey[0] = entity_key[0];
        retval.entityKey[1] = entity_key[1];
        retval.entityKey[2] = entity_key[2];
        retval.entityKind = entity_kind;
        return retval;
      }

      RepoId
      build_id(DDS::DomainId_t domain,
               const unsigned char* participant_id,
               const EntityId_t& entity_id)
      {
        RepoId id;
        id.guidPrefix[0] = DCPS::VENDORID_OCI[0];
        id.guidPrefix[1] = DCPS::VENDORID_OCI[1];
        // id.guidPrefix[2] = domain[0]
        // id.guidPrefix[3] = domain[1]
        // id.guidPrefix[4] = domain[2]
        // id.guidPrefix[5] = domain[3]
        DDS::DomainId_t netdom = ACE_HTONL(domain);
        ACE_OS::memcpy(&id.guidPrefix[2], &netdom, sizeof (DDS::DomainId_t));
        // id.guidPrefix[6] = participant[0]
        // id.guidPrefix[7] = participant[1]
        // id.guidPrefix[8] = participant[2]
        // id.guidPrefix[9] = participant[3]
        // id.guidPrefix[10] = participant[4]
        // id.guidPrefix[11] = participant[5]
        ACE_OS::memcpy(&id.guidPrefix[6], participant_id, 6);
        id.entityId = entity_id;
        return id;
      }
    }

    AddDomainStatus
    StaticDiscovery::add_domain_participant(DDS::DomainId_t domain,
                                            const DDS::DomainParticipantQos& qos)
    {
      DCPS::AddDomainStatus ads = {RepoId(), false /*federated*/};

      if (qos.user_data.value.length() != 6) {
        ACE_DEBUG((LM_ERROR,
                   ACE_TEXT("(%P|%t) StaticDiscovery::add_domain_participant ")
                   ACE_TEXT("No userdata to identify participant\n")));
        return ads;
      }

      RepoId id = build_id(domain,
                                          qos.user_data.value.get_buffer(),
                                          ENTITYID_PARTICIPANT);
      if (!get_part(domain, id).is_nil()) {
        ACE_DEBUG((LM_ERROR,
                   ACE_TEXT("(%P|%t) StaticDiscovery::add_domain_participant ")
                   ACE_TEXT("Duplicate participant\n")));
        return ads;
      }

      const DCPS::RcHandle<StaticParticipant> participant = new StaticParticipant(id, qos, registry_);

      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ads);
        participants_[domain][id] = participant;
      }

      ads.id = id;

      return ads;
    }

    namespace {
      const ACE_TCHAR TOPIC_SECTION_NAME[] = ACE_TEXT("topic");
      const ACE_TCHAR DATAWRITERQOS_SECTION_NAME[] = ACE_TEXT("datawriterqos");
      const ACE_TCHAR DATAREADERQOS_SECTION_NAME[] = ACE_TEXT("datareaderqos");
      const ACE_TCHAR PUBLISHERQOS_SECTION_NAME[]  = ACE_TEXT("publisherqos");
      const ACE_TCHAR SUBSCRIBERQOS_SECTION_NAME[] = ACE_TEXT("subscriberqos");
      const ACE_TCHAR ENDPOINT_SECTION_NAME[] = ACE_TEXT("endpoint");

      void parse_second(CORBA::Long& x, const std::string& value)
      {
        if (value == "DURATION_INFINITE_SEC") {
          x = DDS::DURATION_INFINITE_SEC;
        } else {
          x = atoi(value.c_str());
        }
      }

      void parse_nanosecond(CORBA::ULong& x, const std::string& value)
      {
        if (value == "DURATION_INFINITE_NANOSEC") {
          x = DDS::DURATION_INFINITE_NSEC;
        } else {
          x = atoi(value.c_str());
        }
      }

      bool parse_bool(CORBA::Boolean& x, const std::string& value)
      {
        if (value == "true") {
          x = true;
          return true;
        } else if (value == "false") {
          x = false;
          return true;
        }
        return false;
      }

      void parse_list(DDS::PartitionQosPolicy& x, const std::string& value)
      {
        // Value can be a comma-separated list
        const char* start = value.c_str();
        char buffer[128];
        std::memset(buffer, 0, sizeof(buffer));
        while (const char* next_comma = std::strchr(start, ',')) {
          // Copy into temp buffer, won't have null
          std::strncpy(buffer, start, next_comma - start);
          // Append null
          buffer[next_comma - start] = '\0';
          // Add to QOS
          x.name.length(x.name.length() + 1);
          x.name[x.name.length() - 1] = static_cast<const char*>(buffer);
          // Advance pointer
          start = next_comma + 1;
        }
        // Append everything after last comma
        x.name.length(x.name.length() + 1);
        x.name[x.name.length() - 1] = start;
      }
    }

    int
    StaticDiscovery::load_configuration(ACE_Configuration_Heap& cf)
    {
      if (parse_topics(cf) ||
          parse_datawriterqos(cf) ||
          parse_datareaderqos(cf) ||
          parse_publisherqos(cf) ||
          parse_subscriberqos(cf) ||
          parse_endpoints(cf)) {
        return -1;
      }
      return 0;
    }

    int
    StaticDiscovery::parse_topics(ACE_Configuration_Heap& cf)
    {
      const ACE_Configuration_Section_Key& root = cf.root_section();
      ACE_Configuration_Section_Key section;

      if (cf.open_section(root, TOPIC_SECTION_NAME, 0, section) != 0) {
        if (DCPS_debug_level > 0) {
          // This is not an error if the configuration file does not have
          // any topic (sub)section.
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_topics ")
                     ACE_TEXT("no [%s] sections.\n"),
                     TOPIC_SECTION_NAME));
        }
        return 0;
      }

      // Ensure there are no key/values in the [topic] section.
      // Every key/value must be in a [topic/*] sub-section.
      ValueMap vm;
      if (pullValues(cf, section, vm) > 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                          ACE_TEXT("[topic] sections must have a subsection name\n")),
                         -1);
      }
      // Process the subsections of this section
      KeyList keys;
      if (processSections(cf, section, keys) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                          ACE_TEXT("too many nesting layers in the [topic] section.\n")),
                         -1);
      }

      // Loop through the [topic/*] sections
      for (KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
        std::string topic_name = (*it).first;

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                     ACE_TEXT("processing [topic/%C] section.\n"),
                     topic_name.c_str()));
        }

        ValueMap values;
        pullValues( cf, (*it).second, values );

        EndpointRegistry::Topic topic;
        bool name_Specified = false,
          type_name_Specified = false,
          max_message_size_Specified = false;

        for (ValueMap::const_iterator it=values.begin(); it != values.end(); ++it) {
          std::string name = (*it).first;
          std::string value = (*it).second;

          if (name == "name") {
            topic.name = value;
            name_Specified = true;
          } else if (name == "type_name") {
            if (value.size() >= 128) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                                ACE_TEXT("type_name (%C) must be less than 128 characters in [topic/%C] section.\n"),
                                value.c_str(), topic_name.c_str()),
                               -1);
            }
            topic.type_name = value;
            type_name_Specified = true;
          } else if (name == "max_message_size") {
            if (convertToInteger(value, topic.max_message_size)) {
              max_message_size_Specified = true;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                                ACE_TEXT("Illegal integer value for max_message_size (%C) in [topic/%C] section.\n"),
                                value.c_str(), topic_name.c_str()),
                               -1);
            }
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                              ACE_TEXT("Unexpected entry (%C) in [topic/%C] section.\n"),
                              name.c_str(), topic_name.c_str()),
                             -1);
          }
        }

        if (!name_Specified) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                            ACE_TEXT("No type_name specified for [topic/%C] section.\n"),
                            topic_name.c_str()),
                           -1);
        }

        if (!type_name_Specified) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                            ACE_TEXT("No type_name specified for [topic/%C] section.\n"),
                            topic_name.c_str()),
                           -1);
        }

        if (!max_message_size_Specified) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                            ACE_TEXT("No max_message_size specified for [topic/%C] section.\n"),
                            topic_name.c_str()),
                           -1);
        }

        registry_.topic_map.insert(std::make_pair(topic_name, topic));
      }

      return 0;
    }

    int
    StaticDiscovery::parse_datawriterqos(ACE_Configuration_Heap& cf)
    {
      const ACE_Configuration_Section_Key& root = cf.root_section();
      ACE_Configuration_Section_Key section;

      if (cf.open_section(root, DATAWRITERQOS_SECTION_NAME, 0, section) != 0) {
        if (DCPS_debug_level > 0) {
          // This is not an error if the configuration file does not have
          // any datawriterqos (sub)section.
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_datawriterqos ")
                     ACE_TEXT("no [%s] sections.\n"),
                     DATAWRITERQOS_SECTION_NAME));
        }
        return 0;
      }

      // Ensure there are no key/values in the [datawriterqos] section.
      // Every key/value must be in a [datawriterqos/*] sub-section.
      ValueMap vm;
      if (pullValues(cf, section, vm) > 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                          ACE_TEXT("[datawriterqos] sections must have a subsection name\n")),
                         -1);
      }
      // Process the subsections of this section
      KeyList keys;
      if (processSections(cf, section, keys) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                          ACE_TEXT("too many nesting layers in the [datawriterqos] section.\n")),
                         -1);
      }

      // Loop through the [datawriterqos/*] sections
      for (KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
        std::string datawriterqos_name = (*it).first;

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                     ACE_TEXT("processing [datawriterqos/%C] section.\n"),
                     datawriterqos_name.c_str()));
        }

        ValueMap values;
        pullValues( cf, (*it).second, values );

        DDS::DataWriterQos datawriterqos(TheServiceParticipant->initial_DataWriterQos());

        for (ValueMap::const_iterator it=values.begin(); it != values.end(); ++it) {
          std::string name = (*it).first;
          std::string value = (*it).second;

          if (name == "durability.kind") {
            if (value == "VOLATILE") {
              datawriterqos.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
            } else if (value == "TRANSIENT_LOCAL") {
              datawriterqos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
            } else if (value == "TRANSIENT") {
              datawriterqos.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
            } else if (value == "PERSISTENT") {
              datawriterqos.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
#endif
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                                ACE_TEXT("Illegal value for durability.kind (%C) in [datawriterqos/%C] section.\n"),
                                value.c_str(), datawriterqos_name.c_str()),
                               -1);
            }
          } else if (name == "deadline.period.sec") {
            parse_second(datawriterqos.deadline.period.sec, value);
          } else if (name == "deadline.period.nanosec") {
            parse_nanosecond(datawriterqos.deadline.period.nanosec, value);
          } else if (name == "latency_budget.duration.sec") {
            parse_second(datawriterqos.latency_budget.duration.sec, value);
          } else if (name == "latency_budget.duration.nanosec") {
            parse_nanosecond(datawriterqos.latency_budget.duration.nanosec, value);
          } else if (name == "liveliness.kind") {
            if (value == "AUTOMATIC") {
              datawriterqos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
            } else if (value == "MANUAL_BY_TOPIC") {
              datawriterqos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
            } else if (value == "MANUAL_BY_PARTICIPANT") {
              datawriterqos.liveliness.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                                ACE_TEXT("Illegal value for liveliness.kind (%C) in [datawriterqos/%C] section.\n"),
                                value.c_str(), datawriterqos_name.c_str()),
                               -1);
            }
          } else if (name == "liveliness.lease_duration.sec") {
            parse_second(datawriterqos.liveliness.lease_duration.sec, value);
          } else if (name == "liveliness.lease_duration.nanosec") {
            parse_nanosecond(datawriterqos.liveliness.lease_duration.nanosec, value);
          } else if (name == "reliability.kind") {
            if (value == "BEST_EFFORT") {
              datawriterqos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
            } else if (value == "RELIABLE") {
              datawriterqos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                                ACE_TEXT("Illegal value for reliability.kind (%C) in [datawriterqos/%C] section.\n"),
                                value.c_str(), datawriterqos_name.c_str()),
                               -1);
            }
          } else if (name == "reliability.max_blocking_time.sec") {
            parse_second(datawriterqos.reliability.max_blocking_time.sec, value);
          } else if (name == "reliability.max_blocking_time.nanosec") {
            parse_nanosecond(datawriterqos.reliability.max_blocking_time.nanosec, value);
          } else if (name == "destination_order.kind") {
            if (value == "BY_RECEPTION_TIMESTAMP") {
              datawriterqos.destination_order.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
            } else if (value == "BY_SOURCE_TIMESTAMP") {
              datawriterqos.destination_order.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                                ACE_TEXT("Illegal value for destination_order.kind (%C) in [datawriterqos/%C] section.\n"),
                                value.c_str(), datawriterqos_name.c_str()),
                               -1);
            }
          } else if (name == "history.kind") {
            if (value == "KEEP_ALL") {
              datawriterqos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
            } else if (value == "KEEP_LAST") {
              datawriterqos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                                ACE_TEXT("Illegal value for history.kind (%C) in [datawriterqos/%C] section.\n"),
                                value.c_str(), datawriterqos_name.c_str()),
                               -1);
            }
          } else if (name == "history.depth") {
            datawriterqos.history.depth = atoi(value.c_str());
          } else if (name == "resource_limits.max_samples") {
            datawriterqos.resource_limits.max_samples = atoi(value.c_str());
          } else if (name == "resource_limits.max_instances") {
            datawriterqos.resource_limits.max_instances = atoi(value.c_str());
          } else if (name == "resource_limits.max_samples_per_instance") {
            datawriterqos.resource_limits.max_samples_per_instance = atoi(value.c_str());
          } else if (name == "transport_priority.value") {
            datawriterqos.transport_priority.value = atoi(value.c_str());
          } else if (name == "lifespan.duration.sec") {
            parse_second(datawriterqos.lifespan.duration.sec, value);
          } else if (name == "lifespan.duration.nanosec") {
            parse_nanosecond(datawriterqos.lifespan.duration.nanosec, value);
          } else if (name == "ownership.kind") {
            if (value == "SHARED") {
              datawriterqos.ownership.kind = DDS::SHARED_OWNERSHIP_QOS;
            } else if (value == "EXCLUSIVE") {
              datawriterqos.ownership.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                                ACE_TEXT("Illegal value for ownership.kind (%C) in [datawriterqos/%C] section.\n"),
                                value.c_str(), datawriterqos_name.c_str()),
                               -1);
            }
          } else if (name == "ownership_strength.value") {
            datawriterqos.ownership_strength.value = atoi(value.c_str());
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                              ACE_TEXT("Unexpected entry (%C) in [datawriterqos/%C] section.\n"),
                              name.c_str(), datawriterqos_name.c_str()),
                             -1);
          }
        }

        registry_.datawriterqos_map.insert(std::make_pair(datawriterqos_name, datawriterqos));
      }

      return 0;
    }

    int
    StaticDiscovery::parse_datareaderqos(ACE_Configuration_Heap& cf)
    {
      const ACE_Configuration_Section_Key& root = cf.root_section();
      ACE_Configuration_Section_Key section;

      if (cf.open_section(root, DATAREADERQOS_SECTION_NAME, 0, section) != 0) {
        if (DCPS_debug_level > 0) {
          // This is not an error if the configuration file does not have
          // any datareaderqos (sub)section.
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_datareaderqos ")
                     ACE_TEXT("no [%s] sections.\n"),
                     DATAREADERQOS_SECTION_NAME));
        }
        return 0;
      }

      // Ensure there are no key/values in the [datareaderqos] section.
      // Every key/value must be in a [datareaderqos/*] sub-section.
      ValueMap vm;
      if (pullValues(cf, section, vm) > 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                          ACE_TEXT("[datareaderqos] sections must have a subsection name\n")),
                         -1);
      }
      // Process the subsections of this section
      KeyList keys;
      if (processSections(cf, section, keys) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                          ACE_TEXT("too many nesting layers in the [datareaderqos] section.\n")),
                         -1);
      }

      // Loop through the [datareaderqos/*] sections
      for (KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
        std::string datareaderqos_name = (*it).first;

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                     ACE_TEXT("processing [datareaderqos/%C] section.\n"),
                     datareaderqos_name.c_str()));
        }

        ValueMap values;
        pullValues( cf, (*it).second, values );

        DDS::DataReaderQos datareaderqos(TheServiceParticipant->initial_DataReaderQos());

        for (ValueMap::const_iterator it=values.begin(); it != values.end(); ++it) {
          std::string name = (*it).first;
          std::string value = (*it).second;

          if (name == "durability.kind") {
            if (value == "VOLATILE") {
              datareaderqos.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
            } else if (value == "TRANSIENT_LOCAL") {
              datareaderqos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
            } else if (value == "TRANSIENT") {
              datareaderqos.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
            } else if (value == "PERSISTENT") {
              datareaderqos.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
#endif
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                                ACE_TEXT("Illegal value for durability.kind (%C) in [datareaderqos/%C] section.\n"),
                                value.c_str(), datareaderqos_name.c_str()),
                               -1);
            }
          } else if (name == "deadline.period.sec") {
            parse_second(datareaderqos.deadline.period.sec, value);
          } else if (name == "deadline.period.nanosec") {
            parse_nanosecond(datareaderqos.deadline.period.nanosec, value);
          } else if (name == "latency_budget.duration.sec") {
            parse_second(datareaderqos.latency_budget.duration.sec, value);
          } else if (name == "latency_budget.duration.nanosec") {
            parse_nanosecond(datareaderqos.latency_budget.duration.nanosec, value);
          } else if (name == "liveliness.kind") {
            if (value == "AUTOMATIC") {
              datareaderqos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
            } else if (value == "MANUAL_BY_TOPIC") {
              datareaderqos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
            } else if (value == "MANUAL_BY_PARTICIPANT") {
              datareaderqos.liveliness.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                                ACE_TEXT("Illegal value for liveliness.kind (%C) in [datareaderqos/%C] section.\n"),
                                value.c_str(), datareaderqos_name.c_str()),
                               -1);
            }
          } else if (name == "liveliness.lease_duration.sec") {
            parse_second(datareaderqos.liveliness.lease_duration.sec, value);
          } else if (name == "liveliness.lease_duration.nanosec") {
            parse_nanosecond(datareaderqos.liveliness.lease_duration.nanosec, value);
          } else if (name == "reliability.kind") {
            if (value == "BEST_EFFORT") {
              datareaderqos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
            } else if (value == "RELIABLE") {
              datareaderqos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                                ACE_TEXT("Illegal value for reliability.kind (%C) in [datareaderqos/%C] section.\n"),
                                value.c_str(), datareaderqos_name.c_str()),
                               -1);
            }
          } else if (name == "reliability.max_blocking_time.sec") {
            parse_second(datareaderqos.reliability.max_blocking_time.sec, value);
          } else if (name == "reliability.max_blocking_time.nanosec") {
            parse_nanosecond(datareaderqos.reliability.max_blocking_time.nanosec, value);
          } else if (name == "destination_order.kind") {
            if (value == "BY_RECEPTION_TIMESTAMP") {
              datareaderqos.destination_order.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
            } else if (value == "BY_SOURCE_TIMESTAMP") {
              datareaderqos.destination_order.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                                ACE_TEXT("Illegal value for destination_order.kind (%C) in [datareaderqos/%C] section.\n"),
                                value.c_str(), datareaderqos_name.c_str()),
                               -1);
            }
          } else if (name == "history.kind") {
            if (value == "KEEP_ALL") {
              datareaderqos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
            } else if (value == "KEEP_LAST") {
              datareaderqos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                                ACE_TEXT("Illegal value for history.kind (%C) in [datareaderqos/%C] section.\n"),
                                value.c_str(), datareaderqos_name.c_str()),
                               -1);
            }
          } else if (name == "history.depth") {
            datareaderqos.history.depth = atoi(value.c_str());
          } else if (name == "resource_limits.max_samples") {
            datareaderqos.resource_limits.max_samples = atoi(value.c_str());
          } else if (name == "resource_limits.max_instances") {
            datareaderqos.resource_limits.max_instances = atoi(value.c_str());
          } else if (name == "resource_limits.max_samples_per_instance") {
            datareaderqos.resource_limits.max_samples_per_instance = atoi(value.c_str());
          } else if (name == "time_based_filter.minimum_separation.sec") {
            parse_second(datareaderqos.time_based_filter.minimum_separation.sec, value);
          } else if (name == "time_based_filter.minimum_separation.nanosec") {
            parse_nanosecond(datareaderqos.time_based_filter.minimum_separation.nanosec, value);
          } else if (name == "reader_data_lifecycle.autopurge_nowriter_samples_delay.sec") {
            parse_second(datareaderqos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec, value);
          } else if (name == "reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec") {
            parse_nanosecond(datareaderqos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec, value);
          } else if (name == "reader_data_lifecycle.autopurge_disposed_samples_delay.sec") {
            parse_second(datareaderqos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec, value);
          } else if (name == "reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec") {
            parse_nanosecond(datareaderqos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec, value);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                              ACE_TEXT("Unexpected entry (%C) in [datareaderqos/%C] section.\n"),
                              name.c_str(), datareaderqos_name.c_str()),
                             -1);
          }
        }

        registry_.datareaderqos_map.insert(std::make_pair(datareaderqos_name, datareaderqos));
      }

      return 0;
    }

    int
    StaticDiscovery::parse_publisherqos(ACE_Configuration_Heap& cf)
    {
      const ACE_Configuration_Section_Key& root = cf.root_section();
      ACE_Configuration_Section_Key section;

      if (cf.open_section(root, PUBLISHERQOS_SECTION_NAME, 0, section) != 0) {
        if (DCPS_debug_level > 0) {
          // This is not an error if the configuration file does not have
          // any publisherqos (sub)section.
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_publisherqos ")
                     ACE_TEXT("no [%s] sections.\n"),
                     PUBLISHERQOS_SECTION_NAME));
        }
        return 0;
      }

      // Ensure there are no key/values in the [publisherqos] section.
      // Every key/value must be in a [publisherqos/*] sub-section.
      ValueMap vm;
      if (pullValues(cf, section, vm) > 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                          ACE_TEXT("[publisherqos] sections must have a subsection name\n")),
                         -1);
      }
      // Process the subsections of this section
      KeyList keys;
      if (processSections(cf, section, keys) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                          ACE_TEXT("too many nesting layers in the [publisherqos] section.\n")),
                         -1);
      }

      // Loop through the [publisherqos/*] sections
      for (KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
        std::string publisherqos_name = (*it).first;

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                     ACE_TEXT("processing [publisherqos/%C] section.\n"),
                     publisherqos_name.c_str()));
        }

        ValueMap values;
        pullValues( cf, (*it).second, values );

        DDS::PublisherQos publisherqos(TheServiceParticipant->initial_PublisherQos());

        for (ValueMap::const_iterator it=values.begin(); it != values.end(); ++it) {
          std::string name = (*it).first;
          std::string value = (*it).second;

          if (name == "presentation.access_scope") {
            if (value == "INSTANCE") {
              publisherqos.presentation.access_scope = DDS::INSTANCE_PRESENTATION_QOS;
            } else if (value == "TOPIC") {
              publisherqos.presentation.access_scope = DDS::TOPIC_PRESENTATION_QOS;
            } else if (value == "GROUP") {
              publisherqos.presentation.access_scope = DDS::GROUP_PRESENTATION_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                                ACE_TEXT("Illegal value for presentation.access_scope (%C) in [publisherqos/%C] section.\n"),
                                value.c_str(), publisherqos_name.c_str()),
                               -1);
            }
          } else if (name == "presentation.coherent_access") {
            if (parse_bool(publisherqos.presentation.coherent_access, value)) {
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                                ACE_TEXT("Illegal value for presentation.coherent_access (%C) in [publisherqos/%C] section.\n"),
                                value.c_str(), publisherqos_name.c_str()),
                               -1);
            }
          } else if (name == "presentation.ordered_access") {
            if (parse_bool(publisherqos.presentation.ordered_access, value)) {
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                                ACE_TEXT("Illegal value for presentation.ordered_access (%C) in [publisherqos/%C] section.\n"),
                                value.c_str(), publisherqos_name.c_str()),
                               -1);
            }
          } else if (name == "partition.name") {
            parse_list(publisherqos.partition, value);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                              ACE_TEXT("Unexpected entry (%C) in [publisherqos/%C] section.\n"),
                              name.c_str(), publisherqos_name.c_str()),
                             -1);
          }
        }

        registry_.publisherqos_map.insert(std::make_pair(publisherqos_name, publisherqos));
      }

      return 0;
    }

    int
    StaticDiscovery::parse_subscriberqos(ACE_Configuration_Heap& cf)
    {
      const ACE_Configuration_Section_Key& root = cf.root_section();
      ACE_Configuration_Section_Key section;

      if (cf.open_section(root, SUBSCRIBERQOS_SECTION_NAME, 0, section) != 0) {
        if (DCPS_debug_level > 0) {
          // This is not an error if the configuration file does not have
          // any subscriberqos (sub)section.
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_subscriberqos ")
                     ACE_TEXT("no [%s] sections.\n"),
                     SUBSCRIBERQOS_SECTION_NAME));
        }
        return 0;
      }

      // Ensure there are no key/values in the [subscriberqos] section.
      // Every key/value must be in a [subscriberqos/*] sub-section.
      ValueMap vm;
      if (pullValues(cf, section, vm) > 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                          ACE_TEXT("[subscriberqos] sections must have a subsection name\n")),
                         -1);
      }
      // Process the subsections of this section
      KeyList keys;
      if (processSections(cf, section, keys) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                          ACE_TEXT("too many nesting layers in the [subscriberqos] section.\n")),
                         -1);
      }

      // Loop through the [subscriberqos/*] sections
      for (KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
        std::string subscriberqos_name = (*it).first;

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                     ACE_TEXT("processing [subscriberqos/%C] section.\n"),
                     subscriberqos_name.c_str()));
        }

        ValueMap values;
        pullValues( cf, (*it).second, values );

        DDS::SubscriberQos subscriberqos(TheServiceParticipant->initial_SubscriberQos());

        for (ValueMap::const_iterator it=values.begin(); it != values.end(); ++it) {
          std::string name = (*it).first;
          std::string value = (*it).second;

          if (name == "presentation.access_scope") {
            if (value == "INSTANCE") {
              subscriberqos.presentation.access_scope = DDS::INSTANCE_PRESENTATION_QOS;
            } else if (value == "TOPIC") {
              subscriberqos.presentation.access_scope = DDS::TOPIC_PRESENTATION_QOS;
            } else if (value == "GROUP") {
              subscriberqos.presentation.access_scope = DDS::GROUP_PRESENTATION_QOS;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                                ACE_TEXT("Illegal value for presentation.access_scope (%C) in [subscriberqos/%C] section.\n"),
                                value.c_str(), subscriberqos_name.c_str()),
                               -1);
            }
          } else if (name == "presentation.coherent_access") {
            if (parse_bool(subscriberqos.presentation.coherent_access, value)) {
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                                ACE_TEXT("Illegal value for presentation.coherent_access (%C) in [subscriberqos/%C] section.\n"),
                                value.c_str(), subscriberqos_name.c_str()),
                               -1);
            }
          } else if (name == "presentation.ordered_access") {
            if (parse_bool(subscriberqos.presentation.ordered_access, value)) {
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                                ACE_TEXT("Illegal value for presentation.ordered_access (%C) in [subscriberqos/%C] section.\n"),
                                value.c_str(), subscriberqos_name.c_str()),
                               -1);
            }
          } else if (name == "partition.name") {
            parse_list(subscriberqos.partition, value);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                              ACE_TEXT("Unexpected entry (%C) in [subscriberqos/%C] section.\n"),
                              name.c_str(), subscriberqos_name.c_str()),
                             -1);
          }
        }

        registry_.subscriberqos_map.insert(std::make_pair(subscriberqos_name, subscriberqos));
      }

      return 0;
    }

    int
    StaticDiscovery::parse_endpoints(ACE_Configuration_Heap& cf)
    {
      const ACE_Configuration_Section_Key& root = cf.root_section();
      ACE_Configuration_Section_Key section;

      if (cf.open_section(root, ENDPOINT_SECTION_NAME, 0, section) != 0) {
        if (DCPS_debug_level > 0) {
          // This is not an error if the configuration file does not have
          // any endpoint (sub)section.
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_endpoints ")
                     ACE_TEXT("no [%s] sections.\n"),
                     ENDPOINT_SECTION_NAME));
        }
        return 0;
      }

      // Ensure there are no key/values in the [endpoint] section.
      // Every key/value must be in a [endpoint/*] sub-section.
      ValueMap vm;
      if (pullValues(cf, section, vm) > 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                          ACE_TEXT("[endpoint] sections must have a subsection name\n")),
                         -1);
      }
      // Process the subsections of this section
      KeyList keys;
      if (processSections(cf, section, keys) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                          ACE_TEXT("too many nesting layers in the [endpoint] section.\n")),
                         -1);
      }

      // Loop through the [endpoint/*] sections
      for (KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
        std::string endpoint_name = (*it).first;

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_endpoints ")
                     ACE_TEXT("processing [endpoint/%C] section.\n"),
                     endpoint_name.c_str()));
        }

        ValueMap values;
        pullValues( cf, (*it).second, values );
        int domain;
        unsigned char participant[6];
        unsigned char entity[3];
        enum Type {
          Reader,
          Writer
        };
        Type type;
        OPENDDS_STRING topic_name;
        DDS::DataWriterQos datawriterqos(TheServiceParticipant->initial_DataWriterQos());
        DDS::DataReaderQos datareaderqos(TheServiceParticipant->initial_DataReaderQos());
        DDS::PublisherQos publisherqos(TheServiceParticipant->initial_PublisherQos());
        DDS::SubscriberQos subscriberqos(TheServiceParticipant->initial_SubscriberQos());
        DCPS::TransportLocatorSeq trans_info;

        bool domainSpecified = false,
          participantSpecified = false,
          entitySpecified = false,
          typeSpecified = false,
          topic_name_Specified = false,
          config_name_Specified = false;

        for (ValueMap::const_iterator it=values.begin(); it != values.end(); ++it) {
          std::string name = (*it).first;
          std::string value = (*it).second;

          if (name == "domain") {
            if (convertToInteger(value, domain)) {
              domainSpecified = true;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("Illegal integer value for domain (%C) in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }
          } else if (name == "participant") {
            if (value.size() != 12 ||
                std::count_if(value.begin(), value.end(), isxdigit) != 12) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("participant (%C) must be 12 hexadecimal digits in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }

            for (size_t idx = 0; idx != 6; ++idx) {
              participant[idx] = fromhex(value, idx);
            }
            participantSpecified = true;
          } else if (name == "entity") {
            if (value.size() != 6 ||
                std::count_if(value.begin(), value.end(), isxdigit) != 6) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("entity (%C) must be 6 hexadecimal digits in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }

            for (size_t idx = 0; idx != 3; ++idx) {
              entity[idx] = fromhex(value, idx);
            }
            entitySpecified = true;
          } else if (name == "type") {
            if (value == "reader") {
              type = Reader;
              typeSpecified = true;
            } else if (value == "writer") {
              type = Writer;
              typeSpecified = true;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("Illegal string value for type (%C) in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }
          } else if (name == "topic") {
            EndpointRegistry::TopicMapType::const_iterator pos = registry_.topic_map.find(value);
            if (pos != registry_.topic_map.end()) {
              topic_name = pos->second.name;
              topic_name_Specified = true;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("Illegal topic reference (%C) in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }
          } else if (name == "datawriterqos") {
            EndpointRegistry::DataWriterQosMapType::const_iterator pos = registry_.datawriterqos_map.find(value);
            if (pos != registry_.datawriterqos_map.end()) {
              datawriterqos = pos->second;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("Illegal datawriterqos reference (%C) in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }
          } else if (name == "publisherqos") {
            EndpointRegistry::PublisherQosMapType::const_iterator pos = registry_.publisherqos_map.find(value);
            if (pos != registry_.publisherqos_map.end()) {
              publisherqos = pos->second;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("Illegal publisherqos reference (%C) in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }
          } else if (name == "datareaderqos") {
            EndpointRegistry::DataReaderQosMapType::const_iterator pos = registry_.datareaderqos_map.find(value);
            if (pos != registry_.datareaderqos_map.end()) {
              datareaderqos = pos->second;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("Illegal datareaderqos reference (%C) in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }
          } else if (name == "subscriberqos") {
            EndpointRegistry::SubscriberQosMapType::const_iterator pos = registry_.subscriberqos_map.find(value);
            if (pos != registry_.subscriberqos_map.end()) {
              subscriberqos = pos->second;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("Illegal subscriberqos reference (%C) in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }
          } else if (name == "config") {
            TransportConfig_rch config = TheTransportRegistry->get_config(value);
            if (!config.is_nil()) {
              config->populate_locators(trans_info);
              config_name_Specified = true;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                                ACE_TEXT("Illegal config reference (%C) in [endpoint/%C] section.\n"),
                                value.c_str(), endpoint_name.c_str()),
                               -1);
            }
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                              ACE_TEXT("Unexpected entry (%C) in [endpoint/%C] section.\n"),
                              name.c_str(), endpoint_name.c_str()),
                             -1);
          }
        }

        if (!domainSpecified) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("No domain specified for [endpoint/%C] section.\n"),
                            endpoint_name.c_str()),
                           -1);
        }

        if (!participantSpecified) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("No participant specified for [endpoint/%C] section.\n"),
                            endpoint_name.c_str()),
                           -1);
        }

        if (!entitySpecified) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("No entity specified for [endpoint/%C] section.\n"),
                            endpoint_name.c_str()),
                           -1);
        }

        if (!typeSpecified) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR:StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("No type specified for [endpoint/%C] section.\n"),
                            endpoint_name.c_str()),
                           -1);
        }

        if (!topic_name_Specified) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("No topic specified for [endpoint/%C] section.\n"),
                            endpoint_name.c_str()),
                           -1);
        }

        if (!config_name_Specified) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("No config specified for [endpoint/%C] section.\n"),
                            endpoint_name.c_str()),
                           -1);
        }

        EntityId_t entity_id;

        switch (type) {
        case Reader:
          entity_id = build_id(entity, DCPS::ENTITYKIND_USER_READER_WITH_KEY);
          break;
        case Writer:
          entity_id = build_id(entity, DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);
          break;
        }

        RepoId id = build_id(domain, participant, entity_id);

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: StaticDiscovery::parse_endpoints adding entity with id %s\n"), LogGuid(id).c_str()));
        }

        switch (type) {
        case Reader:
          if (!registry_.reader_map.insert(std::make_pair(id, EndpointRegistry::Reader(topic_name, datareaderqos, subscriberqos, trans_info))).second) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                              ACE_TEXT("Section [endpoint/%C] ignored - duplicate reader.\n"),
                              endpoint_name.c_str()),
                             -1);
          }
          break;
        case Writer:
          if (!registry_.writer_map.insert(std::make_pair(id, EndpointRegistry::Writer(topic_name, datawriterqos, publisherqos, trans_info))).second) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                              ACE_TEXT("Section [endpoint/%C] ignored - duplicate writer.\n"),
                              endpoint_name.c_str()),
                             -1);
          }
          break;
        }
      }

      return 0;
    }
  }
}

#endif /* DDS_HAS_MINIMUM_BIT */
