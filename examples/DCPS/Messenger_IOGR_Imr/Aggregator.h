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
  // Ctor
  Aggregator();

  // Initialize the ORB, POA etc.
  void init (int argc,
             char *argv[]);


  // Merges the different IORS
  int make_merged_iors ();

  // Sets the properties for the profiles
  int set_properties ();

  // Run the  ORB event loop..
  int run ();

  // Write the merged IOR to a file
  int write_to_file ();

  // Return the pointer to the copy of our ORB
  CORBA::ORB_ptr orb ();

private:
  // Our ORB
  CORBA::ORB_var orb_;

  // The merged IOR set
  CORBA::Object_var merged_set_;

  TAO_IOP::TAO_IOR_Manipulation_var iorm_;
  CORBA::Object_var object_primary_;
  CORBA::Object_var object_secondary_;
};

#endif /*TEST_FT_IOGR_MANAGER_H */
