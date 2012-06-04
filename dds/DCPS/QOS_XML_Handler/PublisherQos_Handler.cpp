// $Id$
#include "PublisherQos_Handler.h"
#include "QOS_PubSub_T.h"

bool
PublisherQos_Handler::get_publisher_qos (DDS::PublisherQos& pub_qos,
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
      if (profile->count_publisher_qos () == 0)
        {
          if (OpenDDS::DCPS::DCPS_debug_level > 7)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("PublisherQos_Handler::get_publisher_qos - ")
                ACE_TEXT ("No Publisher QOS available in profile <%C>\n"),
                profile->name ().c_str ()));
            }
          return true;
        }
      // get the first DataReader in the XML
      dds::qosProfile::publisher_qos_iterator pub_it = profile->begin_publisher_qos ();
      return PublisherQos_Handler::get_publisher_qos (pub_qos, pub_it->get ());
    }

  ACE_ERROR ((LM_DEBUG,
    ACE_TEXT ("PublisherQos_Handler::get_publisher_qos - ")
    ACE_TEXT ("Unable to find PublisherQos <%C>\n"),
    name));
  return false;
}

bool
PublisherQos_Handler::get_publisher_qos (DDS::PublisherQos& pub_qos,
                                         dds::publisherQos * pub)
{
  typedef QOS_PubSub_T<dds::publisherQos*, DDS::PublisherQos> pub_type;
  pub_type publisher_qos_handler;
  publisher_qos_handler.read_qos (pub_qos, pub);

  return true;
}
