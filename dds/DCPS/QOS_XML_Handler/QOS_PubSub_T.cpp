
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
    const ::dds::groupDataQosPolicy* group_data_if_set_ptr = xml_qos->group_data_if_set();
    if (group_data_if_set_ptr)
      {
        const ::XMLSchema::string<char>* value_if_set_ptr = group_data_if_set_ptr->value_if_set();
        if (value_if_set_ptr)
          {
            QosCommon::get_binary_data(dds_qos.group_data.value, value_if_set_ptr->c_str());
          }
      }

    const ::dds::presentationQosPolicy* presentation_if_set_ptr = xml_qos->presentation_if_set();
    if (presentation_if_set_ptr)
      {
        const ::dds::presentationAccessScopeKind* access_scope_if_set_ptr =
          presentation_if_set_ptr->access_scope_if_set();
        if (access_scope_if_set_ptr)
          {
            switch (access_scope_if_set_ptr->integral())
              {
              case ::dds::presentationAccessScopeKind::INSTANCE_PRESENTATION_QOS:
                dds_qos.presentation.access_scope = ::DDS::INSTANCE_PRESENTATION_QOS;
                break;
              case ::dds::presentationAccessScopeKind::TOPIC_PRESENTATION_QOS:
                dds_qos.presentation.access_scope = ::DDS::TOPIC_PRESENTATION_QOS;
                break;
              case ::dds::presentationAccessScopeKind::GROUP_PRESENTATION_QOS:
                dds_qos.presentation.access_scope = ::DDS::GROUP_PRESENTATION_QOS;
                break;
              default:
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: QOS_PubSub_T::read_qos - ")
                  ACE_TEXT("Unknown presentation access scope found <%d>; setting it to INSTANCE_PRESENTATION_QOS\n"),
                  access_scope_if_set_ptr->integral()));
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
        const ::XMLSchema::boolean* coherent_access_if_set_ptr = presentation_if_set_ptr->coherent_access_if_set();
        if (coherent_access_if_set_ptr)
          {
            dds_qos.presentation.coherent_access =
              *coherent_access_if_set_ptr;

            if (OpenDDS::DCPS::DCPS_debug_level > 9)
              {
                ACE_DEBUG((LM_TRACE,
                  ACE_TEXT("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                  ACE_TEXT("Set presentation coherent_access to <%d>\n"),
                  dds_qos.presentation.coherent_access));
              }
          }
        const ::XMLSchema::boolean* ordered_access_if_set_ptr = presentation_if_set_ptr->ordered_access_if_set();
        if (ordered_access_if_set_ptr)
          {
            dds_qos.presentation.ordered_access =
              *ordered_access_if_set_ptr;

            if (OpenDDS::DCPS::DCPS_debug_level > 9)
              {
                ACE_DEBUG((LM_TRACE,
                  ACE_TEXT("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                  ACE_TEXT("Set presentation ordered_access to <%d>\n"),
                  dds_qos.presentation.ordered_access));
              }
          }
      }

  const ::dds::partitionQosPolicy* partition_if_set_ptr = xml_qos->partition_if_set();
  if (partition_if_set_ptr)
    {
      const ::dds::stringSeq* name_if_set_ptr = partition_if_set_ptr->name_if_set();
      if (name_if_set_ptr)
        {
          const dds::stringSeq::element_container_type& names =
            name_if_set_ptr->element();
          dds_qos.partition.name.length(static_cast<CORBA::ULong>(names.size()));
          CORBA::ULong pos = 0;
          for (const XMLSchema::string<char>& name : names)
            {
              dds_qos.partition.name[pos] = ::CORBA::string_dup(name.c_str());

              if (OpenDDS::DCPS::DCPS_debug_level > 9)
                {
                  ACE_DEBUG((LM_TRACE,
                    ACE_TEXT("QOS_PubSub_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                    ACE_TEXT("New name <%C> inserted in partition at position <%u>\n"),
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
