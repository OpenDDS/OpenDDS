
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
  // First start parsing the QOS settings which the DataWriter, the DataReader,
  // and the Topic have in common
  DwDrTpBase::read_qos (dds_qos, xml_qos);

  // Now parse the topic-only QOS settings.

//  if (xml_qos->topic_data ())
//    {
      // TODO: Have a good look at this.

//       const std::string value = *xml_qos->topic_data ()->value ();
//
//       ACE_DEBUG ((LM_TRACE,
//         ACE_TEXT ("QOS_Topic_T<XML_QOS_TYPE, DDS_QOS_TYPE>::read_qos - ")
//         ACE_TEXT ("Set topic_data to <%C>\n"),
//         value.c_str ()));
//
//       dds_qos.topic_data.value =
//         *xml_qos->topic_data ()->value ();
//     }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
