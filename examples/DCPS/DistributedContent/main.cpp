
#include "ace/Get_Opt.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_sys_stat.h"
#include "ace/Time_Value.h"
#include "ace/streams.h"

#include "AbstractionLayer.h"
#include "ApplicationLevel.h"

#include <string>

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  /// return status of main
  int status = 0;

  // Initialize DDS before parsing the command line parameters.
  // This is done so the DDS command line parameters do not go through the
  // application's parameter processing.
  AbstractionLayer abs_layer;

  if ( abs_layer.init_DDS(argc, argv) )
  {

    /// Where the distributed content will be written defaults to "files"
    ACE_TString directory_name(ACE_TEXT("files"));
    /// Name of the file to start writing (option should only be given on one node)
    /// This file will be created by this application.
    ACE_TString publish_file_name;
    /// Indicates that this node will publish the first file
    bool        publish_first_file = false;
    /// Overide the default name of this node.  The default is the hostname.
    ACE_TString node_name;


    // parse command line options
    ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("d:f:n:"));
    int optionIterator;
    while ((optionIterator = get_opts ()) != EOF) {
      switch (optionIterator) {

            case 'd': /* d specifes file directory */
              directory_name = get_opts.opt_arg ();
              break;

            case 'f': /* f specifies that this node will publish the file specified */
              publish_first_file = true;
              publish_file_name = get_opts.opt_arg ();
              break;

            case 'n': /* n specifies this nodes name */
              node_name = get_opts.opt_arg ();
              break;

            case '?':
              ACE_ERROR_RETURN(
                (LM_ERROR,
                  ACE_TEXT("usage: -d <file directory>  -n <node name>  -f <start file name>\n")),
                0);
              break;
            default: /* no parameters */
              break;
      }
    }


    // make the file storage directory
    if (0 != ACE_OS::mkdir(directory_name.c_str()) )
    {
      // Check if the error is something beside the directory exists.
      if (errno != EEXIST)
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("Failed to create directory %s with return code %m\n"),
          directory_name.c_str()));
        status = -1;
      }
    }


    // check that the node name is set
    if (0 == ACE_OS::strcmp(node_name.c_str(), ACE_TEXT("")))
    {
      ACE_TCHAR local_hostname[1024];
      if ( 0 == ACE_OS::hostname(local_hostname, 1023))
      {
        node_name = local_hostname;
      }
      else
      {
        ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to retreive the hostname.\n") ));
        status = -1;
      }
    }

    // Only proceed if there have been no errors
    if (status == 0)
    {

      // Create the application level (which connects itself to the abstraction layer)
      ApplicationLevel app_level(&abs_layer, directory_name, node_name);

      // Initialize random number generator
      ACE_Time_Value curr_time = ACE_OS::gettimeofday();
      u_int seed = (u_int) (curr_time.usec() + curr_time.sec());
      ACE_OS::srand(seed);

      // Wait before publishing
      ACE_OS::sleep(2);

#if defined DDS_DISTRIBUTEDCONTENT_REQUIRES_USER
      // Check if this node publishes the first file or not
      if (publish_first_file)
      {
        app_level.publish_file(publish_file_name);
      }

      char user_file_name[1025];

      std::cout << "Enter name of file to publish (Enter to quit):" << std::endl;
      std::cin.getline(user_file_name, 1025);
      publish_file_name = user_file_name;

      while (publish_file_name.size() > 0)
      {
        app_level.publish_file(publish_file_name);

        std::cout << "Enter name of file to publish (Enter to quit):" << std::endl;
        std::cin.getline(user_file_name, 1025);
        publish_file_name = user_file_name;
      }
#else
      // Check if this node publishes the first file or not
      if (publish_first_file)
      {
        app_level.generate_new_file(publish_file_name, 1000000);
      }
      else
      {
        // Wait up to 20 seconds to receive the file
        int sleep_count = 0;
        while ( (0 == app_level.get_write_count()) && (sleep_count++ < 20))
        {
          ACE_OS::sleep(1);
        }
      }

      // keep track of total time spent
      int time = 0;

      int delay = 1 + (ACE_OS::rand() % 10);

      // Loop sleeping and creating diffs
      for (int diff_cnt = 0; diff_cnt < 3; ++diff_cnt)
      {
        time += delay;
        ACE_OS::sleep(delay);
        delay = 1 + (ACE_OS::rand() % 7);

        if ( ! app_level.generate_diff(ACE_OS::rand() % 100000))
        {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("Error - node %s failed to generate diff of size %d\n"),
            node_name.c_str(),
            delay));
        }
      }

      // sleep waiting for other nodes to finish
      ACE_OS::sleep(40 - time);
#endif /* DDS_DISTRIBUTEDCONTENT_REQUIRES_USER */

    }


    // Clean up DDS before finishing.
    abs_layer.shutdown_DDS();

  }
  else
  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR: Initialization of DDS failed!\n")));
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("usage: -d <file directory>  -n <node name>  -f <start file name>\n")));

    status = -1;
  }

  return status;
}
