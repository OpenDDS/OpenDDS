######################
RtpsDurableReplay Test
######################

This test exercises durable replay in a connect-disconnect-connect scenario for RTPS.

There are two participants:

* The Publisher - contains a durable and reliable writer with a short lease duration (5 seconds)
* The Subscriber - contains a durable and reliable reader with a long lease duration (300 seconds)

The test proceeds as follows:

1. The Publisher starts writing a fixed set of instances.
2. The test waits for the Subscriber to receive the instances.
2. Sending is disabled by using the `drop_messages` API for the transports.
3. The Publisher times out in the Subscriber.
4. The test sleeps for 15 seconds to allow the heartbeat fallback to increase.
5. Sending is enabled.
6. The test checks that Subscriber receives the durable instances again in under 6 seconds.

Due to the "one discovery object per domain" limitation in OpenDDS, this is a multi-process test.
