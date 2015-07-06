// -*- C++ -*-

// ============================================================================
//
// = LIBRARY
//   test
//
// = FILENAME
//    Aggregator.h
//
// = DESCRIPTION
//    This class is based upon $TAO_ROOT/orbsvcs/tests/FaultTolerance/IOGR/Manager
//     The only thing this class does is combine two IOR's into a single
//     IOGR, setting the first IOR as the primary.
//
// = AUTHOR
//     Ciju John <johnc at ociweb dot com>
//
// ============================================================================
#ifndef TEST_FT_IOGR_MANAGER_H
#define TEST_FT_IOGR_MANAGER_H

#include "tao/ORB.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "tao/Object.h"
#include "tao/IORManipulation/IORManip_Loader.h"

class Aggregator
{
public:

  Aggregator (void);
  // Ctor

  void init (int argc,
             char *argv[]);

  // Initialize the ORB, POA etc.

  int make_merged_iors ();
  // Merges the different IORS

  int set_properties ();
  // Sets the properties for the profiles

  int run ();
  // Run the  ORB event loop..

  int write_to_file (void);
  // Write the merged IOR to a file

  CORBA::ORB_ptr orb (void);
  // Return the pointer to the copy of our ORB
private:
  CORBA::ORB_var orb_;
  // Our ORB

  CORBA::Object_var merged_set_;
  // The merged IOR set

  TAO_IOP::TAO_IOR_Manipulation_var iorm_;
  CORBA::Object_var object_primary_;
  CORBA::Object_var object_secondary_;
};

#endif /*TEST_FT_IOGR_MANAGER_H */
