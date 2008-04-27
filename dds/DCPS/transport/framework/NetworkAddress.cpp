// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "NetworkAddress.h"
#include "ace/OS_NS_netdb.h"


#if !defined (__ACE_INLINE__)
# include "NetworkAddress.inl"
#endif /* ! __ACE_INLINE__ */


ACE_CDR::Boolean
operator<< (ACE_OutputCDR& outCdr, OpenDDS::DCPS::NetworkAddress& value)
{
  outCdr << value.reserved_;
  outCdr << value.addr_.c_str();

  return outCdr.good_bit ();
}


ACE_CDR::Boolean
operator>> (ACE_InputCDR& inCdr, OpenDDS::DCPS::NetworkAddress& value)
{
  inCdr >> ACE_InputCDR::to_octet (value.reserved_);
  char* buf = 0;
  inCdr >> buf;
  value.addr_ = buf;

  return inCdr.good_bit ();
}


const std::string& get_fully_qualified_hostname ()
{
  static std::string fullname;

  if (fullname.empty ())
  {
    char hostname[MAXHOSTNAMELEN+1];      

    //Discover the fully qualified hostname

    ACE_INET_Addr addr;
    if (addr.get_host_name(hostname, MAXHOSTNAMELEN+1) == -1)
    {
      ACE_ERROR((LM_ERROR, "(%P|%t)ERROR: get_full_qualified_hostname failed %p\n", 
        "get_host_name"));
    }

    if (strchr (hostname, '.') == 0)
    {
      ACE_INET_Addr addr (1000, hostname);
      if (addr.get_host_name(hostname, MAXHOSTNAMELEN+1) == -1)
      {
        ACE_ERROR((LM_ERROR, "(%P|%t)ERROR: failed to get host name %p\n", 
          "get_host_name"));
      }
    }

    fullname = hostname;
  }

  return fullname;
}
