#include "TransportDirectives.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

const ACE_TCHAR*
OpenDDS::Model::Transport::Tcp::svcName = ACE_TEXT("OpenDDS_Tcp");

const ACE_TCHAR*
OpenDDS::Model::Transport::Tcp::svcConfDir =
    ACE_TEXT("dynamic OpenDDS_Tcp Service_Object * ")
    ACE_TEXT("OpenDDS_Tcp:_make_TcpLoader()");

const ACE_TCHAR*
OpenDDS::Model::Transport::Udp::svcName = ACE_TEXT("OpenDDS_Udp");

const ACE_TCHAR*
OpenDDS::Model::Transport::Udp::svcConfDir =
    ACE_TEXT("dynamic OpenDDS_Udp Service_Object *")
    ACE_TEXT(" OpenDDS_Udp:_make_UdpLoader()");

const ACE_TCHAR*
OpenDDS::Model::Transport::Multicast::svcName =
    ACE_TEXT("OpenDDS_Multicast");

const ACE_TCHAR*
OpenDDS::Model::Transport::Multicast::svcConfDir =
    ACE_TEXT("dynamic OpenDDS_Multicast Service_Object *")
    ACE_TEXT(" OpenDDS_Multicast:_make_MulticastLoader()");

OPENDDS_END_VERSIONED_NAMESPACE_DECL
