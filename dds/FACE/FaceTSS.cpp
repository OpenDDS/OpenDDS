#include "FaceTSS.h"
#include "FACE/TS.hpp"
#include "config/Parser.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/SafetyProfileStreams.h"
#include "dds/DCPS/SafetyProfilePool.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DdsDcpsCoreC.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include <cstring>

#ifndef OPENDDS_SAFETY_PROFILE
using OpenDDS::DCPS::operator==;
#endif

namespace FACE {
namespace TS {

bool MessageHeader::operator==(const MessageHeader& rhs) const
{
  return message_instance_guid == rhs.message_instance_guid
    && platform_view_guid == rhs.platform_view_guid
    && message_source_guid == rhs.message_source_guid
    && message_timestamp == rhs.message_timestamp
    && message_validity == rhs.message_validity;
}

using OpenDDS::FaceTSS::config::ConnectionSettings;
using OpenDDS::FaceTSS::config::TopicSettings;
using OpenDDS::FaceTSS::config::QosSettings;

namespace {
  OpenDDS::FaceTSS::config::Parser parser;

  void find_or_create_dp(const DDS::DomainId_t& domainId,
                         int participantId,
                         const DDS::DomainParticipantFactory_var& dpf,
                         DDS::DomainParticipant_var& dp);
  void find_or_create_pub(const DDS::PublisherQos& qos,
                          const DDS::DomainParticipant_var& dp,
                          DDS::Publisher_var& pub);
  void find_or_create_sub(const DDS::SubscriberQos& qos,
                          const DDS::DomainParticipant_var& dp,
                          DDS::Subscriber_var& sub);

  bool cleanup_opendds_publisher(const DDS::Publisher_var pub);
  bool cleanup_opendds_subscriber(const DDS::Subscriber_var sub);
  void cleanup_opendds_participant(const DDS::DomainParticipant_var dp);

  RETURN_CODE_TYPE create_opendds_entities(CONNECTION_ID_TYPE connectionId,
                                           int participantId,
                                           const DDS::DomainId_t domainId,
                                           const char* topic,
                                           const char* type,
                                           CONNECTION_DIRECTION_TYPE dir,
                                           QosSettings& qos,
                                           const char* transport);
}

using OpenDDS::FaceTSS::Entities;

void Initialize(const CONFIGURATION_RESOURCE configuration_file,
                RETURN_CODE_TYPE& return_code)
{
  int status = parser.parse(configuration_file);
  if (status != 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Initialize() ")
               ACE_TEXT("Parser::parse () returned %d\n"),
               status));
    return_code = INVALID_PARAM;
  } else {
    return_code = RC_NO_ERROR;
#if defined OPENDDS_SAFETY_PROFILE && defined ACE_HAS_ALLOC_HOOKS
    TheServiceParticipant->configure_pool();
#endif
  }
}

void Create_Connection(const CONNECTION_NAME_TYPE connection_name,
                       MESSAGING_PATTERN_TYPE pattern,
                       CONNECTION_ID_TYPE& connection_id,
                       CONNECTION_DIRECTION_TYPE& connection_direction,
                       MESSAGE_SIZE_TYPE& max_message_size,
                       TIMEOUT_TYPE,
                       RETURN_CODE_TYPE& return_code)
{
  return_code = RC_NO_ERROR;

  if (pattern != PUB_SUB) {
    return_code = INVALID_CONFIG;
    return;
  }

  ConnectionSettings connection;
  TopicSettings topic;
  QosSettings qos;

  // Find connection
  if (!parser.find_connection(connection_name, connection)) {
    // Find topic
    if (!parser.find_topic(connection.topic_name_, topic)) {
      // Find Qos by specified name(s)
      if (parser.find_qos(connection, qos)) {
        return_code = INVALID_CONFIG;
      }
    } else {
      return_code = INVALID_CONFIG;
    }
  } else {
    return_code = INVALID_CONFIG;
  }

  if (return_code != RC_NO_ERROR) {
    return;
  }

  connection_id = connection.connection_id_;
  connection_direction = connection.direction_;
  max_message_size = topic.max_message_size_;

  return_code = create_opendds_entities(connection_id,
                                        connection.participant_id_,
                                        connection.domain_id_,
                                        connection.topic_name_,
                                        topic.type_name_,
                                        connection_direction,
                                        qos,
                                        connection.config_name_);
  if (return_code != RC_NO_ERROR) {
    return;
  }

  const SYSTEM_TIME_TYPE refresh_period =
    (connection_direction == SOURCE) ?
    OpenDDS::FaceTSS::convertDuration(qos.datawriter_qos().lifespan.duration) : 0;

  const TRANSPORT_CONNECTION_STATUS_TYPE status = {
    0, // MESSAGE currently set to 0 due to type mismatch in MESSAGE_RANGE_TYPE and MESSAGE_TYPE_GUID
    max_message_size, // MAX_MESSAGE with no transformations, set to config value
    max_message_size,
    connection_direction,
    0, // WAITING_PROCESSES_OR_MESSAGES
    refresh_period,
    INVALID,
  };
  Entities::ConnectionInfo& conn_info = Entities::instance()->connections_[connection_id];
  conn_info.connection_name = connection_name;
  conn_info.connection_status = status;
  conn_info.platform_view_guid = topic.platform_view_guid_;
}

