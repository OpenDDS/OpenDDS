# Bench

## Test Controller

### Command Line Arguments

- `--domain N`: The DDS Domain to use. The default is 89.

- `--wait-for-nodes N`: The number of seconds to wait for nodes before
  broadcasting the scenario to them. The default is 10 seconds.

- `--wait-for-reports N`: The number of seconds to wait for a report to come in
  before timing out. The default is 120 seconds.

## Node Controller

### Command Line Arguments

- `--domain N`: The DDS Domain to use. The default is 89.

- `--name STRING`: Human friendly name for the node. Will be used by the test
  controller for referring to the node, but otherwise has no effect on
  behavior. Multiple node could even have the same name.
