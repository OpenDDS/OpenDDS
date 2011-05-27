// -*- C++ -*-
//
// $Id$


#ifndef TAO_DDS_DCPS_DATAWRITERREMOTE_H
#define TAO_DDS_DCPS_DATAWRITERREMOTE_H

#include "dds/DdsDcpsDataWriterRemoteS.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO
{
  namespace DCPS
  {
    class DataWriterImpl;

    /**
    * @class DataWriterRemoteImpl
    *
    * @brief Implements the ::TAO::DCPS::DataWriterRemote interface.
    *
    */
    class TAO_DdsDcps_Export DataWriterRemoteImpl
      : public virtual POA_TAO::DCPS::DataWriterRemote
      , public virtual PortableServer::RefCountServantBase
    {
    public:
      ///Constructor
      DataWriterRemoteImpl (DataWriterImpl* parent);

      ///Destructor
      virtual ~DataWriterRemoteImpl (void);


    virtual void add_associations (
        ::TAO::DCPS::RepoId yourId,
        const ReaderAssociationSeq & readers
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual void remove_associations (
        const ReaderIdSeq & readers,
        ::CORBA::Boolean callback
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual void update_incompatible_qos (
        const TAO::DCPS::IncompatibleQosStatus & status
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));


    private:
        DataWriterImpl* parent_;
   };

  } // namespace DCPS
} // namespace TAO

#endif /* DDSDCPSPUBLICATION_DATAWRITER_H_  */
