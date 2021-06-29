The Thrasher test is designed to test the stability and robustness of OpenDDS
in scenarios where each of the specified number of publisher participants sends
the specified number of messages to a single subscriber participant in the same
domain.

The aspects to be tested within a predefined time limit:

1. Each publisher has successfully been created and configured,
   sent the required messages to the subscriber, and cleanly exited.

2. The subscriber has successfully received exactly the same number of messages
   from each publisher, and the test cleanly exited.

3. The rtps discovery and durability are tested with these configurations:
3.1 the specified numbers of publishers and messages (single, ..., high, aggressive)
3.2 3.1 plus rtps
3.3 3.1 plus durable
3.4 3.1 plus rtps and durable
3.5 If configured with/without durable, publishers wait for match after/before
    sending messages.

4. Multiple publishers publish non-optimally without sleeps or yielding.
