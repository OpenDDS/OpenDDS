########################
ConcurrentAuthLimit Test
########################

This test is to test the ``MaxParticipantsInAuthentication`` parameter.

The creates one particpant and two SPDP writers.  The writers send an
SPDP message to the particpant to start authentication.

The test has two modes:

* By default, MaxParticipantsInAuthentication is set to 1.  In this
  mode, the test checks that authentication does not start for the
  second SPDP writer.
* When given the ``no_limit`` flag,
  ``MaxParticipantsInAuthentication`` is unset and the test checks
  that authentication starts for both SPDP writers.

This is a single-process test.
