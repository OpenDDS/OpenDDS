/*
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
  : num_pub_processes(1)
  , num_pub_participants(1)
  , num_writers(1)
  , num_samples(2)
  , num_sub_processes(1)
  , num_sub_participants(1)
  , num_readers(1)
  , reliable(false)
  , sample_size(10)
  , delay_msec(1000)
  , total_duration_msec(delay_msec * num_samples * num_writers * num_pub_participants * 2)
  , no_validation(false)
  {
    ACE_Arg_Shifter parser( argc, argv);
    while( parser.is_anything_left()) {
      const ACE_TCHAR* currentArg = 0;
      if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-pub_processes")))) {
        num_pub_processes = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) num_pub_processes=%d\n"), num_pub_processes));

      } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-pub_participants")))) {
        num_pub_participants = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) num_pub_participants=%d\n"), num_pub_participants));

      } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-writers")))) {
        num_writers = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) num_writers=%d\n"), num_writers));

      } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-samples")))) {
        num_samples = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) num_samples=%d\n"), num_samples));

      } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-sub_processes")))) {
        num_sub_processes = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) num_sub_processes=%d\n"), num_sub_processes));

      } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-sub_participants")))) {
        num_sub_participants = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) num_sub_participants=%d\n"), num_sub_participants));

      } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-readers")))) {
        num_readers = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) num_readers=%d\n"), num_readers));

      } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-sample_size")))) {
        sample_size = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) sample_size=%d\n"), sample_size));

      } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-delay_msec")))) {
        delay_msec = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) delay_msec=%d\n"), delay_msec));

      } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-total_duration_msec")))) {
        total_duration_msec = ACE_OS::atoi( currentArg);
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) total_duration_msec=%d\n"), total_duration_msec));

      } else if( 0 <= (parser.cur_arg_strncasecmp(ACE_TEXT("-reliable")))) {
        reliable = true;
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) reliable\n")));

      } else if( 0 <= (parser.cur_arg_strncasecmp(ACE_TEXT("-no_validation")))) {
        no_validation = true;
        parser.consume_arg();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) no validation\n")));

      } else {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) skipping %s\n"), parser.get_current()));
        parser.ignore_arg();
      }
    }
  }

  unsigned int num_pub_processes;
  unsigned int num_pub_participants; // per process
  unsigned int num_writers; // per participant
  unsigned int num_samples; // per writer
  unsigned int num_sub_processes;
  unsigned int num_sub_participants; // in this process
  unsigned int num_readers; // per participant
  bool reliable;
  unsigned int sample_size;
  unsigned int delay_msec;
  unsigned int total_duration_msec;
  bool no_validation;
};

#endif /* ManyToMany_Options_h  */
