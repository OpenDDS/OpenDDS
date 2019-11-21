#include "Aggregator.h"
#include "ace/Get_Opt.h"
#include "ace/Read_Buffer.h"
#include "tao/PortableServer/PortableServer.h"
#include "orbsvcs/FaultTolerance/FT_Service_Activate.h"
#include "orbsvcs/FaultTolerance/FT_IOGR_Property.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_fcntl.h"
#include "ace/Argv_Type_Converter.h"

// Files which have the IOR
const ACE_TCHAR *first_ior = 0;
const ACE_TCHAR *second_ior = 0;
const ACE_TCHAR *ior_output_file = 0;


int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("a:b:c:"));
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
          ACE_TEXT("usage:  %s -a <iorfile> -b <iorfile> ")
          ACE_TEXT("-c <output ior file>\n"),
          argv [0]), -1);
      }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  Aggregator manager;

  try
    {
      ACE_Argv_Type_Converter converter (argc, argv);
      // Initilaize the ORB, POA etc.
      manager.init (converter.get_argc(), converter.get_ASCII_argv());

      // the command line arguments
      if (parse_args (argc, argv) == -1)
        return -1;

      // Merge the different IORS
      manager.make_merged_iors ();

      // Set properties. This is the most important portion of the
      // test
      manager.set_properties ();

      // Write IOR to file
      manager.write_to_file ();
    }
  catch (const CORBA::Exception& e)
    {
      e._tao_print_exception(ACE_TEXT("Caught"));
      return -1;
    }

  return 0;
}

Aggregator::Aggregator (void)
  :orb_ (0),
   merged_set_ (0)
{
  //no-op
}

void
Aggregator::init (int argc, char *argv[])
{
  this->orb_ = CORBA::ORB_init (argc, argv);

  // Obtain the RootPOA.
  CORBA::Object_var obj_var =
    this->orb_->resolve_initial_references ("RootPOA");

  // Get the POA_var object from Object_var.
  PortableServer::POA_var root_poa_var =
    PortableServer::POA::_narrow (obj_var.in ());

  // Get the POAManager of the RootPOA.
  PortableServer::POAManager_var poa_manager_var =
    root_poa_var->the_POAManager ();

  poa_manager_var->activate ();
}

int
Aggregator::make_merged_iors ()
{
  // First  server
  object_primary_ =
    this->orb_->string_to_object (ACE_TEXT_ALWAYS_CHAR(first_ior));

  //Second server
  object_secondary_ =
    this->orb_->string_to_object (ACE_TEXT_ALWAYS_CHAR(second_ior));

  // Get an object reference for the ORBs IORManipultion object!
  CORBA::Object_var IORM =
    this->orb_->resolve_initial_references (TAO_OBJID_IORMANIPULATION, 0);

  iorm_ =
    TAO_IOP::TAO_IOR_Manipulation::_narrow (IORM.in());


  // Create the list
  TAO_IOP::TAO_IOR_Manipulation::IORList iors (2);
  iors.length(2);
  iors [0] = CORBA::Object::_duplicate (object_primary_.in ());
  iors [1] = CORBA::Object::_duplicate (object_secondary_.in ());

  // Create a merged set 1;
  merged_set_ =
    iorm_->merge_iors (iors);
  return 0;
}

int
Aggregator::set_properties ()
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
                                              this->merged_set_.in ());

  // Set the primary
  // See we are setting the first ior as the primary
  if (retval != 0)
    {
      retval = iorm_->set_primary (&iogr_prop,
                                  object_primary_.in (),
                                  this->merged_set_.in ());
    }

  return 0;
}

int
Aggregator::run ()
{
  try
    {
      this->orb_->run ();
    }
  catch (const CORBA::Exception&)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         "(%P|%t) ERROR: Error in run \n"),
                        -1);
    }

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
      FILE *output_file= ACE_OS::fopen (ior_output_file, ACE_TEXT("w"));
      if (output_file == 0)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%P|%t) ERROR: Cannot open output file for writing IOR: %s",
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
