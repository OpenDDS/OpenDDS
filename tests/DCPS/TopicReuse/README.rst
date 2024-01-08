###############
TopicReuse Test
###############

This test checks that
* `delete_topic` fails when the topic is still in use (2.2.2.2.1.6)
* `delete_topic` fails if called for a participant that didn't create the topic (2.2.2.2.1.6)
* `find_topic` increments the number of times `delete_topic` must be called (2.2.2.2.1.11)
* `create_datawriter` fails if the topic does not belong to the same participant (2.2.2.4.1.5)
* `create_datareader` fails if the topic does not belong to the same particpant (2.2.2.5.2.5)

This test creates a single process.

To run the tests with RTPS: `./run_test.pl`
