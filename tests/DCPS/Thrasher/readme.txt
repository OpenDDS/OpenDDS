The Thrasher test is designed to test the stability and robustness of OpenDDS in scenarios that 1 to P publisher participant(s) send(s) the same number of 1 to M messages to a single subscriber participant in the same domain. Currently, P = 64, and M = 1024.

The aspects to be tested within a predefined time limit:

1. Each publisher has successfully been created and configured, sent the required messages to the subscriber, and cleanly exited.

2. The subscriber has successfully received exactly the same number of messages from each publisher, and the test cleanly exited.

3. The rtps discovery and durability are tested with these configurations:
3.1 all the defined publisher P and M settings (single, ..., low, medium, high, aggressive) 
3.2 3.1 plus rtps
3.3 3.1 plus durable
3.4 3.1 plus rtps and durable
3.5 If configured with durable, publishers wait for match after sending messages, otherwise before.

4. Multiple publishers publish concurrently to simulate the real world communication dynamic.
