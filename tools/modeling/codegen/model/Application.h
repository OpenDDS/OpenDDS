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
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {
  class DomainParticipant;
  class DomainParticipantFactory;
  class Topic;
  class Publisher;
  class Subscriber;
  class DataWriter;
  class DataReader;
} // End of namespace DDS

namespace OpenDDS { namespace DCPS {
  class TransportInst;
  class TransportImpl;
} } // End of namespace OpenDDS::DCPS

namespace OpenDDS { namespace Model {

  class OpenDDS_Model_Export Application {
    public:
      Application(int& argc, ACE_TCHAR *argv[]);
      ~Application();
    private:
      DDS::DomainParticipantFactory* factory_;
  };

} } // End of namespace OpenDDS::Model

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* APPLICATION_H */

