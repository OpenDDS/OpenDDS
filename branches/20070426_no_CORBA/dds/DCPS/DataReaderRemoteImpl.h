// -*- C++ -*-
//
// $Id$

#ifndef TAO_DDS_DCPS_DATAREADERREMOTE_H
#define TAO_DDS_DCPS_DATAREADERREMOTE_H

#include "dcps_export.h"
#include "DdsDcpsDataReaderRemoteS.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO
{
  namespace DCPS
  {

    class DataReaderImpl;

    /**
    * @class DataReaderRemoteImpl
    *
    * @brief Implements the ::TAO::DCPS::ReaderRemote interface that 
    *        is used to add and remove associations.
    *
    */
    class TAO_DdsDcps_Export DataReaderRemoteImpl
      : public virtual POA_TAO::DCPS::DataReaderRemote
      , public virtual PortableServer::RefCountServantBase
    {
    public:


      //Constructor
      DataReaderRemoteImpl (DataReaderImpl* parent);

      //Destructor
      virtual ~DataReaderRemoteImpl (void);


      virtual void add_associations (
          ::TAO::DCPS::RepoId yourId,
          const TAO::DCPS::WriterAssociationSeq & writers
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual void remove_associations (
          const TAO::DCPS::WriterIdSeq & writers,
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
      DataReaderImpl* parent_;

    };

  } // namespace DCPS
} // namespace TAO

#endif /* TAO_DDS_DCPS_DATAREADERREMOTE_H  */
