// $Id$
#include "ParticipantQos_Handler.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/debug.h"

bool
ParticipantQos_Handler::get_participant_qos (DDS::DomainParticipantQos& dp_qos,
                                           dds::qosProfile * profile,
                                           const ACE_TCHAR * name)
{
  if (name)
    {
//       // find the correct Participant_qos
//       for (dds::qosProfile::Participant_qos_iterator dw_it = profile->begin_Participant_qos ();
//           dw_it != profile->end_Participant_qos();
//           ++dw_it)
//         {
//           if (ACE_OS::strcmp (dw_name.c_str (), name) == 0)
//             {
//               return get_Participant_qos (dw_qos, *dw_it->get());
//             }
//         }
    }
  else
    {
      if (profile->count_domainparticipant_qos () == 0)
        {
          if (OpenDDS::DCPS::DCPS_debug_level > 8)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("ParticipantQos_Handler::get_participant_qos - ")
                ACE_TEXT ("No Participant QOS available in profile <%C>\n"),
                profile->name ().c_str ()));
            }
          return true;
        }
      // get the first Participant in the XML
      dds::qosProfile::domainparticipant_qos_iterator dr_it = profile->begin_domainparticipant_qos ();
      return ParticipantQos_Handler::get_participant_qos (dp_qos, dr_it->get ());
    }

  ACE_ERROR ((LM_DEBUG,
    ACE_TEXT ("ParticipantQos_Handler::get_participant_qos - ")
    ACE_TEXT ("Unable to find ParticipantQos <%C>\n"),
    name));
  return false;
}

bool
ParticipantQos_Handler::get_participant_qos (DDS::DomainParticipantQos& dp_qos,
                                           dds::domainparticipantQos * dp)
{

//  if (xml_qos->user_data ())
//    {
      // TODO: Have a good look at this.

//       const std::string value = *xml_qos->user_data ()->value ();
//
//       ACE_DEBUG ((LM_TRACE,
//         ACE_TEXT ("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
//         ACE_TEXT ("Set user_data to <%C>\n"),
//         value.c_str ()));
//
//       dds_qos.user_data.value =
//         *xml_qos->user_data ()->value ();
//     }


  if (dp->entity_factory_p ())
    {
      if (dp->entity_factory ().autoenable_created_entities_p ())
        {
          dp_qos.entity_factory.autoenable_created_entities =
            dp->entity_factory ().autoenable_created_entities ();

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("ParticipantQos_Handler::get_participant_qos - ")
                ACE_TEXT ("Set entity_factory autoenable_created_entities to <%d>\n"),
                dp_qos.entity_factory.autoenable_created_entities));
            }
        }
    }
  return true;
}
