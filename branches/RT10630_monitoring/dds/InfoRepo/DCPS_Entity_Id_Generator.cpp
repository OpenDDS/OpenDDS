#include "DcpsInfo_pch.h"
#include "DCPS_Entity_Id_Generator.h"

#include "ace/Log_Msg.h"

DCPS_Entity_Id_Generator::DCPS_Entity_Id_Generator (void)
{
  nextEntity_ = (CORBA::Long) 1;
}

DCPS_Entity_Id_Generator::~DCPS_Entity_Id_Generator (void)
{
}



CORBA::Long DCPS_Entity_Id_Generator::get_next_id ()
{
  CORBA::Long retValue = nextEntity_;

  ++nextEntity_;

  if (nextEntity_ == 0)
    {
      // We have rolled over and there can now exist objects with
      // the same key.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: DCPS_Entity_Key_Generator::get_next_key() Exceeded Maximum number of keys!\n")
        ACE_TEXT(" Next key will be a duplicate!\n")
        ));

      nextEntity_ = (CORBA::Long) 1;
    }

  return retValue;
}

bool
DCPS_Entity_Id_Generator::set_base_id (CORBA::Long id)
{
  if (id > nextEntity_) {
    nextEntity_ = id;
  }

  return true;
}
