#include "DcpsInfo_pch.h"

#include "UpdateManager.h"

UpdateManager::UpdateManager (void)
  : info_ (0)
{ }

UpdateManager::~UpdateManager (void)
{ }

void
UpdateManager::add (TAO_DDS_DCPSInfo_i* info)
{
  info_ = info;
}

int
UpdateManager::svc (void)
{
  return 0;
}

void
UpdateManager::shutdown (void)
{
}

int
UpdateManager::init (int , ACE_TCHAR *[])
{
  return 0;
}

int
UpdateManager::fini (void)
{
  return 0;
}

int
UpdateManager_Loader::init (void)
{
  return ACE_Service_Config::process_directive
    (ace_svc_desc_UpdateManager);
}

ACE_FACTORY_DEFINE (ACE_Local_Service, UpdateManager)
ACE_STATIC_SVC_DEFINE (UpdateManager,
                         ACE_TEXT ("UpdateManager"),
                         ACE_SVC_OBJ_T,
                         &ACE_SVC_NAME (UpdateManager),
                         ACE_Service_Type::DELETE_THIS
                         | ACE_Service_Type::DELETE_OBJ,
                         0)
