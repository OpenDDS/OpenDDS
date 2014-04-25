/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ManyToMany_Options_h
#define ManyToMany_Options_h

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Arg_Shifter.h"

struct Options
{
  Options(int argc, ACE_TCHAR* argv[])
  : num_processes(1)
  , num_pub_participants(1)
  , num_writers(1)
  , num_sub_participants(1)
  , num_readers(1)
  , reliable(false)
  , sample_size(10)
  , delay_ms(1000)
  {
    ACE_Arg_Shifter parser( argc, argv);
    while( parser.is_anything_left()) {
      const ACE_TCHAR* currentArg = 0;
      if( 0 != (currentArg = parser.get_the_parameter("processes"))) {
        num_processes = ACE_OS::atoi( currentArg);
        parser.consume_arg();

      } else if( 0 != (currentArg = parser.get_the_parameter("pub_participants"))) {
        num_pub_participants = ACE_OS::atoi( currentArg);
        parser.consume_arg();

      } else if( 0 != (currentArg = parser.get_the_parameter("writers"))) {
        num_writers = ACE_OS::atoi( currentArg);
        parser.consume_arg();

      } else if( 0 != (currentArg = parser.get_the_parameter("samples"))) {
        num_samples = ACE_OS::atoi( currentArg);
        parser.consume_arg();

      } else if( 0 != (currentArg = parser.get_the_parameter("sub_participants"))) {
        num_sub_participants = ACE_OS::atoi( currentArg);
        parser.consume_arg();

      } else if( 0 != (currentArg = parser.get_the_parameter("readers"))) {
        num_readers = ACE_OS::atoi( currentArg);
        parser.consume_arg();

      } else if( 0 != (currentArg = parser.get_the_parameter("sample_size"))) {
        sample_size = ACE_OS::atoi( currentArg);
        parser.consume_arg();

      } else if( 0 != (currentArg = parser.get_the_parameter("delay_ms"))) {
        delay_ms = ACE_OS::atoi( currentArg);
        parser.consume_arg();

      } else if( 0 <= (parser.cur_arg_strncasecmp("reliable"))) {
        reliable = true;
        parser.consume_arg();

      } else {
        parser.ignore_arg();
      }
    }
  }

  unsigned int num_processes;
  unsigned int num_pub_participants; // per process
  unsigned int num_writers; // per participant
  unsigned int num_samples; // per writer
  unsigned int num_sub_participants; // in this process
  unsigned int num_readers; // per participant
  bool reliable;
  unsigned int sample_size;
  unsigned int delay_ms;
};

#endif /* ManyToMany_Options_h  */
