// $Id$

#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_DataReader_T (void)
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_DataReader_T (void)
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos (DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
  // First start parsing the QOS settings which the DataWriter, the DataReader,
  // and the Topic have in common

  DwDrTpBase::read_qos (dds_qos, xml_qos);

  // Now parse the topic-only QOS settings.

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
  if (xml_qos->time_based_filter_p ())
    {
      if (xml_qos->time_based_filter ().minimum_separation_p ())
        {
          const std::string nsec = xml_qos->time_based_filter ().minimum_separation ().nanosec ();
          const std::string sec = xml_qos->time_based_filter ().minimum_separation ().sec ();

          QosCommon::get_duration (dds_qos.time_based_filter.minimum_separation,
                                    sec.c_str (),
                                    nsec.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set time_based_filter minimum_separation to <%d:%u>\n"),
                dds_qos.time_based_filter.minimum_separation.sec,
                dds_qos.time_based_filter.minimum_separation.nanosec));
            }
        }
    }
  if (xml_qos->reader_data_lifecycle_p ())
    {
      if (xml_qos->reader_data_lifecycle ().autopurge_nowriter_samples_delay_p ())
        {
          const std::string nsec = xml_qos->reader_data_lifecycle ().autopurge_nowriter_samples_delay ().nanosec ();
          const std::string sec = xml_qos->reader_data_lifecycle ().autopurge_nowriter_samples_delay ().sec ();

          QosCommon::get_duration (dds_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay,
                                    sec.c_str (),
                                    nsec.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set reader_data_lifecycle autopurge_nowriter_samples_delay to <%d:%u>\n"),
                dds_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec,
                dds_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec));
            }
        }
      if (xml_qos->reader_data_lifecycle ().autopurge_disposed_samples_delay_p ())
        {
          const std::string nsec = xml_qos->reader_data_lifecycle ().autopurge_disposed_samples_delay ().nanosec ();
          const std::string sec = xml_qos->reader_data_lifecycle ().autopurge_disposed_samples_delay ().sec ();

          QosCommon::get_duration (dds_qos.reader_data_lifecycle.autopurge_disposed_samples_delay,
                                    sec.c_str (),
                                    nsec.c_str ());

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT ("Set reader_data_lifecycle autopurge_disposed_samples_delay to <%d:%u>\n"),
                dds_qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec,
                dds_qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec));
            }
        }
    }
}
