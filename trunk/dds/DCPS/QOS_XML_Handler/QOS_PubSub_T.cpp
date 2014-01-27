// $Id$

#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"
#include "dds/DCPS/debug.h"

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_PubSub_T (void)
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_PubSub_T (void)
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos (DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
//  if (xml_qos->group_data ())
//    {
      // TODO: Have a good look at this.

//       const std::string value = *xml_qos->group_data ()->value ();
//
//       ACE_DEBUG ((LM_TRACE,
//         ACE_TEXT ("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
//         ACE_TEXT ("Set group_data to <%C>\n"),
//         value.c_str ()));
//
//       dds_qos.group_data.value =
//         *xml_qos->group_data ()->value ();
//     }

    if (xml_qos->presentation_p ())
      {
        if (xml_qos->presentation ().access_scope_p ())
          {
            switch (xml_qos->presentation ().access_scope ().integral ())
              {
              case ::dds::presentationAccessScopeKind::INSTANCE_PRESENTATION_QOS_l:
                dds_qos.presentation.access_scope = ::DDS::INSTANCE_PRESENTATION_QOS;
                break;
              case ::dds::presentationAccessScopeKind::TOPIC_PRESENTATION_QOS_l:
                dds_qos.presentation.access_scope = ::DDS::TOPIC_PRESENTATION_QOS;
                break;
              case ::dds::presentationAccessScopeKind::GROUP_PRESENTATION_QOS_l:
                dds_qos.presentation.access_scope = ::DDS::GROUP_PRESENTATION_QOS;
                break;
              default:
                ACE_ERROR ((LM_DEBUG,
                  ACE_TEXT ("QOS_PubSub_T::read_qos - ")
                  ACE_TEXT ("Unknown presentation access scope found <%d>; setting it to INSTANCE_PRESENTATION_QOS\n"),
                  xml_qos->presentation ().access_scope ().integral ()));
                dds_qos.presentation.access_scope = ::DDS::INSTANCE_PRESENTATION_QOS;
                break;
              }
            if (OpenDDS::DCPS::DCPS_debug_level > 9)
              {
                ACE_DEBUG ((LM_TRACE,
                  ACE_TEXT ("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                  ACE_TEXT ("Set presentation access scope to <%d>\n"),
                  dds_qos.presentation.access_scope));
              }
          }
        if (xml_qos->presentation ().coherent_access_p ())
          {
            dds_qos.presentation.coherent_access =
              xml_qos->presentation ().coherent_access ();

            if (OpenDDS::DCPS::DCPS_debug_level > 9)
              {
                ACE_DEBUG ((LM_TRACE,
                  ACE_TEXT ("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                  ACE_TEXT ("Set presentation coherent_access to <%d>\n"),
                  dds_qos.presentation.coherent_access));
              }
          }
        if (xml_qos->presentation ().ordered_access_p ())
          {
            dds_qos.presentation.ordered_access =
              xml_qos->presentation ().ordered_access ();

            if (OpenDDS::DCPS::DCPS_debug_level > 9)
              {
                ACE_DEBUG ((LM_TRACE,
                  ACE_TEXT ("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                  ACE_TEXT ("Set presentation ordered_access to <%d>\n"),
                  dds_qos.presentation.ordered_access));
              }
          }
      }

  if (xml_qos->partition_p ())
    {
      if (xml_qos->partition ().name_p ())
        {
          dds_qos.partition.name.length (xml_qos->partition ().name ().count_element ());
          CORBA::ULong pos = 0;
          for (::dds::stringSeq::element_const_iterator it = xml_qos->partition ().name ().begin_element ();
               it != xml_qos->partition ().name ().end_element ();
               ++it, ++pos)
            {
              dds_qos.partition.name[pos] = ::CORBA::string_dup (it->get()->c_str());

              if (OpenDDS::DCPS::DCPS_debug_level > 9)
                {
                  ACE_DEBUG ((LM_TRACE,
                    ACE_TEXT ("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                    ACE_TEXT ("New name <%C> inserted in partition at position <%u>\n"),
                    dds_qos.partition.name[pos].in (), pos));
                }
            }
        }
    }

  if (xml_qos->entity_factory_p ())
    {
      if (xml_qos->entity_factory ().autoenable_created_entities_p ())
        {
          dds_qos.entity_factory.autoenable_created_entities =
            xml_qos->entity_factory ().autoenable_created_entities ();

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set entity_factory autoenable_created_entities to <%d>\n"),
                dds_qos.entity_factory.autoenable_created_entities));
            }
        }
    }
}