void Get_Connection_Parameters(CONNECTION_NAME_TYPE& connection_name,
                               CONNECTION_ID_TYPE& connection_id /* 0 if an out param */,
                               TRANSPORT_CONNECTION_STATUS_TYPE& status,
                               RETURN_CODE_TYPE& return_code)
{
  // connection_name is optional, if absent populate from connection_id lookup
  // connection_id is also optional, if absent populate from connection_name lookup
  // if both provided, validate
  // if neither present, return error
  Entities& entities = *Entities::instance();

  if (connection_id != 0 && entities.connections_.count(connection_id)) {
    // connection_id was provided
    // if validated or populated, set return_code so status will be populated
    if (connection_name[0]) {
      // Validate provided connection_name
      OPENDDS_STRING conn_name = entities.connections_[connection_id].connection_name;
      if (std::strcmp(connection_name, conn_name.c_str()) == 0) {
        return_code = RC_NO_ERROR;
      } else {
        return_code = INVALID_PARAM;
      }
    } else {
      // connection_name not provided
      // so populate from connection_id lookup
      // and set return code so status will be populated
      entities.connections_[connection_id].connection_name.copy(connection_name,
                                                      sizeof(CONNECTION_NAME_TYPE));
      connection_name[sizeof(CONNECTION_NAME_TYPE) - 1] = 0;
      return_code = RC_NO_ERROR;
    }

  } else if (connection_name[0] && connection_id == 0) {
    // connection_id was not specified, but name was provided.
    // lookup connection_id and if found set return code to populate status
    ConnectionSettings settings;
    if (0 == parser.find_connection(connection_name, settings)) {
      connection_id = settings.connection_id_;
      return_code = RC_NO_ERROR;
    } else {
      // could not find connection for connection_name
      return_code = INVALID_PARAM;
    }
  } else {
    //Neither connection_id or connection_name provided
    // a valid connection
    return_code = INVALID_PARAM;
  }
  if (return_code == RC_NO_ERROR) {
    TRANSPORT_CONNECTION_STATUS_TYPE& cur_status = entities.connections_[connection_id].connection_status;
    if (cur_status.CONNECTION_DIRECTION == FACE::DESTINATION) {
      Entities::FaceReceiver& receiver = *entities.receivers_[connection_id];
      if (receiver.status_valid != FACE::VALID) {
        if (OpenDDS::DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, "Get_Connection_Parameters: returning NOT_AVAILABLE due to receiver's status not valid\n"));
        }
        return_code = NOT_AVAILABLE;
        return;
      }
      if (receiver.total_msgs_recvd != 0) {
        cur_status.REFRESH_PERIOD = receiver.sum_recvd_msgs_latency/receiver.total_msgs_recvd;
        cur_status.LAST_MSG_VALIDITY = receiver.last_msg_header.message_validity;
      } else {
        cur_status.REFRESH_PERIOD = 0;
      }
      WAITING_RANGE_TYPE num_waiting;
      if (receiver.messages_waiting(num_waiting) == FACE::RC_NO_ERROR) {
          cur_status.WAITING_PROCESSES_OR_MESSAGES = num_waiting;
      } else {
        if (OpenDDS::DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, "Get_Connection_Parameters: returning NOT_AVAILABLE due to messages_waiting\n"));
        }
        return_code = NOT_AVAILABLE;
        return;
      }
    } else {
      //DDS Only supports Destination/Source therefore
      // CONNECTION_DIRECTION == FACE::SOURCE
      Entities::FaceSender& sender = entities.senders_[connection_id];
      if (sender.status_valid != FACE::VALID) {
        return_code = NOT_AVAILABLE;
        return;
      }
      cur_status.REFRESH_PERIOD = 0;
      cur_status.WAITING_PROCESSES_OR_MESSAGES = 0;
    }
    status = cur_status;
  }
}

