
#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_PubSub_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_PubSub_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos(DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
//  if (xml_qos->group_data())
//    {
      // TODO: Have a good look at this.

//       const std::string value = *xml_qos->group_data()->value_if_set();
//
//       ACE_DEBUG((LM_TRACE,
//         ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
//         ACE_TEXT("Set group_data to <%C>\n"),
//         value.c_str()));
//
//       dds_qos.group_data.value =
//         *xml_qos->group_data()->value_if_set();
//     }

    if (xml_qos->presentation_if_set())
      {
        if (xml_qos->presentation_if_set()->access_scope_if_set())
          {
            switch (xml_qos->presentation_if_set()->access_scope_if_set()->integral())
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
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: QOS_PubSub_T::read_qos - ")
                  ACE_TEXT("Unknown presentation access scope found <%d>; setting it to INSTANCE_PRESENTATION_QOS\n"),
                  xml_qos->presentation_if_set()->access_scope_if_set()->integral()));
                dds_qos.presentation.access_scope = ::DDS::INSTANCE_PRESENTATION_QOS;
                break;
              }
            if (OpenDDS::DCPS::DCPS_debug_level > 9)
              {
                ACE_DEBUG((LM_TRACE,
                  ACE_TEXT("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                  ACE_TEXT("Set presentation access scope to <%d>\n"),
                  dds_qos.presentation.access_scope));
              }
          }
        if (xml_qos->presentation_if_set()->coherent_access_if_set())
          {
            dds_qos.presentation.coherent_access =
              *xml_qos->presentation_if_set()->coherent_access_if_set();

            if (OpenDDS::DCPS::DCPS_debug_level > 9)
              {
                ACE_DEBUG((LM_TRACE,
                  ACE_TEXT("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                  ACE_TEXT("Set presentation coherent_access to <%d>\n"),
                  dds_qos.presentation.coherent_access));
              }
          }
        if (xml_qos->presentation_if_set()->ordered_access_if_set())
          {
            dds_qos.presentation.ordered_access =
              *xml_qos->presentation_if_set()->ordered_access_if_set();

            if (OpenDDS::DCPS::DCPS_debug_level > 9)
              {
                ACE_DEBUG((LM_TRACE,
                  ACE_TEXT("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                  ACE_TEXT("Set presentation ordered_access to <%d>\n"),
                  dds_qos.presentation.ordered_access));
              }
          }
      }

  if (xml_qos->partition_if_set())
    {
      if (xml_qos->partition_if_set()->name_if_set())
        {
          const dds::stringSeq::element_container_type& names =
            xml_qos->partition_if_set()->name_if_set()->element();
          dds_qos.partition.name.length(static_cast<CORBA::ULong>(names.size()));
          CORBA::ULong pos = 0;
          for (const XMLSchema::string<char>& name : names)
            {
              dds_qos.partition.name[pos] = ::CORBA::string_dup(name.c_str());

              if (OpenDDS::DCPS::DCPS_debug_level > 9)
                {
                  ACE_DEBUG((LM_TRACE,
                    ACE_TEXT("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                    ACE_TEXT("New name <%s> inserted in partition at position <%u>\n"),
                    dds_qos.partition.name[pos].in(), pos));
                }
              ++pos;
            }
        }
    }

  const ::dds::entityFactoryQosPolicy* entity_factory_if_set_ptr = xml_qos->entity_factory_if_set();
  if (entity_factory_if_set_ptr)
    {
      const ::XMLSchema::boolean* autoenable_ptr = entity_factory_if_set_ptr->autoenable_created_entities_if_set();
      if (autoenable_ptr)
        {
          dds_qos.entity_factory.autoenable_created_entities = *autoenable_ptr;

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set entity_factory autoenable_created_entities to <%d>\n"),
                dds_qos.entity_factory.autoenable_created_entities));
            }
        }
    }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
