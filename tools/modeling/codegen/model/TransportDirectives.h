#ifndef TRANSPORT_DIRECTIVES_H
#define TRANSPORT_DIRECTIVES_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "model_export.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace Model { namespace Transport {

  class Type {
  public:
    enum Values {
      tcp,
      udp,
      multicast
    };
  };
  namespace Tcp {
    extern OpenDDS_Model_Export const ACE_TCHAR *svcName;
    extern OpenDDS_Model_Export const ACE_TCHAR *svcConfDir;
  };

  namespace Udp {
    extern OpenDDS_Model_Export const ACE_TCHAR *svcName;
    extern OpenDDS_Model_Export const ACE_TCHAR *svcConfDir;
  };

  namespace Multicast {
    extern OpenDDS_Model_Export const ACE_TCHAR *svcName;
    extern OpenDDS_Model_Export const ACE_TCHAR *svcConfDir;
  };
}; }; };

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

