#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"
#include "ace/ace_wchar.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_DataReader_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_DataReader_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos(DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
  // First start parsing the QOS settings which the DataWriter, the DataReader,
  // and the Topic have in common

  DwDrTpBase::read_qos(dds_qos, xml_qos);

  // Now parse the topic-only QOS settings.

//  if (xml_qos->user_data())
//    {
      // TODO: Have a good look at this.

//       const std::string value = *xml_qos->user_data()->value_if_set();
//
//       ACE_DEBUG((LM_TRACE,
//         ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
//         ACE_TEXT("Set user_data to <%C>\n"),
//         value.c_str()));
//
//       dds_qos.user_data.value =
//         *xml_qos->user_data()->value_if_set();
//     }
  const ::dds::timeBasedFilterQosPolicy* time_based_filter_if_set_ptr = xml_qos->time_based_filter_if_set();
  if (time_based_filter_if_set_ptr)
    {
      const ::dds::duration* minimum_separation_if_set_ptr = time_based_filter_if_set_ptr->minimum_separation_if_set();
      if (minimum_separation_if_set_ptr)
        {
          const ::XMLSchema::string<char>* sec_if_set_ptr = minimum_separation_if_set_ptr->sec_if_set();
          const ::XMLSchema::string<char>* nsec_if_set_ptr = minimum_separation_if_set_ptr->nanosec_if_set();

          QosCommon::get_duration(dds_qos.time_based_filter.minimum_separation,
                                  sec_if_set_ptr ? sec_if_set_ptr->c_str() : "",
                                  nsec_if_set_ptr ? nsec_if_set_ptr->c_str() : "");

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set time_based_filter minimum_separation to <%d:%u>\n"),
                dds_qos.time_based_filter.minimum_separation.sec,
                dds_qos.time_based_filter.minimum_separation.nanosec));
            }
        }
    }
  const ::dds::readerDataLifecycleQosPolicy* reader_data_lifecycle_if_set_ptr = xml_qos->reader_data_lifecycle_if_set();
  if (reader_data_lifecycle_if_set_ptr)
    {
      const ::dds::duration* autopurge_nowriter_samples_delay_if_set_ptr =
        reader_data_lifecycle_if_set_ptr->autopurge_nowriter_samples_delay_if_set();
      if (autopurge_nowriter_samples_delay_if_set_ptr)
        {
          const ::XMLSchema::string<char>* sec_if_set_ptr = autopurge_nowriter_samples_delay_if_set_ptr->sec_if_set();
          const ::XMLSchema::string<char>* nsec_if_set_ptr =
            autopurge_nowriter_samples_delay_if_set_ptr->nanosec_if_set();

          QosCommon::get_duration(dds_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay,
                                  sec_if_set_ptr ? sec_if_set_ptr->c_str() : "",
                                  nsec_if_set_ptr ? nsec_if_set_ptr->c_str() : "");

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set reader_data_lifecycle autopurge_nowriter_samples_delay to <%d:%u>\n"),
                dds_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec,
                dds_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec));
            }
        }
      const ::dds::duration* autopurge_disposed_samples_delay_if_set_ptr =
        reader_data_lifecycle_if_set_ptr->autopurge_disposed_samples_delay_if_set();
      if (autopurge_disposed_samples_delay_if_set_ptr)
        {
          const ::XMLSchema::string<char>* sec_if_set_ptr = autopurge_disposed_samples_delay_if_set_ptr->sec_if_set();
          const ::XMLSchema::string<char>* nsec_if_set_ptr =
            autopurge_disposed_samples_delay_if_set_ptr->nanosec_if_set();

          QosCommon::get_duration(dds_qos.reader_data_lifecycle.autopurge_disposed_samples_delay,
                                  sec_if_set_ptr ? sec_if_set_ptr->c_str() : "",
                                  nsec_if_set_ptr ? nsec_if_set_ptr->c_str() : "");

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set reader_data_lifecycle autopurge_disposed_samples_delay to <%d:%u>\n"),
                dds_qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec,
                dds_qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec));
            }
        }
    }
  const ::dds::typeConsistencyEnforcementQosPolicy* type_consistency_if_set_ptr = xml_qos->type_consistency_if_set();
  if (type_consistency_if_set_ptr)
    {
      const ::dds::typeConsistencyKind* kind_if_set_ptr = type_consistency_if_set_ptr->kind_if_set();
      if (kind_if_set_ptr)
        {
          QosCommon::get_type_consistency_kind(*kind_if_set_ptr,
                                               dds_qos.type_consistency.kind);

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("set type_consistency kind to <%d>\n"),
                dds_qos.type_consistency.kind));
            }
        }
      const ::XMLSchema::boolean* ignore_sequence_bounds_if_set_ptr =
        type_consistency_if_set_ptr->ignore_sequence_bounds_if_set();
      if (ignore_sequence_bounds_if_set_ptr)
        {
          dds_qos.type_consistency.ignore_sequence_bounds =
            *ignore_sequence_bounds_if_set_ptr;

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set ignore_sequence_bounds to <%d>\n"),
                dds_qos.type_consistency.ignore_sequence_bounds));
            }
        }
      const ::XMLSchema::boolean* ignore_string_bounds_if_set_ptr =
        type_consistency_if_set_ptr->ignore_string_bounds_if_set();
      if (ignore_string_bounds_if_set_ptr)
        {
          dds_qos.type_consistency.ignore_string_bounds =
            *ignore_string_bounds_if_set_ptr;

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set ignore_string_bounds to <%d>\n"),
                dds_qos.type_consistency.ignore_string_bounds));
            }
        }
      const ::XMLSchema::boolean* ignore_member_names_if_set_ptr =
        type_consistency_if_set_ptr->ignore_member_names_if_set();
      if (ignore_member_names_if_set_ptr)
        {
          dds_qos.type_consistency.ignore_member_names =
            *ignore_member_names_if_set_ptr;

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set ignore_member_names to <%d>\n"),
                dds_qos.type_consistency.ignore_member_names));
            }
        }
      const ::XMLSchema::boolean* prevent_type_widening_if_set_ptr =
        type_consistency_if_set_ptr->prevent_type_widening_if_set();
      if (prevent_type_widening_if_set_ptr)
        {
          dds_qos.type_consistency.prevent_type_widening =
            *prevent_type_widening_if_set_ptr;

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set prevent_type_widening to <%d>\n"),
                dds_qos.type_consistency.prevent_type_widening));
            }
        }
      const ::XMLSchema::boolean* force_type_validation_if_set_ptr =
        type_consistency_if_set_ptr->force_type_validation_if_set();
      if (force_type_validation_if_set_ptr)
        {
          dds_qos.type_consistency.force_type_validation =
            *force_type_validation_if_set_ptr;

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataReader_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set force_type_validation to <%d>\n"),
                dds_qos.type_consistency.force_type_validation));
            }
        }
    }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
