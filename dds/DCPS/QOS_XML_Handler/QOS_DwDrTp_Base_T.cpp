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
  if (xml_qos->durability_if_set())
    {
      if (xml_qos->durability_if_set()->kind_if_set())
        {
          QosCommon::get_durability_kind(*xml_qos->durability_if_set()->kind_if_set(),
                                         dds_qos.durability.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("set durability to <%d>\n"),
                dds_qos.durability.kind));
            }
        }
    }
  if (xml_qos->deadline_if_set())
    {
      const ACE_TString sec(xml_qos->deadline_if_set()->period_if_set()->sec_if_set()->c_str());
      const ACE_TString nsec(xml_qos->deadline_if_set()->period_if_set()->nanosec_if_set()->c_str());

      QosCommon::get_duration(dds_qos.deadline.period,
                              sec.c_str(),
                              nsec.c_str());

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG((LM_TRACE,
            ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT("Set deadline to <%d:%u>\n"),
            dds_qos.deadline.period.sec,
            dds_qos.deadline.period.nanosec));
        }
    }
  if (xml_qos->latency_budget_if_set())
    {
      const ACE_TString sec(xml_qos->latency_budget_if_set()->duration_if_set()->sec_if_set()->c_str());
      const ACE_TString nsec(xml_qos->latency_budget_if_set()->duration_if_set()->nanosec_if_set()->c_str());

      QosCommon::get_duration(dds_qos.latency_budget.duration,
                              sec.c_str(),
                              nsec.c_str());

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG((LM_TRACE,
            ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT("Set latency_budget to <%d:%u>\n"),
            dds_qos.latency_budget.duration.sec, \
            dds_qos.latency_budget.duration.nanosec));
        }
    }
  if (xml_qos->liveliness_if_set())
    {
      if (xml_qos->liveliness_if_set()->kind_if_set())
        {
          QosCommon::get_liveliness_kind(*xml_qos->liveliness_if_set()->kind_if_set(),
                                         dds_qos.liveliness.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set liveliness_kind to <%d>\n"),
                dds_qos.liveliness.kind));
            }
        }
      if (xml_qos->liveliness_if_set()->lease_duration_if_set())
        {
          const ACE_TString sec(xml_qos->liveliness_if_set()->lease_duration_if_set()->sec_if_set()->c_str());
          const ACE_TString nsec(xml_qos->liveliness_if_set()->lease_duration_if_set()->nanosec_if_set()->c_str());

          QosCommon::get_duration(dds_qos.liveliness.lease_duration,
                                  sec.c_str(),
                                  nsec.c_str());

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
  if (xml_qos->reliability_if_set())
    {
      if (xml_qos->reliability_if_set()->kind_if_set())
        {
          QosCommon::get_realiability_kind(*xml_qos->reliability_if_set()->kind_if_set(),
                                           dds_qos.reliability.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set reliability_kind to <%d>\n"),
                dds_qos.reliability.kind));
            }
        }
      if (xml_qos->reliability_if_set()->max_blocking_time_if_set())
        {
          const ACE_TString sec(xml_qos->reliability_if_set()->max_blocking_time_if_set()->sec_if_set()->c_str());
          const ACE_TString nsec(xml_qos->reliability_if_set()->max_blocking_time_if_set()->nanosec_if_set()->c_str());

          QosCommon::get_duration(dds_qos.reliability.max_blocking_time,
                                  sec.c_str(),
                                  nsec.c_str());

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
  if (xml_qos->destination_order_if_set())
    {
      QosCommon::get_destination_order_kind(*xml_qos->destination_order_if_set()->kind_if_set(),
                                            dds_qos.destination_order.kind);

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG((LM_TRACE,
            ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT("Set destination_order to <%d>\n"),
            dds_qos.destination_order.kind));
        }
    }
  if (xml_qos->history_if_set())
    {
      if (xml_qos->history_if_set()->kind_if_set())
        {
          QosCommon::get_history_kind(*xml_qos->history_if_set()->kind_if_set(),
                                      dds_qos.history.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set history to <%d>\n"),
                dds_qos.history.kind));
            }
        }
      if (xml_qos->history_if_set()->depth_if_set())
        {
          dds_qos.history.depth = static_cast<CORBA::Long>(*xml_qos->history_if_set()->depth_if_set());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set history depth to <%u>\n"),
                dds_qos.history.depth));
            }
        }
    }
  if (xml_qos->resource_limits_if_set())
    {
      if (xml_qos->resource_limits_if_set()->max_samples_if_set())
        {
          const ACE_TString max_samples(xml_qos->resource_limits_if_set()->max_samples_if_set()->c_str());

          dds_qos.resource_limits.max_samples =
            QosCommon::get_qos_long(max_samples.c_str());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set Resource Limits max_samples to <%d>\n"),
                dds_qos.resource_limits.max_samples));
            }
        }
      if (xml_qos->resource_limits_if_set()->max_instances_if_set())
        {
          const ACE_TString max_instances(xml_qos->resource_limits_if_set()->max_instances_if_set()->c_str());

          dds_qos.resource_limits.max_instances =
            QosCommon::get_qos_long(max_instances.c_str());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set Resource Limits max_instances to <%d>\n"),
                dds_qos.resource_limits.max_instances));
            }
        }
      if (xml_qos->resource_limits_if_set()->max_samples_per_instance_if_set())
        {
          const ACE_TString max_samples_per_instance(
            xml_qos->resource_limits_if_set()->max_samples_per_instance_if_set()->c_str());

          dds_qos.resource_limits.max_samples_per_instance =
            QosCommon::get_qos_long(max_samples_per_instance.c_str());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set Resource Limits max_samples_per_instance to <%d>\n"),
                dds_qos.resource_limits.max_samples_per_instance));
            }
        }
    }
  if (xml_qos->ownership_if_set())
    {
      QosCommon::get_ownership_kind(*xml_qos->ownership_if_set()->kind_if_set(),
                                    dds_qos.ownership.kind);

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG((LM_TRACE,
            ACE_TEXT("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT("Set ownership to <%d>\n"),
            dds_qos.ownership.kind));
        }
    }
  if (xml_qos->representation_if_set())
    {
      if (xml_qos->representation_if_set()->value_if_set())
        {
          const dds::dataRepresentationIdSeq::element_container_type& values =
            xml_qos->representation_if_set()->value_if_set()->element();
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
