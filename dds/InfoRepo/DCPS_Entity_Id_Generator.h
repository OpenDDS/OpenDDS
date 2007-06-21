// ============================================================================
/**
 *  @file   DCPS_Entity_Id_Generator.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef DCPS_ENTITY_ID_GENERATOR_H
#define DCPS_ENTITY_ID_GENERATOR_H

#include "tao/corbafwd.h"

/**
 * @class DCPS_Entity_Id_Generator
 *
 * @brief Used to generate ids for the entities in the DCPSInfoRepo.
 *        The high 16 bits is the domain id and the low 16 bits is
 *        the counter. 
 *
 * The ids are intended to be unique acoss domains but duplicates 
 * will be created inside domain after 65534 ids have been generated. 
 * Zero (0) will not be generated.  Zero is reservered for an invalid Id.
 */
class DCPS_Entity_Id_Generator
{
public:
  DCPS_Entity_Id_Generator();
  ~DCPS_Entity_Id_Generator(void);

  CORBA::Long get_next_part_id ();
  CORBA::Long get_next_topic_id ();
  CORBA::Long get_next_sub_pub_id ();

  bool set_base_part_id (CORBA::Long id);
  bool set_base_topic_id (CORBA::Long id);
  bool set_base_sub_pub_id (CORBA::Long id);
};

#endif /* DCPS_ENTITY_ID_GENERATOR_H  */
