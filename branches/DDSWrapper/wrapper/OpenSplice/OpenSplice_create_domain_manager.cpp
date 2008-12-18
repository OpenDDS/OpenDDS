// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenSplice_create_domain_manager.cpp
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include "OpenSplice_Domain_Manager.h"

/// factory method for creating an OpenDDS domain manager
/// this is used within the Domain_Manager class
Domain_Manager_Impl * create_domain_manager (int & argc,
					     char * argv[],
					     DDS::DomainId_t domain_id)
{
  return new OpenSplice_Domain_Manager (argc,
					argv,
					domain_id);
}
