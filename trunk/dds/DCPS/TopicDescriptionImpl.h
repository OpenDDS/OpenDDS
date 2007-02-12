// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_TOPIC_DESCRIPTION_IMPL_H
#define TAO_DDS_DCPS_TOPIC_DESCRIPTION_IMPL_H

#include "dds/DdsDcpsTopicS.h"
#include "dds/DdsDcpsTypeSupportTaoS.h"
#include "ace/SString.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO
{
  namespace DCPS
  {
    /**
    * @class TopicDescriptionImpl
    *
    * @brief Implements the ::DDS::TopicDescription interface.
    *
    * See the DDS specification, OMG formal/04-12-02, for a description of
    * the interface this class is implementing.
    *
    */
    class TAO_DdsDcps_Export TopicDescriptionImpl
      : public virtual POA_DDS::TopicDescription
    {
    public:
      //Constructor
      TopicDescriptionImpl (const char*                    topic_name,
                            const char*                    type_name,
                            POA_TAO::DCPS::TypeSupport_ptr type_support,
                            ::DDS::DomainParticipant_ptr   participant);

      //Destructor
      virtual ~TopicDescriptionImpl (void);


      virtual char * get_type_name (
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual char * get_name (
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::DomainParticipant_ptr get_participant (
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      /** This method is not defined in the IDL and is defined for
      *  internal use.
      *  Return the type support of the topic.
      */
      POA_TAO::DCPS::TypeSupport_ptr get_type_support ();

    protected:
      /// The name of the topic.
      ACE_CString                    topic_name_;
      /// The datatype of the topic.
      ACE_CString                    type_name_;

      /// The participant that creates this topic.
      ::DDS::DomainParticipant_ptr   participant_;

      /// The type_support for this topic.
      POA_TAO::DCPS::TypeSupport_ptr type_support_;
    };



  } // namespace DCPS
} // namespace TAO

#endif /* TAO_DDS_DCPS_TOPIC_DESCRIPTION_IMPL_H  */
