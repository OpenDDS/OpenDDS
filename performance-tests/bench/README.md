# Bench

## Test Controller

### Command Line Arguments

Usage: `test_controller [-h|--help] | TEST_CONTEXT SCENARIO_ID [OPTIONS...]`

- `TEST_CONTEXT`: Path to the directory of the test configurations and artifacts

- `SCENARIO_ID`: Name of the scenario file in the test context without the
  `.json` extension.

- `OPTIONS`:

  - `--domain N`: The DDS Domain to use. The default is 89.

  - `--wait-for-nodes N`: The number of seconds to wait for nodes before
    broadcasting the scenario to them. The default is 10 seconds.

  - `--timeout N`: The number of seconds to wait for a scenario to complete.
    Overrides the value defined in the scenario. If N is 0, there is no
    timeout.

  - `--prealloc-scenario-out PATH`: Instead of running the scenario, write
    directives (in JSON) that would have been sent to the node controllers to
    the file given by this path.

  - `--pretty`: Write the JSON output of `--prealloc-scenario-output` with
    indentation.

  - `--debug-alloc`: Print out a debug version of what is saved with
    `--prealloc-scenario-out` and exit.

  - `--prealloc-scenario-in PATH`: Take result of `--prealloc-scneario-out` and
    use that to run the scenario instead of discovering nodes. This might fail
    if the nodes go offline after the preallocated scenario is saved.

  - `--result-id ID`: Name to store the results under. By default incrementing
    numbers are assigned, but `ID` doesn't have to be a valid number, just a
    valid file name.

  - `--overwrite-result`: Write the result when using `--result-id`, even if it
    would overwrite another result.

## Node Controller

### Command Line Arguments

Usage: `node_controller RUN_MODE [OPTIONS...]`

- `RUN_MODE`:

  - `one-shot`: Exit after running one node configuration.

  - `daemon`: Wait for another node configuration after running one.

  - `daemon-exit-on-error`: Same as `daemon` but will exit if an any error
    occurs.

- `OPTIONS`:

  - `--domain N`: The DDS Domain to use. The default is 89.

  - `--name STRING`: Human friendly name for the node. Will be used by the test
    controller for referring to the node, but otherwise has no effect on
    behavior. Multiple nodes could even have the same name.

## Worker

### Command Line Argument

Usage: `worker CONFIG [OPTIONS...]`

- `CONFIG`: The path to a worker configuration file.

- `OPTIONS`:

  - `--log FILE`: Path to write the worker debug messages and a copy of the
    report to. Will log to `stdout` if not passed.

  - `--report FILE`: Path to write the worker JSON report. If not passed then
    no JSON version of the report will be produced.
