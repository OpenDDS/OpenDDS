#############
Observer Test
#############

This test checks support for the Observer interface (`dds/DCPS/Observer.h`).
It is a single-process test that uses a distributed condition set.

The test creates a participant, publisher, writer, subscriber, and reader and installs an observer for each.
The test checks that the appropriate observer is returned for various masks.
Then, it exercises the different methods in the Observer interface.
Each observer posts conditions to the distributed condition set and the test driver checks for these conditions.

To run the tests: `./run_test.pl`

The test will print out the samples received by the Observers if compiled with support JSON.
