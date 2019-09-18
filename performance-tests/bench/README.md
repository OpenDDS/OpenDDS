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
  behavior. Multiple nodes could even have the same name.

## Worker

### Command Line Argument

- Takes one required argument that's the path to a worker configuration file.

- `--log FILE`: Path to write the worker debug messages and a copy of the
  report to. Will log to `stdout` if not passed.

- `--report FILE`: Path to write the worker JSON report. If not passed then no
  JSON version of the report will be produced.
