// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "DataLinkSet.h"

#include  "dds/DCPS/DataSampleList.h"
#include  "TransportSendListener.h"

#include "EntryExit.h"


#if !defined (__ACE_INLINE__)
#include "DataLinkSet.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::DataLinkSet::~DataLinkSet()
{
  DBG_ENTRY("DataLinkSet","~DataLinkSet");
}

