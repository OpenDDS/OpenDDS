###############
FlexibleTS Test
###############

This test creates a two processes, one represents an old version of an
application and the other represents the current version of the application.
The old and new versions each have their own IDL which is not compatible
according to XTypes.  Normally these applications would not be able to
communicate, but OpenDDS has an extension to XTypes called FlexibleTypeSupport.

The test uses file-based synchronization between test processes.  Both the old
and new apps publish and subscribe (using distinct topics).  First the old app
publishes on one topic, then when the new app receives that sample it will
publish on a different topic.  Once the old app receives this sample the test
is complete.

To run the test (in the only supported configuration): `run_test.pl rtps`
