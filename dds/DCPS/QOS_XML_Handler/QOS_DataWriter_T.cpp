
#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DataWriter_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_DataWriter_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_DataWriter_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_DataWriter_T()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_DataWriter_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos(DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
  // First start parsing the QOS settings which the DataWriter, the DataReader,
  // and the Topic have in common

  DwTpBase::read_qos(dds_qos, xml_qos);

  // Now parse the topic-only QOS settings.

  const ::dds::userDataQosPolicy* user_data_if_set_ptr = xml_qos->user_data_if_set();
  if (user_data_if_set_ptr) {
    const ::XMLSchema::string<char>* value_if_set_ptr = user_data_if_set_ptr->value_if_set();
    if (value_if_set_ptr) {
      QosCommon::get_binary_data(dds_qos.user_data.value, value_if_set_ptr->c_str());
    }
  }
  const ::dds::ownershipStrengthQosPolicy* ownership_strength_if_set_ptr = xml_qos->ownership_strength_if_set();
  if (ownership_strength_if_set_ptr)
    {
      const ::XMLSchema::nonNegativeInteger* value_if_set_ptr = ownership_strength_if_set_ptr->value_if_set();
      if (value_if_set_ptr)
        {
          dds_qos.ownership_strength.value = QosCommon::get_corba_long(static_cast<std::int64_t>(*value_if_set_ptr));

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataWriter_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set ownership_strength to <%u>\n"),
                dds_qos.ownership_strength.value));
            }
        }
    }
  const ::dds::writerDataLifecycleQosPolicy* writer_data_lifecycle_if_set_ptr = xml_qos->writer_data_lifecycle_if_set();
  if (writer_data_lifecycle_if_set_ptr)
    {
      const ::XMLSchema::boolean* autodispose_if_set_ptr =
        writer_data_lifecycle_if_set_ptr->autodispose_unregistered_instances_if_set();
      if (autodispose_if_set_ptr)
        {
          dds_qos.writer_data_lifecycle.autodispose_unregistered_instances = *autodispose_if_set_ptr;

          if (OpenDDS::DCPS::DCPS_debug_level > 9)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("QOS_DataWriter_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
                ACE_TEXT("Set autodispose_unregistered_instances to <%d>\n"),
                dds_qos.writer_data_lifecycle.autodispose_unregistered_instances));
            }
        }
    }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
