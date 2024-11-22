###############
FlexibleTS Test
###############

This test creates a two "device" processes, one represents an old version of an
application and the other represents the current version of the application.
The old and new versions each have their own IDL which is not compatible
according to XTypes.

A single "controller" process communicates with both devices.  Its IDL is
identical to the new version of the device.  Normally the controller
application would not be able to communicate, with the old device due to its
incompatible IDL, but OpenDDS has an extension to XTypes called
FlexibleTypeSupport.  The device applications don't need to be aware of
(or even compiled with) FlexibleTypeSupport, only the controller uses it.

Both directions of data flow, device to controller and controller to device
are tested.  The controller needs to adapt the data samples it sends to and
receives from the old device to account for differences in schema.

The test uses file-based synchronization between test processes.

To run the test (in the only supported configuration): `run_test.pl rtps`
