#include "TransportDirectives.h"

const ACE_TCHAR*
OpenDDS::Model::Transport::Tcp::svcName = ACE_TEXT("DCPS_SimpleTcpLoader");

const ACE_TCHAR*
OpenDDS::Model::Transport::Tcp::svcConfDir =
    ACE_TEXT("dynamic DCPS_SimpleTcpLoader Service_Object * ")
    ACE_TEXT("SimpleTcp:_make_DCPS_SimpleTcpLoader() \"-type SimpleTcp\"");

const ACE_TCHAR*
OpenDDS::Model::Transport::Udp::svcName = ACE_TEXT("OpenDDS_DCPS_Udp_Service");

const ACE_TCHAR*
OpenDDS::Model::Transport::Udp::svcConfDir =
    ACE_TEXT("dynamic OpenDDS_DCPS_Udp_Service Service_Object *")
    ACE_TEXT(" OpenDDS_Udp:_make_UdpLoader()");

const ACE_TCHAR*
OpenDDS::Model::Transport::Multicast::svcName = 
    ACE_TEXT("OpenDDS_DCPS_Multicast_Service");

const ACE_TCHAR*
OpenDDS::Model::Transport::Multicast::svcConfDir =
    ACE_TEXT("dynamic OpenDDS_DCPS_Multicast_Service Service_Object *")
    ACE_TEXT(" OpenDDS_Multicast:_make_MulticastLoader()");

