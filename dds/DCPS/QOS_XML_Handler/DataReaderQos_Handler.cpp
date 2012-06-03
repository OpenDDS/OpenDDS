// $Id$
#include "DataReaderQos_Handler.h"
#include "QOS_DataReader_T.h"
#include "dds/DCPS/debug.h"

bool
DataReaderQos_Handler::get_datareader_qos (DDS::DataReaderQos& dr_qos,
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
      if (profile->count_datareader_qos () == 0)
        {
          if (OpenDDS::DCPS::DCPS_debug_level > 7)
            {
              ACE_DEBUG ((LM_TRACE,
                ACE_TEXT ("DataReaderQos_Handler::get_datareader_qos - ")
                ACE_TEXT ("No DataReader QOS available in profile <%C>\n"),
                profile->name ().c_str ()));
            }
          return true;
        }
      // get the first DataReader in the XML
      dds::qosProfile::datareader_qos_iterator dr_it = profile->begin_datareader_qos ();
      return DataReaderQos_Handler::get_datareader_qos (dr_qos, dr_it->get ());
    }

  ACE_ERROR ((LM_DEBUG,
    ACE_TEXT ("DataReaderQos_Handler::get_datareader_qos - ")
    ACE_TEXT ("Unable to find DataReaderQos <%C>\n"),
    name));
  return false;
}

bool
DataReaderQos_Handler::get_datareader_qos (DDS::DataReaderQos& dr_qos,
                                           dds::datareaderQos * dr)
{
  typedef QOS_DataReader_T<dds::datareaderQos*, DDS::DataReaderQos> dr_type;
  dr_type datareader_qos_handler;
  datareader_qos_handler.read_qos (dr_qos, dr);

  return true;
}
