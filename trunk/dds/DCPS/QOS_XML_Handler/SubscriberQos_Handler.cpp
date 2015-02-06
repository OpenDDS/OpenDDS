// $Id$

#include "SubscriberQos_Handler.h"
#include "QOS_PubSub_T.h"

bool
SubscriberQos_Handler::get_subscriber_qos (DDS::SubscriberQos& sub_qos,
                                      dds::qosProfile * profile,
                                      const ACE_TCHAR * name)
{
  if (name)
    {
//       // find the correct DataReader_qos
//       for (dds::qosProfile::DataReader_qos_iterator dw_it = profile->begin_DataReader_qos ();
//           dw_it != profile->end_DataReader_qos();
//           ++dw_it)
//         {
//           if (ACE_OS::strcmp (dw_name.c_str (), name) == 0)
//             {
//               return get_DataReader_qos (dw_qos, *dw_it->get());
//             }
//         }
    }
  else
    {
      if (profile->count_subscriber_qos () == 0)
        {
          if (OpenDDS::DCPS::DCPS_debug_level > 7)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("SubscriberQos_Handler::get_subscriber_qos - ")
                ACE_TEXT ("No Subscriber QOS available in profile <%C>\n"),
                profile->name ().c_str ()));
            }
          return true;
        }
      // get the first DataReader in the XML
      dds::qosProfile::subscriber_qos_iterator sub_it = profile->begin_subscriber_qos ();
      return SubscriberQos_Handler::get_subscriber_qos (sub_qos, sub_it->get ());
    }

  ACE_ERROR ((LM_DEBUG,
    ACE_TEXT ("SubscriberQos_Handler::get_subscriber_qos - ")
    ACE_TEXT ("Unable to find SubscriberQos <%C>\n"),
    name));
  return false;
}

bool
SubscriberQos_Handler::get_subscriber_qos (DDS::SubscriberQos& sub_qos,
                                         dds::subscriberQos * sub)
{
  typedef QOS_PubSub_T<dds::subscriberQos*, DDS::SubscriberQos> sub_type;
  sub_type subscriber_qos_handler;
  subscriber_qos_handler.read_qos (sub_qos, sub);

  return true;
}
