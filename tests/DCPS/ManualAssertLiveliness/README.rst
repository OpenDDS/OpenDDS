###########################
ManualAssertLiveliness Test
###########################

This test is to test the MANUAL_BY_PARTICIPANT_LIVELINESS_QOS and
MANUAL_BY_TOPIC_LIVELINESS_QOS support.

The test creates 4 writers that associate with 1 reader.  Two writers
are created with MANUAL_BY_PARTICIPANT_LIVELINESS_QOS and two are
created with MANUAL_BY_TOPIC_LIVELINESS_QOS.  They have the same lease
duration. The first MANUAL_BY_PARTICIPANT_LIVELINESS_QOS writer
asserts liveliness via DomainParticipant::assert_liveliness call and
the second MANUAL_BY_PARTICIPANT_LIVELINESS_QOS writer asserts
liveliness via write().  The first MANUAL_BY_TOPIC_LIVELINESS_QOS
writer asserts liveliness via DataWriter::assert_liveliness and the
second MANUAL_BY_TOPIC_LIVELINESS_QOS writer asserts liveliness via
write().  The operations are performed every lease duration.

This is a single-process test.

The test comes in two modes.  In the default mode, liveliness is not
lost.  The subscriber expects 8 callbacks: 2 for each writer when it
becomes alive and when it becomes dead.  In the "lost" mode,
liveliness is lost in the middle of the test.  The subscriber expects
16 callbacks: 4 for each writer when it becomes alive and when it
becomes dead.
