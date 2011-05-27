//$Id$
#include "Aggregator.h"
#include "ace/Get_Opt.h"
#include "ace/Read_Buffer.h"
#include "tao/PortableServer/PortableServer.h"
#include "orbsvcs/FaultTolerance/FT_Service_Activate.h"
#include "orbsvcs/FaultTolerance/FT_IOGR_Property.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_fcntl.h"

// Files which have the IOR
const char *first_ior = 0;
const char *second_ior = 0;
const char *ior_output_file = 0;


int
parse_args (int argc, char *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, "a:b:c:");
  int c;

  while ((c = get_opts ()) != -1)
    switch (c)
      {
      case 'a':
        first_ior = get_opts.opt_arg ();
        break;
      case 'b':
        second_ior = get_opts.opt_arg ();
        break;
      case 'c':
        ior_output_file = get_opts.opt_arg ();
        break;
      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage:  %s "
                           "-a <iorfile>"
                           "-b <iorfile>"
                           "-c <output ior file>"
                           "\n",
                           argv [0]),
                          -1);
      }
  // Indicates sucessful parsing of the command line
  return 0;
}


int
main (int argc,
      char *argv[])
{
  ACE_DECLARE_NEW_CORBA_ENV;

  Aggregator manager;

  ACE_TRY
    {
      // Initilaize the ORB, POA etc.
      manager.init (argc,
                    argv
                    ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;

      // the command line arguments
      if (parse_args (argc, argv) == -1)
        return -1;

      // Merge the different IORS
      manager.make_merged_iors (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;

      // Set properties. This is the most important portion of the
      // test
      manager.set_properties (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;

      // Write IOR to file
      manager.write_to_file ();
    }
  ACE_CATCHANY
    {
      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                           "Caught");
      return -1;
    }
  ACE_ENDTRY;

  return 0;
}

Aggregator::Aggregator (void)
  :orb_ (0),
   merged_set_ (0)
{
  //no-op
}

void
Aggregator::init (int argc,
               char *argv[]
               ACE_ENV_ARG_DECL)
{
  this->orb_ = CORBA::ORB_init (argc,
                                argv,
                                0
                                ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  // Obtain the RootPOA.
  CORBA::Object_var obj_var =
    this->orb_->resolve_initial_references ("RootPOA"
                                            ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  // Get the POA_var object from Object_var.
  PortableServer::POA_var root_poa_var =
    PortableServer::POA::_narrow (obj_var.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  // Get the POAManager of the RootPOA.
  PortableServer::POAManager_var poa_manager_var =
    root_poa_var->the_POAManager (ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_CHECK;

  poa_manager_var->activate (ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_CHECK;
}

int
Aggregator::make_merged_iors (ACE_ENV_SINGLE_ARG_DECL)
{
  // First  server
  object_primary_ =
    this->orb_->string_to_object (first_ior
                                  ACE_ENV_ARG_PARAMETER);
  ACE_CHECK_RETURN (-1);

  //Second server
  object_secondary_ =
    this->orb_->string_to_object (second_ior
                                  ACE_ENV_ARG_PARAMETER);
  ACE_CHECK_RETURN (-1);

  // Get an object reference for the ORBs IORManipultion object!
  CORBA::Object_var IORM =
    this->orb_->resolve_initial_references (TAO_OBJID_IORMANIPULATION,
                                            0
                                            ACE_ENV_ARG_PARAMETER);
  ACE_CHECK_RETURN (-1);

  iorm_ =
    TAO_IOP::TAO_IOR_Manipulation::_narrow (IORM.in() ACE_ENV_ARG_PARAMETER);
  ACE_CHECK_RETURN (-1);


  // Create the list
  TAO_IOP::TAO_IOR_Manipulation::IORList iors (2);
  iors.length(2);
  iors [0] = CORBA::Object::_duplicate (object_primary_.in ());
  iors [1] = CORBA::Object::_duplicate (object_secondary_.in ());

  // Create a merged set 1;
  merged_set_ =
    iorm_->merge_iors (iors ACE_ENV_ARG_PARAMETER);
  ACE_CHECK_RETURN (-1);

  return 0;
}

int
Aggregator::set_properties (ACE_ENV_SINGLE_ARG_DECL)
{
  FT::TagFTGroupTaggedComponent ft_tag_component;

  // Property values

  // Major and Minor revision numbers
  ft_tag_component.component_version.major = (CORBA::Octet) 1;
  ft_tag_component.component_version.minor = (CORBA::Octet) 0;

  // Domain id
  const char *id = "iogr_testing";
  ft_tag_component.group_domain_id = id;

  // Object group id
  ft_tag_component.object_group_id =
    (CORBA::ULongLong) 10;

  // Version
  ft_tag_component.object_group_ref_version =
    (CORBA::ULong) 5;

  // Construct the IOGR Property class
  TAO_FT_IOGR_Property iogr_prop (ft_tag_component);

  // Set the property
  CORBA::Boolean retval = iorm_->set_property (&iogr_prop,
                                              this->merged_set_.in ()
                                              ACE_ENV_ARG_PARAMETER);
  ACE_CHECK_RETURN (-1);

  // Set the primary
  // See we are setting the first ior as the primary
  if (retval != 0)
    {
      retval = iorm_->set_primary (&iogr_prop,
                                  object_primary_.in (),
                                  this->merged_set_.in ()
                                  ACE_ENV_ARG_PARAMETER);
      ACE_CHECK_RETURN (-1);
    }

  return 0;
}

int
Aggregator::run (ACE_ENV_SINGLE_ARG_DECL)
{
  ACE_TRY
    {
      this->orb_->run (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;
    }
  ACE_CATCHANY
    {
      ACE_ERROR_RETURN ((LM_DEBUG,
                         "Error in run \n"),
                        -1);
    }
  ACE_ENDTRY;

  return 0;
}

int
Aggregator::write_to_file (void)
{
  //
  CORBA::String_var iorref =
    this->orb_->object_to_string (this->merged_set_.in ());

  if (ior_output_file != 0)
    {
      FILE *output_file= ACE_OS::fopen (ior_output_file, "w");
      if (output_file == 0)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "Cannot open output file for writing IOR: %s",
                           ior_output_file),
                          1);
      ACE_OS::fprintf (output_file, "%s", iorref.in ());
      ACE_OS::fclose (output_file);
    }

  return 0;
}

CORBA::ORB_ptr
Aggregator::orb (void)
{
  return this->orb_.in ();
}
