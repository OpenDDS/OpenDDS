// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "NetworkAddress.h"
#include "ace/OS_NS_netdb.h"
#include "ace/Sock_Connect.h"


#if !defined (__ACE_INLINE__)
# include "NetworkAddress.inl"
#endif /* ! __ACE_INLINE__ */


ACE_CDR::Boolean
operator<< (ACE_OutputCDR& outCdr, OpenDDS::DCPS::NetworkAddress& value)
{    
  outCdr << ACE_OutputCDR::from_boolean (ACE_CDR_BYTE_ORDER);

  outCdr << ACE_OutputCDR::from_octet (value.reserved_);
  outCdr << value.addr_.c_str();

  return outCdr.good_bit ();
}


ACE_CDR::Boolean
operator>> (ACE_InputCDR& inCdr, OpenDDS::DCPS::NetworkAddress& value)
{
  CORBA::Boolean byte_order;
  if (inCdr >> ACE_InputCDR::to_boolean (byte_order) == 0)
    return 0;

  inCdr.reset_byte_order (byte_order);

  if (inCdr >> ACE_InputCDR::to_octet (value.reserved_) == 0)
    return 0;

  char* buf = 0;
  
  if (inCdr >> buf == 0)
    return 0;

  value.addr_ = buf;

  delete[] buf;

  return inCdr.good_bit ();
}


const std::string& get_fully_qualified_hostname ()
{
  static std::string fullname;

  if (fullname.empty ())
  {
    size_t addr_count;
    ACE_INET_Addr *addr_array;
    OpenDDS::DCPS::StringVector nonFQDN;

    int result = ACE::get_ip_interfaces(addr_count, addr_array);
    if (result != 0 || addr_count < 1)
    {
      ACE_ERROR ((LM_ERROR,
        "(%P|%t)!!! ERROR: get_fully_qualified_hostname failed on %p\n"
        "ACE::get_ip_interfaces"));
    }
    else
    {
      for( size_t i = 0; i < addr_count; i++) {
        char hostname[MAXHOSTNAMELEN+1] = "";  

        //Discover the fully qualified hostname

        if (ACE::get_fqdn (addr_array[i], hostname, MAXHOSTNAMELEN+1) == 0)
        {
          if (ACE_OS::strchr (hostname, '.') != 0)
          {
            VDBG_LVL ((LM_DEBUG, "(%P|%t)found fqdn %s \n",  
              hostname), 2);
            fullname = hostname;
            return fullname;
          }
          else
          {
            VDBG_LVL ((LM_DEBUG, "(%P|%t)ip interface %s:%d - hostname %s \n",  
              addr_array[i].get_host_addr(), addr_array[i].get_port_number (), hostname), 2);
            nonFQDN.push_back (hostname);
          }
        }
      }
    }

    OpenDDS::DCPS::StringVector::iterator itEnd = nonFQDN.end ();
    for (OpenDDS::DCPS::StringVector::iterator it = nonFQDN.begin (); it != itEnd; ++it)
    {
      if (*it != "localhost")
      {
        ACE_DEBUG ((LM_DEBUG, "(%P|%t)!!! WARNING: Could not discover the FQDN, please "
          "correct system configuration.\n"));
        fullname = *it;
        return fullname;
      }
    }

    ACE_ERROR ((LM_ERROR,
        "(%P|%t)!!! ERROR: failed to discover the fully qualified hostname \n"));
  }

  return fullname;
}


