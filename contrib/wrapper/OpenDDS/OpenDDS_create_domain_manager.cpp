// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_create_domain_manager.cpp
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include "OpenDDS_Domain_Manager.h"

/// factory method for creating an OpenDDS domain manager
/// this is used within the Domain_Manager class
Domain_Manager_Impl * create_domain_manager (int & argc,
                                             ACE_TCHAR * argv[],
                                             DDS::DomainId_t domain_id)
{
  return new OpenDDS_Domain_Manager (argc,
                                     argv,
                                     domain_id);
}

/// factory method for creating an OpenDDS domain manager
/// this is used within the Domain_Manager class
Domain_Manager_Impl * create_domain_manager (int & argc,
                                             ACE_TCHAR * argv[],
                                             DDS::DomainId_t domain_id,
                                             const DDS::DomainParticipantQos & qos)
{
  return new OpenDDS_Domain_Manager (argc,
                                     argv,
                                     domain_id,
                                     qos);
}
