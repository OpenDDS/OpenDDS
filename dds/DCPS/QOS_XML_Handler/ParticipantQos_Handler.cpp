#include "ParticipantQos_Handler.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/debug.h"
#include <memory>
#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

bool
ParticipantQos_Handler::get_participant_qos(DDS::DomainParticipantQos& dp_qos,
                                            const dds::qosProfile * profile,
                                            std::string_view name)
{
  if (!name.empty())
    {
//       // find the correct Participant_qos
//       for (dds::qosProfile::Participant_qos_iterator dw_it = profile->begin_Participant_qos();
//           dw_it != profile->end_Participant_qos();
//           ++dw_it)
//         {
//           if (ACE_OS::strcmp(dw_name.c_str(), name) == 0)
//             {
//               return get_participant_qos(dp_qos, std::addressof(participant_qos));
//             }
//         }
    }
  else
    {
      if (profile->domainparticipant_qos().empty())
        {
          if (OpenDDS::DCPS::DCPS_debug_level > 8)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("ParticipantQos_Handler::get_participant_qos - ")
                ACE_TEXT("No Participant QOS available in profile <%s>\n"),
                profile->name().c_str()));
            }
          return true;
        }
      // get the first Participant in the XML
      const dds::domainparticipantQos& participant_qos = profile->domainparticipant_qos().front();
      return ParticipantQos_Handler::get_participant_qos(dp_qos, std::addressof(participant_qos));
    }

  const std::string name_string (name);
  ACE_ERROR((LM_ERROR,
    ACE_TEXT("(%P|%t) ERROR: ParticipantQos_Handler::get_participant_qos - ")
    ACE_TEXT("Unable to find ParticipantQos <%C>\n"),
    name_string.c_str()));
  return false;
}

bool
ParticipantQos_Handler::get_participant_qos(DDS::DomainParticipantQos& dp_qos,
                                            const dds::domainparticipantQos * dp)
{

//  if (xml_qos->user_data())
//    {
      // TODO: Have a good look at this.

//       const std::string value = *xml_qos->user_data()->value_if_set();
//
//       ACE_DEBUG((LM_TRACE,
//         ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
//         ACE_TEXT("Set user_data to <%C>\n"),
//         value.c_str()));
//
//       dds_qos.user_data.value =
//         *xml_qos->user_data()->value_if_set();
//     }


  const ::dds::entityFactoryQosPolicy* entity_factory_if_set_ptr = dp->entity_factory_if_set();
  if (entity_factory_if_set_ptr)
    {
      const ::XMLSchema::boolean* autoenable_ptr = entity_factory_if_set_ptr->autoenable_created_entities_if_set();
      if (autoenable_ptr)
        {
          dp_qos.entity_factory.autoenable_created_entities = *autoenable_ptr;

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("ParticipantQos_Handler::get_participant_qos - ")
                ACE_TEXT("Set entity_factory autoenable_created_entities to <%d>\n"),
                dp_qos.entity_factory.autoenable_created_entities));
            }
        }
    }
  return true;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
