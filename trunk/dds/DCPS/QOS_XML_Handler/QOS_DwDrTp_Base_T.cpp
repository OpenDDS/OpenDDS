// $Id$

#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"
#include "dds/DCPS/debug.h"

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_DwDrTp_Base_T (void)
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_DwDrTp_Base_T (void)
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos (DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
  if (xml_qos->durability_p ())
    {
      if (xml_qos->durability().kind_p ())
        {
          QosCommon::get_durability_kind (xml_qos->durability().kind (),
                                          dds_qos.durability.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("set durability to <%d>\n"),
                dds_qos.durability.kind));
            }

        }
    }
  if (xml_qos->deadline_p ())
    {
      const std::string nsec = xml_qos->deadline().period ().nanosec ();
      const std::string sec = xml_qos->deadline().period ().sec ();

      QosCommon::get_duration (dds_qos.deadline.period,
                                sec.c_str (),
                                nsec.c_str ());

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG ((LM_TRACE,
            ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT ("Set deadline to <%d:%u>\n"),
            dds_qos.deadline.period.sec,
            dds_qos.deadline.period.nanosec));
        }
    }
  if (xml_qos->latency_budget_p ())
    {
      const std::string nsec = xml_qos->latency_budget().duration ().nanosec ();
      const std::string sec = xml_qos->latency_budget().duration ().sec ();

      QosCommon::get_duration (dds_qos.latency_budget.duration,
                                sec.c_str (),
                                nsec.c_str ());

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG ((LM_TRACE,
            ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT ("Set latency_budget to <%d:%u>\n"),
            dds_qos.latency_budget.duration.sec, \
            dds_qos.latency_budget.duration.nanosec));
        }
    }
  if (xml_qos->liveliness_p ())
    {
      if (xml_qos->liveliness ().kind_p ())
        {
          QosCommon::get_liveliness_kind (xml_qos->liveliness ().kind (),
                                          dds_qos.liveliness.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set liveliness_kind to <%d>\n"),
                dds_qos.liveliness.kind));
            }
        }
      if (xml_qos->liveliness ().lease_duration_p ())
        {
          const std::string nsec = xml_qos->liveliness ().lease_duration ().nanosec ();
          const std::string sec = xml_qos->liveliness ().lease_duration ().sec ();

          QosCommon::get_duration (dds_qos.liveliness.lease_duration,
                                    sec.c_str (),
                                    nsec.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set liveliness lease_duration to <%d:%u>\n"),
                dds_qos.liveliness.lease_duration.sec,
                dds_qos.liveliness.lease_duration.nanosec));
            }
        }
    }
  if (xml_qos->reliability_p ())
    {
      if (xml_qos->reliability ().kind_p ())
        {
          QosCommon::get_realiability_kind (xml_qos->reliability ().kind (),
                                            dds_qos.reliability.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set reliability_kind to <%d>\n"),
                dds_qos.reliability.kind));
            }
        }
      if (xml_qos->reliability ().max_blocking_time_p ())
        {
          const std::string nsec = xml_qos->reliability().max_blocking_time ().nanosec ();
          const std::string sec = xml_qos->reliability().max_blocking_time ().sec ();

          QosCommon::get_duration (dds_qos.reliability.max_blocking_time,
                                    sec.c_str (),
                                    nsec.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set reliability max_blocking_time to <%d:%u>\n"),
                dds_qos.reliability.max_blocking_time.sec,
                dds_qos.reliability.max_blocking_time.nanosec));
            }
        }
    }
  if (xml_qos->destination_order_p ())
    {
      QosCommon::get_destination_order_kind (xml_qos->destination_order ().kind (),
                                             dds_qos.destination_order.kind);

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG ((LM_TRACE,
            ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT ("Set destination_order to <%d>\n"),
            dds_qos.destination_order.kind));
        }
    }
  if (xml_qos->history_p ())
    {
      if (xml_qos->history ().kind_p ())
        {
          QosCommon::get_history_kind (xml_qos->history ().kind (),
                                       dds_qos.history.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set history to <%d>\n"),
                dds_qos.history.kind));
            }
        }
      if (xml_qos->history ().depth_p ())
        {
          dds_qos.history.depth = xml_qos->history ().depth ();

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set history depth to <%u>\n"),
                dds_qos.history.depth));
            }
        }
    }
  if (xml_qos->resource_limits_p ())
    {
      if (xml_qos->resource_limits ().max_samples_p ())
        {
          const std::string max_samples = xml_qos->resource_limits ().max_samples ();

          dds_qos.resource_limits.max_samples =
            QosCommon::get_qos_long (max_samples.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set Resource Limits max_samples to <%d>\n"),
                dds_qos.resource_limits.max_samples));
            }
        }
      if (xml_qos->resource_limits ().max_instances_p ())
        {
          const std::string max_instances = xml_qos->resource_limits ().max_instances ();

          dds_qos.resource_limits.max_instances =
            QosCommon::get_qos_long (max_instances.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set Resource Limits max_instances to <%d>\n"),
                dds_qos.resource_limits.max_instances));
            }
        }
      if (xml_qos->resource_limits ().max_samples_per_instance_p ())
        {
          const std::string max_samples_per_instance =
            xml_qos->resource_limits ().max_samples_per_instance ();

          dds_qos.resource_limits.max_samples_per_instance =
            QosCommon::get_qos_long (max_samples_per_instance.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set Resource Limits max_samples_per_instance to <%d>\n"),
                dds_qos.resource_limits.max_samples_per_instance));
            }
        }
    }
  if (xml_qos->ownership_p ())
    {
      QosCommon::get_ownership_kind (xml_qos->ownership ().kind (),
                                     dds_qos.ownership.kind);

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG ((LM_TRACE,
            ACE_TEXT ("QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT ("Set ownership to <%d>\n"),
            dds_qos.ownership.kind));
        }
    }
}