void Unregister_Callback(CONNECTION_ID_TYPE connection_id,
                         RETURN_CODE_TYPE& return_code)
{
  Entities& entities = *Entities::instance();
  Entities::ConnIdToReceiverMap& readers = entities.receivers_;
  if (readers.count(connection_id)) {
    readers[connection_id]->dr->set_listener(NULL, 0);
    return_code = RC_NO_ERROR;
    return;
  }
  return_code = INVALID_PARAM;
}

void Destroy_Connection(CONNECTION_ID_TYPE connection_id,
                        RETURN_CODE_TYPE& return_code)
{
  Entities& entities = *Entities::instance();
  Entities::ConnIdToSenderMap& writers = entities.senders_;
  Entities::ConnIdToReceiverMap& readers = entities.receivers_;

  DDS::DomainParticipant_var dp;
  bool try_cleanup_participant = false;
  if (writers.count(connection_id)) {
    const DDS::DataWriter_var datawriter = writers[connection_id].dw;
    const DDS::Publisher_var pub = datawriter->get_publisher();
    writers.erase(connection_id);
    pub->delete_datawriter(datawriter);
    dp = pub->get_participant();
    try_cleanup_participant = cleanup_opendds_publisher(pub);

  } else if (readers.count(connection_id)) {
    const DDS::DataReader_var datareader = readers[connection_id]->dr;
    const DDS::Subscriber_var sub = datareader->get_subscriber();
    delete readers[connection_id];
    readers.erase(connection_id);
    sub->delete_datareader(datareader);
    dp = sub->get_participant();
    try_cleanup_participant = cleanup_opendds_subscriber(sub);
  }

  if (!dp) {
    return_code = INVALID_PARAM;
    return;
  }

  if (try_cleanup_participant) {
    cleanup_opendds_participant(dp);
  }

  entities.connections_.erase(connection_id);
  return_code = RC_NO_ERROR;
}
OpenDDS_FACE_Export
void receive_header(/*in*/    FACE::CONNECTION_ID_TYPE connection_id,
                    /*in*/    FACE::TIMEOUT_TYPE /*timeout*/,
                    /*inout*/ FACE::TRANSACTION_ID_TYPE& transaction_id,
                    /*inout*/ FACE::TS::MessageHeader& message_header,
                    /*in*/    FACE::MESSAGE_SIZE_TYPE message_size,
                    /*out*/   FACE::RETURN_CODE_TYPE& return_code)
{
  Entities::ConnIdToReceiverMap& readers =
    Entities::instance()->receivers_;
  // transaction_id cannot be 0 due to initialization
  // of last_msg_tid to 0 before a msg has been received so
  // only valid transaction_ids are > 0.
  if (!readers.count(connection_id) || transaction_id == 0) {
    if (OpenDDS::DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) receive_header - INVALID_PARAM - "
          "could not find reader for connection_id: %d OR transaction id[%d] == 0 \n",
          connection_id,
          transaction_id));
    }
    return_code = FACE::INVALID_PARAM;
    return;
  }

  if (message_size < 0 || (unsigned)message_size < sizeof(FACE::TS::MessageHeader)) {
    if (OpenDDS::DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) receive_header - INVALID_PARAM - message_size: %d is < %d \n",
          message_size,
          sizeof(FACE::TS::MessageHeader)));
    }
    return_code = FACE::INVALID_PARAM;
    return;
  }
  if (transaction_id == readers[connection_id]->last_msg_tid) {
    message_header = readers[connection_id]->last_msg_header;
    return_code = OpenDDS::FaceTSS::update_status(connection_id, DDS::RETCODE_OK);
  } else {
    return_code = OpenDDS::FaceTSS::update_status(connection_id, DDS::RETCODE_BAD_PARAMETER);
  }
}

void Receive_Message(
  /* in */ CONNECTION_ID_TYPE connection_id,
  /* in */ TIMEOUT_TYPE timeout,
  /* inout */ TRANSACTION_ID_TYPE& transaction_id,
  /* out */ MessageHeader& message_header,
  /* in */ MESSAGE_SIZE_TYPE message_size,
  /* out */ RETURN_CODE_TYPE& return_code)
{
  receive_header(connection_id, timeout,
                 transaction_id, message_header,
                 message_size, return_code);
}

