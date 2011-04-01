#ifndef APPLICATION_H
#define APPLICATION_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <ace/ace_wchar.h>
#include "model_export.h"

namespace DDS {
  class DomainParticipant;
  class Topic;
  class Publisher;
  class Subscriber;
  class DataWriter;
  class DataReader;
} // End of namespace DDS

namespace OpenDDS { namespace DCPS {
  class TransportConfiguration;
  class TransportImpl;
} } // End of namespace OpenDDS::DCPS

namespace OpenDDS { namespace Model {

  class OpenDDS_Model_Export Application {
    public:
      Application(int& argc, ACE_TCHAR *argv[]);
      ~Application();
  };

} } // End of namespace OpenDDS::Model

#include /**/ "ace/post.h"

#endif /* APPLICATION_H */

