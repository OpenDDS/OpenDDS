#include "TopicQos_Handler.h"
#include "QOS_Topic_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

bool
TopicQos_Handler::get_topic_qos(DDS::TopicQos& tp_qos,
                                const dds::qosProfile * profile,
                                const ACE_TCHAR * name)
{
  if (name)
    {
//       // find the correct DataReader_qos
//       for (dds::qosProfile::DataReader_qos_iterator dw_it = profile->begin_DataReader_qos();
//           dw_it != profile->end_DataReader_qos();
//           ++dw_it)
//         {
//           if (ACE_OS::strcmp(dw_name.c_str(), name) == 0)
//             {
//               return get_DataReader_qos(dw_qos, *&datawriter_qos);
//             }
//         }
    }
  else
    {
      if (profile->topic_qos().empty())
        {
          if (OpenDDS::DCPS::DCPS_debug_level > 7)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("TopicQos_Handler::get_topic_qos - ")
                ACE_TEXT("No Topic QOS available in profile <%s>\n"),
                profile->name().c_str()));
            }
          return true;
        }
      // get the first DataReader in the XML
      const dds::topicQos& topic_qos = profile->topic_qos().front();
      return TopicQos_Handler::get_topic_qos(tp_qos, &topic_qos);
    }

  ACE_ERROR((LM_ERROR,
    ACE_TEXT("(%P|%t) ERROR: TopicQos_Handler::get_topic_qos - ")
    ACE_TEXT("Unable to find TopicQos <%s>\n"),
    name));
  return false;
}

bool
TopicQos_Handler::get_topic_qos(DDS::TopicQos& tp_qos,
                                const dds::topicQos * tp)
{
  typedef QOS_Topic_T<const dds::topicQos*, DDS::TopicQos> tp_type;
  tp_type topic_qos_handler;
  topic_qos_handler.read_qos(tp_qos, tp);

  return true;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
