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
 */
class DCPS_Entity_Id_Generator
{
public:
  DCPS_Entity_Id_Generator(void);
  ~DCPS_Entity_Id_Generator(void);

  CORBA::Long get_next_id ();

private:
  /// The next value to be returned.
  CORBA::Long nextEntity_;
};

#endif /* DCPS_ENTITY_ID_GENERATOR_H  */
