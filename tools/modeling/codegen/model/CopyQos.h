#ifndef COPYQOS_H
#define COPYQOS_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DdsDcpsInfrastructureC.h" // For QoS Policy types.

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace Model {

  /// Abstract interface for dependency free callbacks to the model data.
  class CopyQos {
    public:
      virtual void
      copyPublicationQos(
        unsigned int        which,
        DDS::DataWriterQos& writerQos
      ) = 0;

      virtual void
      copySubscriptionQos(
        unsigned int        which,
        DDS::DataReaderQos& readerQos
      ) = 0;

    virtual ~CopyQos() {}
  };

} } // End of namespace OpenDDS::Model

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* COPYQOS_H */