namespace {
  void find_or_create_dp(const DDS::DomainId_t& domainId,
                         int participantId,
                         const DDS::DomainParticipantFactory_var& dpf,
                         DDS::DomainParticipant_var& dp) {
    DDS::DomainParticipant_var temp_dp;

    temp_dp = dpf->lookup_participant(domainId);
    if (!temp_dp) {
      if (OpenDDS::DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) find_or_create_dp - created new participant for domainId: %d\n", domainId));
      }
      DDS::DomainParticipantQos qos(PARTICIPANT_QOS_DEFAULT);
      qos.user_data.value.length(6);
      qos.user_data.value[0] = (participantId >> 0) & 0xFF;
      qos.user_data.value[1] = (participantId >> 8) & 0xFF;
      qos.user_data.value[2] = (participantId >> 16) & 0xFF;
      qos.user_data.value[3] = (participantId >> 24) & 0xFF;
      qos.user_data.value[4] = 0; // (participantId >> 32) & 0xFF;
      qos.user_data.value[5] = 0; // (participantId >> 40) & 0xFF;
      dp = dpf->create_participant(domainId, qos, 0, 0);
      return;
    }
    if (OpenDDS::DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) find_or_create_dp - found exiting participant for domainId: %d \n", domainId));
    }
    dp = temp_dp;
  }

  void find_or_create_pub(const DDS::PublisherQos& qos,
                          const DDS::DomainParticipant_var& dp,
                          DDS::Publisher_var& pub) {
    DDS::DomainParticipant_var temp_dp;
    DDS::Publisher_var temp_pub;

    Entities::ConnIdToSenderMap& writers = Entities::instance()->senders_;
    Entities::ConnIdToSenderMap::iterator wtrIter = writers.begin();

    while (wtrIter != writers.end()) {
      temp_pub = wtrIter->second.dw->get_publisher();
      temp_dp = temp_pub->get_participant();
      DDS::PublisherQos temp_qos;
      temp_pub->get_qos(temp_qos);
      if (dp->get_domain_id() == temp_dp->get_domain_id() &&
          temp_qos == qos) {
        if (OpenDDS::DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) find_or_create_pub - found exiting publisher in senders\n"));
        }
        pub = temp_pub;
        return;
      } else {
        ++wtrIter;
      }
    }
    if (OpenDDS::DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) find_or_create_pub - created new publisher\n"));
    }
    pub = dp->create_publisher(qos, 0, 0);
  }

  void find_or_create_sub(const DDS::SubscriberQos& qos,
                          const DDS::DomainParticipant_var& dp,
                          DDS::Subscriber_var& sub)
  {
    DDS::DomainParticipant_var temp_dp;
    DDS::Subscriber_var temp_sub;

    Entities::ConnIdToReceiverMap& readers = Entities::instance()->receivers_;
    Entities::ConnIdToReceiverMap::iterator rdrIter = readers.begin();

    while (rdrIter != readers.end()) {
      temp_sub = rdrIter->second->dr->get_subscriber();
      temp_dp = temp_sub->get_participant();
      DDS::SubscriberQos temp_qos;
      temp_sub->get_qos(temp_qos);
      if (dp->get_domain_id() == temp_dp->get_domain_id() &&
          temp_qos == qos) {
        if (OpenDDS::DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) find_or_create_sub - found exiting subscriber in receivers\n"));
        }
        sub = temp_sub;
        return;
      } else {
        ++rdrIter;
      }
    }
    if (OpenDDS::DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) find_or_create_sub - created new subscriber\n"));
    }
    sub = dp->create_subscriber(qos, 0, 0);
  }

  bool cleanup_opendds_publisher(const DDS::Publisher_var pub)
  {
    DDS::Publisher_var temp_pub;
    DDS::PublisherQos pub_qos;
    pub->get_qos(pub_qos);
    DDS::DomainParticipant_var dp = pub->get_participant();
    DDS::DomainParticipant_var temp_dp;
    DDS::PublisherQos temp_qos;

    Entities::ConnIdToSenderMap& writers = Entities::instance()->senders_;
    Entities::ConnIdToSenderMap::iterator wtrIter = writers.begin();
    while (wtrIter != writers.end()) {
      temp_pub = wtrIter->second.dw->get_publisher();
      temp_dp = temp_pub->get_participant();
      if (dp->get_domain_id() == temp_dp->get_domain_id()) {
        temp_pub->get_qos(temp_qos);
        if (pub_qos == temp_qos) {
          if (OpenDDS::DCPS::DCPS_debug_level > 3) {
            ACE_DEBUG((LM_DEBUG, "(%P|%t) cleanup_opendds_publisher - publisher still in use by other writer\n"));
          }
          return false;
        }
      }
      ++wtrIter;
    }
    if (OpenDDS::DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) cleanup_opendds_publisher - publisher no longer in use, delete pub\n"));
    }
    dp->delete_publisher(pub);
    return true;
  }

  bool cleanup_opendds_subscriber(const DDS::Subscriber_var sub)
  {
    DDS::Subscriber_var temp_sub;
    DDS::SubscriberQos sub_qos;
    sub->get_qos(sub_qos);
    DDS::DomainParticipant_var dp = sub->get_participant();
    DDS::DomainParticipant_var temp_dp;
    DDS::SubscriberQos temp_qos;


    Entities::ConnIdToReceiverMap& readers = Entities::instance()->receivers_;
    Entities::ConnIdToReceiverMap::iterator rdrIter = readers.begin();
    while (rdrIter != readers.end()) {
      temp_sub = rdrIter->second->dr->get_subscriber();
      temp_dp = temp_sub->get_participant();

      if (dp->get_domain_id() == temp_dp->get_domain_id()) {
        temp_sub->get_qos(temp_qos);
        if (sub_qos == temp_qos) {
          if (OpenDDS::DCPS::DCPS_debug_level > 3) {
            ACE_DEBUG((LM_DEBUG, "(%P|%t) cleanup_opendds_subscriber - subscriber still in use by other reader\n"));
          }
          return false;
        }
      }
      ++rdrIter;
    }
    if (OpenDDS::DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) cleanup_opendds_subscriber - subscriber no longer in use, delete sub\n"));
    }
    dp->delete_subscriber(sub);
    return true;
  }

  void cleanup_opendds_participant(const DDS::DomainParticipant_var dp)
  {
    DDS::DomainParticipant_var temp_dp;
    Entities::ConnIdToReceiverMap& readers = Entities::instance()->receivers_;
    Entities::ConnIdToReceiverMap::iterator rdrIter = readers.begin();
    while (rdrIter != readers.end()) {
      temp_dp = rdrIter->second->dr->get_subscriber()->get_participant();
      if (dp->get_domain_id() == temp_dp->get_domain_id()) {
        if (OpenDDS::DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) cleanup_opendds_participant - participant still in use by reader\n"));
        }
        return;
      } else {
        ++rdrIter;
      }
    }

    Entities::ConnIdToSenderMap& writers = Entities::instance()->senders_;
    Entities::ConnIdToSenderMap::iterator wtrIter = writers.begin();

    while (wtrIter != writers.end()) {
      temp_dp = wtrIter->second.dw->get_publisher()->get_participant();
      if (dp->get_domain_id() == temp_dp->get_domain_id()) {
        if (OpenDDS::DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) cleanup_opendds_participant - participant still in use by writer\n"));
        }
        return;
      } else {
        ++wtrIter;
      }
    }
    if (OpenDDS::DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) cleanup_opendds_participant - participant for domain: %d no longer in use, delete entities and participant\n", dp->get_domain_id()));
    }
    dp->delete_contained_entities();
    const DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    dpf->delete_participant(dp);
  }

  RETURN_CODE_TYPE create_opendds_entities(CONNECTION_ID_TYPE connectionId,
                                           int participantId,
                                           const DDS::DomainId_t domainId,
                                           const char* topicName,
                                           const char* type,
                                           CONNECTION_DIRECTION_TYPE dir,
                                           QosSettings& qos_settings,
                                           const char* transport)
  {
#ifdef DEBUG_OPENDDS_FACETSS
    OpenDDS::DCPS::set_DCPS_debug_level(8);
    OpenDDS::DCPS::Transport_debug_level = 5;
    TheServiceParticipant->set_BIT(false);
#endif

    const DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    if (!dpf) return INVALID_PARAM;

    DDS::DomainParticipant_var dp;
    find_or_create_dp(domainId, participantId, dpf, dp);
    if (!dp) return INVALID_PARAM;

    using OpenDDS::DCPS::Data_Types_Register;
    OpenDDS::DCPS::TypeSupport* ts =
      Registered_Data_Types->lookup(dp, type);
    if (!ts) {
      ts = Registered_Data_Types->lookup(0, type);
      if (!ts) return INVALID_PARAM;
      Registered_Data_Types->register_type(dp, type, ts);
    }

    const DDS::Topic_var topic =
      dp->create_topic(topicName, type, TOPIC_QOS_DEFAULT, 0, 0);
    if (!topic) return INVALID_PARAM;

    if (dir == SOURCE) {
      DDS::PublisherQos publisher_qos;
      qos_settings.apply_to(publisher_qos);

      DDS::Publisher_var pub;
      find_or_create_pub(publisher_qos, dp, pub);
      if (!pub) return INVALID_PARAM;

      if (transport && transport[0]) {
        OpenDDS::DCPS::TransportConfig_rch config = TheTransportRegistry->get_config(transport);
        if (config.is_nil()) return INVALID_PARAM;
        TheTransportRegistry->bind_config(config, pub);
      }

      DDS::DataWriterQos datawriter_qos;
      qos_settings.apply_to(datawriter_qos);

      // set up user data in DW qos
      datawriter_qos.user_data.value.length (3);
      datawriter_qos.user_data.value[0] = (connectionId >> 0) & 0xFF;
      datawriter_qos.user_data.value[1] = (connectionId >> 8) & 0xFF;
      datawriter_qos.user_data.value[2] = (connectionId >> 16) & 0xFF;

      const DDS::DataWriter_var dw =
        pub->create_datawriter(topic, datawriter_qos, 0, 0);
      if (!dw) return INVALID_PARAM;

      Entities::instance()->senders_[connectionId].dw = dw;

    } else { // dir == DESTINATION
      DDS::SubscriberQos subscriber_qos;
      qos_settings.apply_to(subscriber_qos);

      DDS::Subscriber_var sub;
      find_or_create_sub(subscriber_qos, dp, sub);
      if (!sub) return INVALID_PARAM;

      if (transport && transport[0]) {
        OpenDDS::DCPS::TransportConfig_rch config = TheTransportRegistry->get_config(transport);
        if (config.is_nil()) return INVALID_PARAM;
        TheTransportRegistry->bind_config(config, sub);
      }

      DDS::DataReaderQos datareader_qos;
      qos_settings.apply_to(datareader_qos);

      // set up user data in DR qos
      datareader_qos.user_data.value.length (3);
      datareader_qos.user_data.value[0] = (connectionId >> 0) & 0xFF;
      datareader_qos.user_data.value[1] = (connectionId >> 8) & 0xFF;
      datareader_qos.user_data.value[2] = (connectionId >> 16) & 0xFF;

      const DDS::DataReader_var dr =
        sub->create_datareader(topic, datareader_qos, 0, 0);
      if (!dr) return INVALID_PARAM;
      Entities::instance()->receivers_[connectionId] = new Entities::FaceReceiver();
      Entities::instance()->receivers_[connectionId]->dr = dr;
    }

    return RC_NO_ERROR;
  }
}

}}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace FaceTSS {

Entities::Entities() {}
Entities::~Entities() {}

Entities* Entities::instance()
{
  return ACE_Singleton<Entities, ACE_Thread_Mutex>::instance();
}

FACE::RETURN_CODE_TYPE update_status(FACE::CONNECTION_ID_TYPE connection_id,
  DDS::ReturnCode_t retcode)
{
  FACE::TRANSPORT_CONNECTION_STATUS_TYPE& status =
    Entities::instance()->connections_[connection_id].connection_status;
  FACE::RETURN_CODE_TYPE rc = FACE::INVALID_PARAM;

  switch (retcode) {
  case DDS::RETCODE_OK:
    status.LAST_MSG_VALIDITY = FACE::VALID;
    return FACE::RC_NO_ERROR;

  case DDS::RETCODE_ERROR:
    rc = FACE::CONNECTION_CLOSED; break;

  case DDS::RETCODE_BAD_PARAMETER:
    rc = FACE::INVALID_PARAM; break;

  case DDS::RETCODE_OUT_OF_RESOURCES:
    rc = FACE::DATA_BUFFER_TOO_SMALL; break;

  case DDS::RETCODE_PRECONDITION_NOT_MET:
  case DDS::RETCODE_NOT_ENABLED:
    rc = FACE::INVALID_MODE; break;

  case DDS::RETCODE_IMMUTABLE_POLICY:
  case DDS::RETCODE_INCONSISTENT_POLICY:
    rc = FACE::INVALID_CONFIG; break;

  case DDS::RETCODE_ALREADY_DELETED:
    rc = FACE::CONNECTION_CLOSED; break;

  case DDS::RETCODE_TIMEOUT:
    rc = FACE::TIMED_OUT; break;

  case DDS::RETCODE_UNSUPPORTED:
  case DDS::RETCODE_NO_DATA:
    rc = FACE::NOT_AVAILABLE; break;

  case DDS::RETCODE_ILLEGAL_OPERATION:
    rc = FACE::PERMISSION_DENIED; break;
  }

  status.LAST_MSG_VALIDITY = FACE::INVALID;
  return rc;
}

enum { NSEC_PER_SEC = 1000000000 };

DDS::Duration_t convertTimeout(FACE::TIMEOUT_TYPE timeout)
{
  if (timeout == FACE::INF_TIME_VALUE) {
    static const DDS::Duration_t dds_inf = {DDS::DURATION_INFINITE_SEC,
                                            DDS::DURATION_INFINITE_NSEC};
    return dds_inf;
  }

  DDS::Duration_t dur = {static_cast<int>(timeout / NSEC_PER_SEC),
                         static_cast<unsigned int>(timeout % NSEC_PER_SEC)};
  return dur;
}

FACE::SYSTEM_TIME_TYPE convertDuration(const DDS::Duration_t& duration)
{
  if (duration.sec == DDS::DURATION_INFINITE_SEC
      && duration.nanosec == DDS::DURATION_INFINITE_NSEC) {
    return FACE::INF_TIME_VALUE;
  }
  return duration.nanosec +
    duration.sec * static_cast<FACE::SYSTEM_TIME_TYPE>(NSEC_PER_SEC);
}

FACE::SYSTEM_TIME_TYPE convertTime(const DDS::Time_t& timestamp)
{
  return timestamp.nanosec +
      timestamp.sec * static_cast<FACE::SYSTEM_TIME_TYPE>(NSEC_PER_SEC);
}

FACE::MESSAGE_INSTANCE_GUID
create_message_instance_guid(const OpenDDS::DCPS::RepoId& pub, const CORBA::LongLong& orig_seq)
{
  OpenDDS::DCPS::GuidConverter writer(pub);

  FACE::MESSAGE_INSTANCE_GUID message_instance_guid;
  FACE::LongLong mig_low;
  FACE::LongLong masked_seq;

  //Until MESSAGE_INSTANCE_GUID becomes 128 bit GUID, use checksum to represent Prefix
  FACE::Long prefix_representation = ACE::crc32(reinterpret_cast<const void*>(&pub), sizeof(pub));
  masked_seq = orig_seq >> 32;

  if (masked_seq) {
    ACE_DEBUG((LM_WARNING, "(%P|%t) WARNING: create_message_instance_guid - seq does not fit in FACE::Long, truncating high bits to fit\n"));
  }
  mig_low = orig_seq & 0xFFFFFFFF;
  message_instance_guid = (((FACE::LongLong) prefix_representation) << 32 ) | ((FACE::LongLong) mig_low);
/*
  //TODO: This is initial work toward defining how a 128 bit guid could be created
  //in the future for a Message Instance Guid when supported.
  // 13 byte prefix contains identifying pieces of guid
  // 3 byte seq - truncated sequence from the sample
  typedef CORBA::Octet MsgInstGuidPrefix_t[13];
  typedef CORBA::Octet MsgInstGuidSeq_t[3];
  MsgInstGuidPrefix_t migPrefix;
  MsgInstGuidSeq_t migSeq;
  ACE_OS::memcpy(&migPrefix[0], &pub.guidPrefix[2], 10);
  ACE_OS::memcpy(&migPrefix[10], &pub.entityId.entityKey[0], 3);
  masked_seq = orig_seq >> 24;

  if (masked_seq) {
    ACE_DEBUG((LM_WARNING, "(%P|%t) WARNING: create_message_instance_guid - seq does not fit in 3 bytes, truncating high bits to fit\n"));
  }
  FACE::LongLong masked = orig_seq & 0xFFFFFF;

#ifdef ACE_BIG_ENDIAN
  masked <<= 8 * (sizeof(FACE::LongLong)-3);
#endif
  ACE_OS::memcpy(&migSeq[0], &masked, 3);
*/
  return message_instance_guid;
}

void populate_header_received(const FACE::CONNECTION_ID_TYPE& connection_id,
                              const DDS::DomainParticipant_var part,
                              const DDS::SampleInfo& sinfo,
                              FACE::RETURN_CODE_TYPE& return_code)
{
  Entities::ConnIdToReceiverMap& readers = Entities::instance()->receivers_;
  if (!readers.count(connection_id)) {
    return_code = FACE::INVALID_PARAM;
    return;
  }
  FACE::TS::MessageHeader& header = readers[connection_id]->last_msg_header;

  header.platform_view_guid = Entities::instance()->connections_[connection_id].platform_view_guid;

  DDS::Subscriber_var temp_sub = readers[connection_id]->dr->get_subscriber();
  DDS::DomainParticipant_var temp_dp = temp_sub->get_participant();
  const OpenDDS::DCPS::RepoId pub = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(temp_dp.in())->get_repoid(sinfo.publication_handle);
  header.message_instance_guid = create_message_instance_guid(pub, sinfo.opendds_reserved_publication_seq);

  header.message_timestamp = convertTime(sinfo.source_timestamp);
  ACE_Time_Value now(ACE_OS::gettimeofday());

  readers[connection_id]->sum_recvd_msgs_latency += (convertTime(OpenDDS::DCPS::time_value_to_time(now)) - header.message_timestamp);
  ++readers[connection_id]->total_msgs_recvd;

  if (OpenDDS::DCPS::DCPS_debug_level > 8) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) populate_header_received: Latency is now (tot_latency %d / tot_msgs_recvd %d): %d\n",
        readers[connection_id]->sum_recvd_msgs_latency,
        readers[connection_id]->total_msgs_recvd,
        readers[connection_id]->sum_recvd_msgs_latency/readers[connection_id]->total_msgs_recvd));
  }
  ::DDS::Subscriber_var bit_subscriber
   = part->get_builtin_subscriber () ;

  ::DDS::DataReader_var reader
   = bit_subscriber->lookup_datareader (OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC) ;
  ::DDS::PublicationBuiltinTopicDataDataReader_var pub_reader
   = ::DDS::PublicationBuiltinTopicDataDataReader::_narrow (reader.in ());
  if (CORBA::is_nil (pub_reader.in ())) {
    ACE_ERROR((LM_ERROR, "(%P|%t) populate_header_received: failed to get BUILT_IN_PUBLICATION_TOPIC datareader.\n"));
    return_code = FACE::NOT_AVAILABLE;
    return;
  }

  ::DDS::ReturnCode_t ret;
  ::DDS::SampleInfoSeq pubinfos(1);
  ::DDS::PublicationBuiltinTopicDataSeq pubdata(1);
  ret = pub_reader->read_instance(pubdata,
                                  pubinfos,
                                  1,
                                  sinfo.publication_handle,
                                  ::DDS::ANY_SAMPLE_STATE,
                                  ::DDS::ANY_VIEW_STATE,
                                  ::DDS::ALIVE_INSTANCE_STATE);

  if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA) {
    ACE_ERROR((LM_ERROR,
        "(%P|%t) populate_header_received:  failed to read BIT publication data.\n"));
    return_code = FACE::NOT_AVAILABLE;
    return;
  }

  const CORBA::ULong i = 0;

  header.message_source_guid =
    (pubdata[i].user_data.value[0] << 0) |
    (pubdata[i].user_data.value[1] << 8) |
    (pubdata[i].user_data.value[2] << 16);

