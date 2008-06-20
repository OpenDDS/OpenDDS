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
 *
 * The ids are intended to be unique but duplicates will be created
 * after 4,294,967,294 ids have been generated.  Zero (0) will not be
 * generated.  Zero is reservered for an invalid Id.
 * 
 * Note: The sub/pub (or participant or topic) ids are unique per 
 *       DCPSInfoRepo instance. This allows multiple domains in same
 *       process (connect to single DCPSInfoRepo instance in current
 *       implementation) to use the same transport instance. If we need
 *       support multiple domains to use same transport instance in same 
 *       process but each domain connect to a different DCPSInfoRepo 
 *       instance then we need review this unique id generation schema.
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
