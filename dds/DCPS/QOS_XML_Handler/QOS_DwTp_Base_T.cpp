// $Id$

#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_DwTp_Base_T (void)
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_DwTp_Base_T (void)
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos (DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
  DwDrTpBase::read_qos (dds_qos, xml_qos);

  if (xml_qos->durability_service_p ())
    {
      if (xml_qos->durability_service ().service_cleanup_delay_p ())
        {
          const std::string nsec = xml_qos->durability_service ().service_cleanup_delay ().nanosec ();
          const std::string sec = xml_qos->durability_service ().service_cleanup_delay ().sec ();

          QosCommon::get_duration (dds_qos.durability_service.service_cleanup_delay,
                                   sec.c_str (),
                                   nsec.c_str ());
          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set durability Service service_cleanup_delay to <%d:%u>\n"),
                dds_qos.durability_service.service_cleanup_delay.sec,
                dds_qos.durability_service.service_cleanup_delay.nanosec));
            }
        }
      if (xml_qos->durability_service ().history_kind_p ())
        {
          QosCommon::get_history_kind (xml_qos->durability_service ().history_kind (),
                                       dds_qos.durability_service.history_kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set durability Service history_kind to <%d>\n"),
                dds_qos.durability_service.history_kind));
            }

        }
      if (xml_qos->durability_service ().history_depth_p ())
        {
          dds_qos.durability_service.history_depth =
            xml_qos->durability_service ().history_depth ();

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set durability Service history_depth to <%u>\n"),
                dds_qos.durability_service.history_depth));
            }
        }
      if (xml_qos->durability_service ().max_samples_p ())
        {
          const std::string max_samples = xml_qos->durability_service ().max_samples ();

          dds_qos.durability_service.max_samples =
            QosCommon::get_qos_long (max_samples.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set durability service max_samples to <%d>\n"),
                dds_qos.durability_service.max_samples));
            }
        }
      if (xml_qos->durability_service ().max_instances_p ())
        {
          const std::string max_instances = xml_qos->durability_service ().max_instances ();

          dds_qos.durability_service.max_instances =
            QosCommon::get_qos_long (max_instances.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set durability service max_instances to <%d>\n"),
                dds_qos.durability_service.max_instances));
            }
        }
      if (xml_qos->durability_service ().max_samples_per_instance_p ())
        {
          const std::string max_samples_per_instance =
            xml_qos->durability_service ().max_samples_per_instance ();

          dds_qos.durability_service.max_samples_per_instance =
            QosCommon::get_qos_long (max_samples_per_instance.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set durability service max_samples_per_instance to <%u>\n"),
                dds_qos.durability_service.max_samples_per_instance));
            }
        }
    }
  if (xml_qos->transport_priority_p ())
    {
      dds_qos.transport_priority.value =
        xml_qos->transport_priority ().value ();

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG ((LM_TRACE,
            ACE_TEXT ("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT ("Set transport_priority to <%u>\n"),
            dds_qos.transport_priority.value));
        }
    }
  if (xml_qos->lifespan_p ())
    {
      if (xml_qos->lifespan ().duration_p ())
        {
          const std::string nsec = xml_qos->lifespan ().duration ().nanosec ();
          const std::string sec = xml_qos->lifespan ().duration ().sec ();

          QosCommon::get_duration (dds_qos.lifespan.duration,
                                   sec.c_str (),
                                   nsec.c_str ());
          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DwTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set Lifespan duration to <%d:%u>\n"),
                dds_qos.lifespan.duration.sec, dds_qos.lifespan.duration.nanosec));
            }
        }
    }
}
