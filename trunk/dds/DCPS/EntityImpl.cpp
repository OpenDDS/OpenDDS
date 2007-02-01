// -*- C++ -*-
//
// $Id$


#include "DCPS/DdsDcps_pch.h"
#include  "EntityImpl.h"

namespace TAO
{
  namespace DCPS
  {
    // Implementation skeleton constructor
    EntityImpl::EntityImpl ()
      : enabled_(false),
        entity_deleted_(false)
      {
      }

    // Implementation skeleton destructor
    EntityImpl::~EntityImpl (void)
      {
      }

    ::DDS::ReturnCode_t
    EntityImpl::set_enabled (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        if (enabled_ == false)
          {
            enabled_ = true;
          }
        return ::DDS::RETCODE_OK;
      }


    ::DDS::StatusKindMask
    EntityImpl::get_status_changes (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ::DDS::StatusKindMask status_changed = 0;
        if (enabled_ == true)
          {
            // iterator status
            Statuses::iterator it;
            for (it = status_changes_.begin ();
                 it != status_changes_.end ();
                 it ++)
              {
                if (it->second == true)
                  {
                    status_changed |= it->first;
                  }
              }
          }
        return status_changed;
      }


      void
      EntityImpl::set_status_changed_flag (
        ::DDS::StatusKind status,
        bool status_changed_flag)
      {
        Statuses::iterator it = status_changes_.find (status);

        if (it == status_changes_.end ())
          {
            std::pair<Statuses::iterator, bool> pair
              = status_changes_.insert(Statuses::value_type(status, status_changed_flag));
            if (pair.second == false)
              {
                ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: EntityImpl::set_status_changed_flag, ")
                            ACE_TEXT("insert status failed. \n")));
              }
          }
        else
          {
            it->second = status_changed_flag;
          }
      }


      void
      EntityImpl::set_deleted (bool state)
      {
        if (entity_deleted_ != state)
          {
            entity_deleted_ = state;
          }
      }


      bool
      EntityImpl::get_deleted ()
      {
        bool deleted_state = true;
        if (entity_deleted_ != true)
          {
            deleted_state = false;
          }
        return deleted_state;
      }


  } // namespace DCPS
} // namespace TAO