//  ACE_DEBUG((LM_DEBUG, "(%P|%t) populate_header_received: DW lifespan qos value: sec: %d nanosec: %d\n",
//                         pubdata[i].lifespan.duration.sec, pubdata[i].lifespan.duration.nanosec));

  DDS::Duration_t lifespan = pubdata[i].lifespan.duration;
  if (lifespan.sec != DDS::DURATION_INFINITE_SEC &&
      lifespan.nanosec != DDS::DURATION_INFINITE_NSEC) {
    // Finite lifespan.  Check if data has expired.

    DDS::Time_t const tmp = {
      sinfo.source_timestamp.sec + lifespan.sec,
      sinfo.source_timestamp.nanosec + lifespan.nanosec
    };

    // We assume that the publisher host's clock and subscriber host's
    // clock are synchronized (allowed by the spec).
    ACE_Time_Value const expiration_time(
        OpenDDS::DCPS::time_to_time_value(tmp));

    if (now >= expiration_time) {
//      ACE_DEBUG((LM_DEBUG, "(%P|%t) populate_header_received: Last message expired, setting message_validity to INVALID\n"));
      header.message_validity = FACE::INVALID;
      return_code = FACE::RC_NO_ERROR;
      return;
    }
  }
//  ACE_DEBUG((LM_DEBUG, "(%P|%t) populate_header_received: Setting message_validity to VALID\n"));
  header.message_validity = FACE::VALID;
  return_code = FACE::RC_NO_ERROR;
}
}}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
