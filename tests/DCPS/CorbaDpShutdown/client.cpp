// $Id: client.cpp 1861 2011-08-31 16:18:08Z mesnierp $

#include "TestC.h"
#include "ace/Get_Opt.h"
#include <iostream>
#include <unistd.h>

const ACE_TCHAR *ior = ACE_TEXT ("file://test.ior");

int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("k:"));
  int c;

  while ((c = get_opts ()) != -1)
    switch (c)
      {
      case 'k':
        ior = get_opts.opt_arg ();
        break;

      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage:  %s "
                           "-k <ior> "
                           "\n",
                           argv [0]),
                          -1);
      }
  // Indicates successful parsing of the command line
  return 0;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
    {
      CORBA::ORB_var orb = CORBA::ORB_init (argc, argv);

      if (parse_args (argc, argv) != 0)
        return 1;

      CORBA::Object_var tmp = orb->string_to_object(ior);

      Test::CorbaDpShutdown_var CorbaDpShutdown_ = Test::CorbaDpShutdown::_narrow(tmp.in ());

      if (CORBA::is_nil (CorbaDpShutdown_.in ()))
        {
          ACE_ERROR_RETURN ((LM_DEBUG,
                             "Nil Test::CorbaDpShutdown reference <%s>\n",
                             ior),
                            1);
        }
      
      for (int i=0;i<2;i++) {
	    std::cout << "Iteration " << i << std::endl;
      	sleep(2);
      	// Start DDS
	    std::cout << "DDS Start" << std::endl;
      	CorbaDpShutdown_->start();
      	sleep(2);
      	// Stop DDS
	    std::cout << "DDS Stop" << std::endl;
      	CorbaDpShutdown_->stop();
      }
      

      CorbaDpShutdown_->shutdown ();

      orb->destroy ();
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught:");
      return 1;
    }

  return 0;
}
