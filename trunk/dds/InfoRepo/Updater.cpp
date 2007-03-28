// -*- C++ -*-

#include "DcpsInfo_pch.h"
#include "Updater.h"
#include "UpdateManager.h"

#include "ace/Dynamic_Service.h"

Updater::~Updater (void)
{ }

UpdaterBase::UpdaterBase (void)
{
  um_ = ACE_Dynamic_Service<UpdateManager>::instance
    ("UpdateManager");

  if (um_ != 0) {
    um_->add (this);
  }
}

UpdaterBase::~UpdaterBase (void)
{ }

void
UpdaterBase::unregisterCallback (void)
{
  if (um_ != 0) {
    um_->remove (const_cast<UpdaterBase*>(this));
  }

  GuardType guard (um_lock_);
  um_ = 0;
}
