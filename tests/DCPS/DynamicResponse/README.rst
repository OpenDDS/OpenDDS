####################
DynamicResponse Test
####################

This test creates two processes called the origin and the responder.

The origin creates three topics
- a struct with CompleteTypeObject
- a union with CompleteTypeObject
- a struct without a CompleteTypeObject

The availability of the CompleteTypeObject means a DynamicType is available in discovery.

The basic flow is
1. The origin creates three topics.
2. The responder discovers the three topics and confirms if the DynamicType is available.
3. The origin writes samples.
4. The responder reads the (dynamic) samples, modifies them, and writes them.
5. The origin reads the samples.

This test uses the file-based Distributed Condition Set.

To run the test: `./run_test.pl`
