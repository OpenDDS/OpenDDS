
#include "dds/DdsDcpsInfrastructureC.h"
#include "QOS_Common.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_Topic_T<XML_QOS_TYPE, DDS_QOS_TYPE>::QOS_Topic_T ()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
QOS_Topic_T<XML_QOS_TYPE, DDS_QOS_TYPE>::~QOS_Topic_T ()
{
}

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
void
QOS_Topic_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos (DDS_QOS_TYPE& dds_qos, const XML_QOS_TYPE xml_qos)
{
  // First start parsing the QOS settings which the DataWriter and the Topic have in common
  DwTpBase::read_qos (dds_qos, xml_qos);

  // Now parse the topic-only QOS settings.

  const ::dds::topicDataQosPolicy* topic_data_if_set_ptr = xml_qos->topic_data_if_set();
  if (topic_data_if_set_ptr)
    {
      const ::XMLSchema::string<char>* value_if_set_ptr = topic_data_if_set_ptr->value_if_set();
      if (value_if_set_ptr)
        {
          QosCommon::get_binary_data(dds_qos.topic_data.value, value_if_set_ptr->c_str());
        }
    }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
