#!/usr/bin/env perl

# Try to find issues with build_and_test.yml
# It requires the YAML cpan module

use strict;
use warnings;

use FindBin;

use YAML qw();

my $error_jobs = 0;

my $gha = YAML::LoadFile("$FindBin::Bin/build_and_test.yml");
for my $job_name (keys(%{$gha->{jobs}})) {
  my $job = $gha->{jobs}->{$job_name};
  # print("#### Job $job_name\n");
  my $expected_matcher;
  my @expected_build_commands = ();
  if ($job->{'runs-on'} =~ /^windows/) {
    $expected_matcher = 'msvc-problem-matcher';
    push(@expected_build_commands, qr/(?<!\w)msbuild/);
  }
  elsif ($job->{'runs-on'} =~ /^ubuntu|macos/) {
    $expected_matcher = 'gcc-problem-matcher';
    push(@expected_build_commands, qr/(?<!\w)make(?!( install|include))/);
  }
  else {
    die("Job $job_name has unexpected runs-on: $job->{'runs-on'}");
  }
  my $expected_matcher_re = quotemeta($expected_matcher);
  push(@expected_build_commands, qr/(?<!\w)cmake --build/, qr/docker build/);

  my $job_has_problem_matcher = 0;
  my $job_has_build_command = 0;
  my $step_i = 0;
  my $error_steps = 0;
  for my $step (@{$job->{steps}}) {
    $step_i += 1;
    if (exists($step->{uses})) {
      if ($step->{uses} =~ $expected_matcher_re) {
        if ($job_has_problem_matcher) {
          print STDERR ("Job $job_name has multiple problem matchers\n");
          $error_steps = 1;
        }
        else {
          $job_has_problem_matcher = 1;
          if ($job_has_build_command) {
            print STDERR ("Job $job_name has problem matcher but it's too late! Move it up!\n");
            $error_steps = 1;
          }
        }
      }
    }
    elsif (exists($step->{run})) {
      die("Expected $job_name step $step_i to have name!") unless (exists($step->{name}));
      my $what = "Job $job_name step \"$step->{name}\"";
      # print("---- $what\n");

      my $avoids_problem_matcher = 0;
      if ($step->{name} =~ /setup gtest/i) {
        $avoids_problem_matcher = 1;
      }
      elsif ($step->{name} =~ /build ace.*tao/i) {
        $avoids_problem_matcher = 1;
      }
      elsif ($step->{name} =~ /build openssl/i) {
        $avoids_problem_matcher = 1;
      }

      if ($avoids_problem_matcher) {
        if ($job_has_problem_matcher) {
          print STDERR ("$what shouldn't have a problem matcher, remove it or move it down!\n");
          $error_steps = 1;
        }
        next;
      }

      my $build_command_expected = 0;
      if ($step->{name} =~ /^build/i) {
        $build_command_expected = 1;
      }
      elsif ($step->{name} =~ /(?<!\w)make(?! install)/i) {
        $build_command_expected = 1;
      }

      my $step_has_build_command = 0;
      for my $expected_build_command (@expected_build_commands) {
        if ($step->{run} =~ /($expected_build_command)/) {
          $job_has_build_command = 1;
          $step_has_build_command = 1;
          die("$what unexpected build command: based on run string: $1")
            unless ($build_command_expected);
          if (!$job_has_problem_matcher) {
            print STDERR ("$what needs $expected_matcher, based on string found in run: $1\n");
            $error_steps += 1;
          }
          last;
        }
      }
      die("$what expected to have a build command based on step name")
        if (!$step_has_build_command && $build_command_expected);
    }
    else {
      die("$job_name step index $step_i doesn't have use or run");
    }
  }

  $error_jobs += 1 if ($error_steps);
}

if ($error_jobs) {
  print STDERR ("ERROR: $error_jobs out of ", scalar(keys(%{$gha->{jobs}})), " jobs have has issues\n");
  exit(1);
}
