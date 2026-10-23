#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_DwDrTp_Base_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_DwDrTp_Base_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos(DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
  const ::dds::durabilityQosPolicy* durability_if_set_ptr = xml_qos->durability_if_set();
  if (durability_if_set_ptr)
    {
      const ::dds::durabilityKind* kind_if_set_ptr = durability_if_set_ptr->kind_if_set();
      if (kind_if_set_ptr)
        {
          QosCommon::get_durability_kind(*kind_if_set_ptr, dds_qos.durability.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("set durability to <%d>\n"),
                dds_qos.durability.kind));
            }
        }
    }
  const ::dds::deadlineQosPolicy* deadline_if_set_ptr = xml_qos->deadline_if_set();
  if (deadline_if_set_ptr)
    {
      const ::dds::duration* period_if_set_ptr = deadline_if_set_ptr->period_if_set();
      if (period_if_set_ptr)
        {
          const ::XMLSchema::string<char>* sec_if_set_ptr = period_if_set_ptr->sec_if_set();
          const ::XMLSchema::string<char>* nsec_if_set_ptr = period_if_set_ptr->nanosec_if_set();

          QosCommon::get_duration(dds_qos.deadline.period,
                                  sec_if_set_ptr ? sec_if_set_ptr->c_str() : "",
                                  nsec_if_set_ptr ? nsec_if_set_ptr->c_str() : "");

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set deadline to <%d:%u>\n"),
                dds_qos.deadline.period.sec,
                dds_qos.deadline.period.nanosec));
            }
        }
    }
  const ::dds::latencyBudgetQosPolicy* latency_budget_if_set_ptr = xml_qos->latency_budget_if_set();
  if (latency_budget_if_set_ptr)
    {
      const ::dds::duration* duration_if_set_ptr = latency_budget_if_set_ptr->duration_if_set();
      if (duration_if_set_ptr)
        {
          const ::XMLSchema::string<char>* sec_if_set_ptr = duration_if_set_ptr->sec_if_set();
          const ::XMLSchema::string<char>* nsec_if_set_ptr = duration_if_set_ptr->nanosec_if_set();

          QosCommon::get_duration(dds_qos.latency_budget.duration,
                                  sec_if_set_ptr ? sec_if_set_ptr->c_str() : "",
                                  nsec_if_set_ptr ? nsec_if_set_ptr->c_str() : "");

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set latency_budget to <%d:%u>\n"),
                dds_qos.latency_budget.duration.sec,
                dds_qos.latency_budget.duration.nanosec));
            }
        }
    }
  const ::dds::livelinessQosPolicy* liveliness_if_set_ptr = xml_qos->liveliness_if_set();
  if (liveliness_if_set_ptr)
    {
      const ::dds::livelinessKind* kind_if_set_ptr = liveliness_if_set_ptr->kind_if_set();
      if (kind_if_set_ptr)
        {
          QosCommon::get_liveliness_kind(*kind_if_set_ptr, dds_qos.liveliness.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set liveliness_kind to <%d>\n"),
                dds_qos.liveliness.kind));
            }
        }
      const ::dds::duration* lease_duration_if_set_ptr = liveliness_if_set_ptr->lease_duration_if_set();
      if (lease_duration_if_set_ptr)
        {
          const ::XMLSchema::string<char>* sec_if_set_ptr = lease_duration_if_set_ptr->sec_if_set();
          const ::XMLSchema::string<char>* nsec_if_set_ptr = lease_duration_if_set_ptr->nanosec_if_set();

          QosCommon::get_duration(dds_qos.liveliness.lease_duration,
                                  sec_if_set_ptr ? sec_if_set_ptr->c_str() : "",
                                  nsec_if_set_ptr ? nsec_if_set_ptr->c_str() : "");

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set liveliness lease_duration to <%d:%u>\n"),
                dds_qos.liveliness.lease_duration.sec,
                dds_qos.liveliness.lease_duration.nanosec));
            }
        }
    }
  const ::dds::reliabilityQosPolicy* reliability_if_set_ptr = xml_qos->reliability_if_set();
  if (reliability_if_set_ptr)
    {
      const ::dds::reliabilityKind* kind_if_set_ptr = reliability_if_set_ptr->kind_if_set();
      if (kind_if_set_ptr)
        {
          QosCommon::get_realiability_kind(*kind_if_set_ptr, dds_qos.reliability.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set reliability_kind to <%d>\n"),
                dds_qos.reliability.kind));
            }
        }
      const ::dds::duration* max_blocking_time_if_set_ptr = reliability_if_set_ptr->max_blocking_time_if_set();
      if (max_blocking_time_if_set_ptr)
        {
          const ::XMLSchema::string<char>* sec_if_set_ptr = max_blocking_time_if_set_ptr->sec_if_set();
          const ::XMLSchema::string<char>* nsec_if_set_ptr = max_blocking_time_if_set_ptr->nanosec_if_set();

          QosCommon::get_duration(dds_qos.reliability.max_blocking_time,
                                  sec_if_set_ptr ? sec_if_set_ptr->c_str() : "",
                                  nsec_if_set_ptr ? nsec_if_set_ptr->c_str() : "");

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set reliability max_blocking_time to <%d:%u>\n"),
                dds_qos.reliability.max_blocking_time.sec,
                dds_qos.reliability.max_blocking_time.nanosec));
            }
        }
    }
  const ::dds::destinationOrderQosPolicy* destination_order_if_set_ptr = xml_qos->destination_order_if_set();
  if (destination_order_if_set_ptr)
    {
      const ::dds::destinationOrderKind* kind_if_set_ptr = destination_order_if_set_ptr->kind_if_set();
      if (kind_if_set_ptr)
        {
          QosCommon::get_destination_order_kind(*kind_if_set_ptr, dds_qos.destination_order.kind);
        }

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG((LM_TRACE,
            ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT("Set destination_order to <%d>\n"),
            dds_qos.destination_order.kind));
        }
    }
  const ::dds::historyQosPolicy* history_if_set_ptr = xml_qos->history_if_set();
  if (history_if_set_ptr)
    {
      const ::dds::historyKind* kind_if_set_ptr = history_if_set_ptr->kind_if_set();
      if (kind_if_set_ptr)
        {
          QosCommon::get_history_kind(*kind_if_set_ptr, dds_qos.history.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set history to <%d>\n"),
                dds_qos.history.kind));
            }
        }
      const ::XMLSchema::positiveInteger* depth_if_set_ptr = history_if_set_ptr->depth_if_set();
      if (depth_if_set_ptr)
        {
          dds_qos.history.depth = static_cast<CORBA::Long>(*depth_if_set_ptr);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set history depth to <%u>\n"),
                dds_qos.history.depth));
            }
        }
    }
  const ::dds::resourceLimitsQosPolicy* resource_limits_if_set_ptr = xml_qos->resource_limits_if_set();
  if (resource_limits_if_set_ptr)
    {
      const ::XMLSchema::string<char>* max_samples_if_set_ptr = resource_limits_if_set_ptr->max_samples_if_set();
      if (max_samples_if_set_ptr)
        {
          dds_qos.resource_limits.max_samples = QosCommon::get_qos_long(*max_samples_if_set_ptr);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set Resource Limits max_samples to <%d>\n"),
                dds_qos.resource_limits.max_samples));
            }
        }
      const ::XMLSchema::string<char>* max_instances_if_set_ptr = resource_limits_if_set_ptr->max_instances_if_set();
      if (max_instances_if_set_ptr)
        {
          dds_qos.resource_limits.max_instances = QosCommon::get_qos_long(*max_instances_if_set_ptr);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set Resource Limits max_instances to <%d>\n"),
                dds_qos.resource_limits.max_instances));
            }
        }
      const ::XMLSchema::string<char>* max_samples_per_instance_if_set_ptr =
        resource_limits_if_set_ptr->max_samples_per_instance_if_set();
      if (max_samples_per_instance_if_set_ptr)
        {
          dds_qos.resource_limits.max_samples_per_instance =
            QosCommon::get_qos_long(*max_samples_per_instance_if_set_ptr);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set Resource Limits max_samples_per_instance to <%d>\n"),
                dds_qos.resource_limits.max_samples_per_instance));
            }
        }
    }
  const ::dds::ownershipQosPolicy* ownership_if_set_ptr = xml_qos->ownership_if_set();
  if (ownership_if_set_ptr)
    {
      const ::dds::ownershipKind* kind_if_set_ptr = ownership_if_set_ptr->kind_if_set();
      if (kind_if_set_ptr)
        {
          QosCommon::get_ownership_kind(*kind_if_set_ptr, dds_qos.ownership.kind);
        }

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG((LM_TRACE,
            ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT("Set ownership to <%d>\n"),
            dds_qos.ownership.kind));
        }
    }
  const ::dds::dataRepresentationQosPolicy* representation_if_set_ptr = xml_qos->representation_if_set();
  if (representation_if_set_ptr)
    {
      const ::dds::dataRepresentationIdSeq* value_if_set_ptr = representation_if_set_ptr->value_if_set();
      if (value_if_set_ptr)
        {
          const dds::dataRepresentationIdSeq::element_container_type& values =
            value_if_set_ptr->element();
          dds_qos.representation.value.length(static_cast<CORBA::ULong>(values.size()));
          CORBA::ULong pos = 0;
          for (const ::dds::dataRepresentationIdKind& value : values)
            {
              QosCommon::get_data_presentation_id_kind(value, dds_qos.representation.value[pos]);

              if (OpenDDS::DCPS::DCPS_debug_level > 9)
                {
                  ACE_DEBUG((LM_TRACE,
                    ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                    ACE_TEXT("Data representation <%d> inserted in representation at position <%u>\n"),
                    dds_qos.representation.value[pos], pos));
                }
              ++pos;
            }
        }
    }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
