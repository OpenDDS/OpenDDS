###########################
SHM Post-Match Delivery MWE
###########################

This is a two-process minimum working example for an OpenDDS shared-memory
transport delivery failure after endpoint matching. It uses only public DDS
APIs, RTPS discovery, and best-effort writers and readers. An RTPS/UDP control
uses the same executable, discovery configuration, QoS, and timing.

The ping process creates a writer for the ping topic and a reader for the pong
topic. The pong process creates a reader for ping and a writer for pong. Both
processes wait for their writer and reader matches, followed by a 1.2-second
settle interval. Ping then writes exactly one sample and waits a bounded time
for its echo. A missing request or reply makes the test fail.

Generate and build the test from this directory with::

  source ../../../setenv.sh
  "$MPC_ROOT/mpc.pl" -type gnuace ShmemWakeupRace.mpc
  make -f GNUmakefile.ShmemWakeupRace_Publisher
  make -f GNUmakefile.ShmemWakeupRace_Subscriber

Run one trial with::

  perl run_test.pl transport=shmem

Run multiple fresh process pairs to measure the failure rate with::

  perl run_test.pl transport=shmem trials=20

Run the transport control with::

  perl run_test.pl transport=rtps_udp trials=20

On Linux, repeated SHM trials may leave detached System V shared-memory
segments. After running many trials, inspect them with ``ipcs -m`` and remove
only segments known to belong to this test. Other applications on the same
host may also be using System V shared memory.

A healthy control reports ``failed trials: 0/N`` and exits with status zero.
The SHM defect is reproduced when one or more fresh process pairs time out
after both directions report matched endpoints. The launcher reports the
number of failed trials and exits nonzero in that case.

The test deliberately sends no second application sample. A later SHM
notification could make an earlier stranded sample visible and hide the
observed timeout.

This MWE is not a deterministic CI regression test. Public DDS APIs do not
control the internal ordering between a remote SHM notification and local
DataLink registration, so a passing trial is a non-reproduction rather than
evidence that the transport is fixed.

Best-effort QoS does not guarantee delivery, so this test alone does not claim
an OMG DDS reliability-contract violation. It demonstrates an association-
adjacent missing request or reply in a same-host SHM ping/pong exchange after
both directions have observed endpoint matches and the request writer has
accepted its write. The RTPS/UDP control distinguishes this from a generic
startup or bidirectional-association failure.
