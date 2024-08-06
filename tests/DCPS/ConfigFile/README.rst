###############
ConfigFile Test
###############

This test has two processes.

The first process parses an environment variable, config file, and command line argument and checks that the configuration was parsed correctly.
The second process parses two config files with `-DCPSSingleConfigFile 0` and checks that both were parsed and that settings from the last overwrite the settings from the first.

Run th test with `./run_test.pl`.
