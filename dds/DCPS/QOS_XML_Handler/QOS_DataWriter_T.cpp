
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

//  if (xml_qos->user_data())
//    {
      // TODO: Have a good look at this.

//       const std::string value = *xml_qos->user_data()->value();
//
//       ACE_DEBUG((LM_TRACE,
//         ACE_TEXT("QOS_DataWriter_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
//         ACE_TEXT("Set user_data to <%C>\n"),
//         value.c_str()));
//
//       dds_qos.user_data.value =
//         *xml_qos->user_data()->value();
//     }
 if (xml_qos->ownership_strength_p())
   {
      dds_qos.ownership_strength.value = static_cast<CORBA::Long>(xml_qos->ownership_strength().value());

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG((LM_TRACE,
            ACE_TEXT("QOS_DataWriter_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT("Set ownership_strength to <%u>\n"),
            dds_qos.ownership_strength.value));
        }
   }
  if (xml_qos->writer_data_lifecycle_p() &&
      xml_qos->writer_data_lifecycle().autodispose_unregistered_instances_p())
    {
      dds_qos.writer_data_lifecycle.autodispose_unregistered_instances =
        xml_qos->writer_data_lifecycle().autodispose_unregistered_instances();

      if (OpenDDS::DCPS::DCPS_debug_level > 9)
        {
          ACE_DEBUG((LM_TRACE,
            ACE_TEXT("QOS_DataWriter_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
            ACE_TEXT("Set autodispose_unregistered_instances to <%d>\n"),
            dds_qos.writer_data_lifecycle.autodispose_unregistered_instances));
        }
    }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
