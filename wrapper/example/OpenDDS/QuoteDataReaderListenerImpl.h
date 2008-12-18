// -*- C++ -*-
// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

#ifndef QUOTE_DATAREADER_LISTENER_IMPL
#define QUOTE_DATAREADER_LISTENER_IMPL

#include "DataReader_Listener_Base.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class QuoteDataReaderListenerImpl
  : public DataReader_Listener_Base
{
public:
  //Constructor
  QuoteDataReaderListenerImpl ();

  //Destructor
  virtual ~QuoteDataReaderListenerImpl (void);

  virtual void on_data_available(
    DDS::DataReader_ptr reader
  )
  throw (CORBA::SystemException);

};

#endif /* QUOTE_DATAREADER_LISTENER_IMPL  */
