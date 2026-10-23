
#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_DwTp_Base_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_DwTp_Base_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos(DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
  DwDrTpBase::read_qos(dds_qos, xml_qos);

  const ::dds::durabilityServiceQosPolicy* durability_service_if_set_ptr = xml_qos->durability_service_if_set();
  if (durability_service_if_set_ptr)
    {
      const ::dds::duration* service_cleanup_delay_if_set_ptr =
        durability_service_if_set_ptr->service_cleanup_delay_if_set();
      if (service_cleanup_delay_if_set_ptr)
        {
          const ::XMLSchema::string<char>* sec_if_set_ptr = service_cleanup_delay_if_set_ptr->sec_if_set();
          const ::XMLSchema::string<char>* nsec_if_set_ptr = service_cleanup_delay_if_set_ptr->nanosec_if_set();

          QosCommon::get_duration(dds_qos.durability_service.service_cleanup_delay,
                                  sec_if_set_ptr ? sec_if_set_ptr->c_str() : "",
                                  nsec_if_set_ptr ? nsec_if_set_ptr->c_str() : "");
          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set durability Service service_cleanup_delay to <%d:%u>\n"),
                dds_qos.durability_service.service_cleanup_delay.sec,
                dds_qos.durability_service.service_cleanup_delay.nanosec));
            }
        }
      const ::dds::historyKind* history_kind_if_set_ptr = durability_service_if_set_ptr->history_kind_if_set();
      if (history_kind_if_set_ptr)
        {
          QosCommon::get_history_kind(*history_kind_if_set_ptr,
                                      dds_qos.durability_service.history_kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set durability Service history_kind to <%d>\n"),
                dds_qos.durability_service.history_kind));
            }

        }
      const ::XMLSchema::positiveInteger* history_depth_if_set_ptr =
        durability_service_if_set_ptr->history_depth_if_set();
      if (history_depth_if_set_ptr)
        {
          dds_qos.durability_service.history_depth =
            static_cast<CORBA::Long>(*history_depth_if_set_ptr);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set durability Service history_depth to <%u>\n"),
                dds_qos.durability_service.history_depth));
            }
        }
      const ::XMLSchema::string<char>* max_samples_if_set_ptr = durability_service_if_set_ptr->max_samples_if_set();
      if (max_samples_if_set_ptr)
        {
          dds_qos.durability_service.max_samples = QosCommon::get_qos_long(*max_samples_if_set_ptr);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set durability service max_samples to <%d>\n"),
                dds_qos.durability_service.max_samples));
            }
        }
      const ::XMLSchema::string<char>* max_instances_if_set_ptr = durability_service_if_set_ptr->max_instances_if_set();
      if (max_instances_if_set_ptr)
        {
          dds_qos.durability_service.max_instances = QosCommon::get_qos_long(*max_instances_if_set_ptr);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set durability service max_instances to <%d>\n"),
                dds_qos.durability_service.max_instances));
            }
        }
      const ::XMLSchema::string<char>* max_samples_per_instance_if_set_ptr =
        durability_service_if_set_ptr->max_samples_per_instance_if_set();
      if (max_samples_per_instance_if_set_ptr)
        {
          dds_qos.durability_service.max_samples_per_instance =
            QosCommon::get_qos_long(*max_samples_per_instance_if_set_ptr);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set durability service max_samples_per_instance to <%u>\n"),
                dds_qos.durability_service.max_samples_per_instance));
            }
        }
    }
  const ::dds::transportPriorityQosPolicy* transport_priority_if_set_ptr = xml_qos->transport_priority_if_set();
  if (transport_priority_if_set_ptr)
    {
      const ::XMLSchema::nonNegativeInteger* value_if_set_ptr = transport_priority_if_set_ptr->value_if_set();
      if (value_if_set_ptr)
        {
          dds_qos.transport_priority.value = static_cast<CORBA::Long>(*value_if_set_ptr);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set transport_priority to <%u>\n"),
                dds_qos.transport_priority.value));
            }
        }
    }
  const ::dds::lifespanQosPolicy* lifespan_if_set_ptr = xml_qos->lifespan_if_set();
  if (lifespan_if_set_ptr)
    {
      const ::dds::duration* duration_if_set_ptr = lifespan_if_set_ptr->duration_if_set();
      if (duration_if_set_ptr)
        {
          const ::XMLSchema::string<char>* sec_if_set_ptr = duration_if_set_ptr->sec_if_set();
          const ::XMLSchema::string<char>* nsec_if_set_ptr = duration_if_set_ptr->nanosec_if_set();

          QosCommon::get_duration(dds_qos.lifespan.duration,
                                  sec_if_set_ptr ? sec_if_set_ptr->c_str() : "",
                                  nsec_if_set_ptr ? nsec_if_set_ptr->c_str() : "");
          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set Lifespan duration to <%d:%u>\n"),
                dds_qos.lifespan.duration.sec, dds_qos.lifespan.duration.nanosec));
            }
        }
    }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
