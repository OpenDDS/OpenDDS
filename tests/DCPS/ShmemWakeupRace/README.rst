######################
SHM Wakeup Race MWE
######################

This is a two-process minimum working example for an OpenDDS shared-memory
transport wakeup race. It uses only public DDS APIs, RTPS discovery, and a
best-effort writer and reader.

The ping process creates a writer for the ping topic and a reader for the pong
topic. The pong process creates a reader for ping and a writer for pong. Ping
waits until its writer observes one matched reader, writes exactly one sample,
and waits a bounded time for its echo. A missing request or reply makes the
test fail.

Run one trial with::

  perl run_test.pl

Run multiple fresh process pairs to measure the failure rate with::

  perl run_test.pl trials=20

The test deliberately sends no second application sample. In the investigated
failure mode, a later SHM notification can make an earlier stranded sample
visible and hide the problem.

This MWE is not a deterministic CI regression test. Public DDS APIs do not
control the internal ordering between a remote SHM notification and local
DataLink registration, so a passing trial is a non-reproduction rather than
evidence that the transport is fixed.

Best-effort QoS does not guarantee delivery, so this test alone does not claim
an OMG DDS reliability-contract violation. It demonstrates an association-
adjacent missing request or reply in a same-host SHM ping/pong exchange after
the request writer has observed a match and accepted its write.
