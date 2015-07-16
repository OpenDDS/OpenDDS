// -*- C++ -*-

//=============================================================================
/**
 *  @file    Domain_Manager.cpp
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include "Domain_Manager.h"

#if !defined (__ACE_INLINE__)
#include "Domain_Manager.inl"
#endif

// this factory function has to be implemented for each specific DDS
// implementation
extern Domain_Manager_Impl * create_domain_manager (int & argc,
                                                    ACE_TCHAR * argv[],
                                                    DDS::DomainId_t domain_id);

// this factory function has to be implemented for each specific DDS
// implementation
extern Domain_Manager_Impl * create_domain_manager (int & argc,
                                                    ACE_TCHAR * argv[],
                                                    DDS::DomainId_t domain_id,
                                                    const DDS::DomainParticipantQos & qos);

Manager_Exception::Manager_Exception (const std::string& reason)
 : reason_ (reason)
{
}

Domain_Manager::Domain_Manager ()
  : manager_impl_ (0)
{
}

Domain_Manager::Domain_Manager (int & argc,
                                ACE_TCHAR *argv[],
                                DDS::DomainId_t domain_id)
  : manager_impl_ (create_domain_manager (argc, argv, domain_id))
{
}

Domain_Manager::Domain_Manager (int & argc,
                                ACE_TCHAR *argv[],
                                DDS::DomainId_t domain_id,
                                const DDS::DomainParticipantQos & qos)
  : manager_impl_ (create_domain_manager (argc, argv, domain_id, qos))
{
}

Domain_Manager::Domain_Manager (Domain_Manager_Ptr impl)
  : manager_impl_ (impl)
{
}

Domain_Manager::Domain_Manager (const Domain_Manager & copy)
  : manager_impl_ (copy.manager_impl_)
{
}

void
Domain_Manager::operator= (const Domain_Manager& copy)
{
  // check for self assignment first
  if (this != &copy)
    {
      manager_impl_ = copy.manager_impl_;
    }
}
