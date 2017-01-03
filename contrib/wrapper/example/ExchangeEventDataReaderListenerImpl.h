// -*- C++ -*-
// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

#ifndef EXCHANGE_EVENT_DATAREADER_LISTENER_IMPL
#define EXCHANGE_EVENT_DATAREADER_LISTENER_IMPL

#include "DataReader_Listener_Base.h"
#include <ace/Synch.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class ExchangeEventDataReaderListenerImpl
  : public DataReader_Listener_Base
{
public:
  ExchangeEventDataReaderListenerImpl ();

  virtual ~ExchangeEventDataReaderListenerImpl (void);

  // app-specific
  CORBA::Boolean is_exchange_closed_received();

  virtual void on_data_available(DDS::DataReader_ptr reader);

private:
  CORBA::Boolean is_exchange_closed_received_;
  ACE_Mutex lock_;
};

#endif /* EXCHANGE_EVENT_DATAREADER_LISTENER_IMPL  */
