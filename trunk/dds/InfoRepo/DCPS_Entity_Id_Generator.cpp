#include "DcpsInfo_pch.h"
#include "DCPS_Entity_Id_Generator.h"
#include "ace/Log_Msg.h"

DCPS_Entity_Id_Generator::DCPS_Entity_Id_Generator ()
{
}

DCPS_Entity_Id_Generator::~DCPS_Entity_Id_Generator (void)
{
}



CORBA::Long DCPS_Entity_Id_Generator::get_next_part_id ()
{
  static CORBA::Long nextEntity = 1;

  CORBA::Long retValue = nextEntity;

  ++nextEntity;

  if (nextEntity == 0)
    {
      // We have rolled over and there can now exist objects with
      // the same key.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: DCPS_Entity_Key_Generator::get_next_part_id() Exceeded Maximum number of keys!\n")
        ACE_TEXT(" Next key will be a duplicate!\n")
        ));

      nextEntity = (CORBA::Long) 1;
    }

  return retValue;
}

CORBA::Long DCPS_Entity_Id_Generator::get_next_topic_id ()
{
  static CORBA::Long nextEntity = 1;

  CORBA::Long retValue = nextEntity;

  ++nextEntity;

  if (nextEntity == 0)
    {
      // We have rolled over and there can now exist objects with
      // the same key.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: DCPS_Entity_Key_Generator::get_next_topic_id() Exceeded Maximum number of keys!\n")
        ACE_TEXT(" Next key will be a duplicate!\n")
        ));

      nextEntity = (CORBA::Long) 1;
    }

  return retValue;
}

CORBA::Long DCPS_Entity_Id_Generator::get_next_sub_pub_id ()
{
  static CORBA::Long nextEntity = 1;

  CORBA::Long retValue = nextEntity;

  ++nextEntity;

  if (nextEntity == 0)
    {
      // We have rolled over and there can now exist objects with
      // the same key.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: DCPS_Entity_Key_Generator::get_next_sub_pub_id() Exceeded Maximum number of keys!\n")
        ACE_TEXT(" Next key will be a duplicate!\n")
        ));

      nextEntity = (CORBA::Long) 1;
    }

  return retValue;
}

bool
DCPS_Entity_Id_Generator::set_base_part_id (CORBA::Long id)
{
  static CORBA::Long nextEntity = 1;

  if (id > nextEntity) {
    nextEntity = id;
  }
  return true;
}

bool
DCPS_Entity_Id_Generator::set_base_topic_id (CORBA::Long id)
{
  static CORBA::Long nextEntity = 1;

  if (id > nextEntity) {
    nextEntity = id;
  }
  return true;
}

bool
DCPS_Entity_Id_Generator::set_base_sub_pub_id (CORBA::Long id)
{
  static CORBA::Long nextEntity = 1;

  if (id > nextEntity) {
    nextEntity = id;
  }
  return true;
}

