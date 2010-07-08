// -*- C++ -*-
//
// $Id$
#ifndef NULLTOPICLISTENER_H
#define NULLTOPICLISTENER_H

#include "model_export.h"
#include "dds/DdsDcpsTopicS.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS { namespace Model {

  class OpenDDS_Model_Export NullTopicListener
    : public virtual OpenDDS::DCPS::LocalObject<DDS::TopicListener>
  {
  public:
    NullTopicListener();

    virtual ~NullTopicListener();

    virtual void on_inconsistent_topic (
      DDS::Topic_ptr topic,
      const DDS::InconsistentTopicStatus& status
    ) ACE_THROW_SPEC((CORBA::SystemException));

  };

} } // End of namespace OpenDDS::Model

#endif /* NULLTOPICLISTENER_H  */

