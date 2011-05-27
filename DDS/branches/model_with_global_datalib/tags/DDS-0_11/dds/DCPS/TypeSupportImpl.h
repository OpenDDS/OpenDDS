// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_TOPIC_TYPE_SUPPORT_IMPL_H
#define TAO_DDS_DCPS_TOPIC_TYPE_SUPPORT_IMPL_H

#include  "dds/DdsDcpsTopicS.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace TAO
{
  namespace DCPS
  {
    //Class TypeSupport
    class TAO_DdsDcps_Export TypeSupportImpl 
      : public virtual POA_DDS::TypeSupport
    {
    public:
      //Constructor 
      TypeSupportImpl (void);
      
      //Destructor 
      virtual ~TypeSupportImpl (void);
      
    };

  } // namespace DCPS
} // namespace TAO

#endif /* TAO_DDS_DCPS_TOPIC_TYPE_SUPPORT_IMPL_H  */
