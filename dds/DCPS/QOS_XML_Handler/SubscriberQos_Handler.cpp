
#include "SubscriberQos_Handler.h"
#include "QOS_PubSub_T.h"
#include <memory>
#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

bool
SubscriberQos_Handler::get_subscriber_qos(DDS::SubscriberQos& sub_qos,
                                          const dds::qosProfile * profile,
                                          std::string_view name)
{
  if (!name.empty())
    {
//       // find the correct DataReader_qos
//       for (dds::qosProfile::DataReader_qos_iterator dw_it = profile->begin_DataReader_qos();
//           dw_it != profile->end_DataReader_qos();
//           ++dw_it)
//         {
//           if (ACE_OS::strcmp(dw_name.c_str(), name) == 0)
//             {
//               return get_subscriber_qos(sub_qos, std::addressof(subscriber_qos));
//             }
//         }
    }
  else
    {
      if (profile->subscriber_qos().empty())
        {
          if (OpenDDS::DCPS::DCPS_debug_level > 7)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("SubscriberQos_Handler::get_subscriber_qos - ")
                ACE_TEXT("No Subscriber QOS available in profile <%s>\n"),
                profile->name().c_str()));
            }
          return true;
        }
      // get the first DataReader in the XML
      const dds::subscriberQos& subscriber_qos = profile->subscriber_qos().front();
      return SubscriberQos_Handler::get_subscriber_qos(sub_qos, std::addressof(subscriber_qos));
    }

  const std::string name_string (name);
  ACE_ERROR((LM_ERROR,
    ACE_TEXT("(%P|%t) ERROR: SubscriberQos_Handler::get_subscriber_qos - ")
    ACE_TEXT("Unable to find SubscriberQos <%C>\n"),
    name_string.c_str()));
  return false;
}

bool
SubscriberQos_Handler::get_subscriber_qos(DDS::SubscriberQos& sub_qos,
                                          const dds::subscriberQos * sub)
{
  typedef QOS_PubSub_T<const dds::subscriberQos*, DDS::SubscriberQos> sub_type;
  sub_type subscriber_qos_handler;
  subscriber_qos_handler.read_qos(sub_qos, sub);

  return true;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
