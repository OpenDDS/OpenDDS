##################
CheckInstance Test
##################

This test checks the following methods in the AccessControl plugin:

* check_local_datawriter_register_instance
* check_local_datawriter_dispose_instance
* check_remote_datawriter_register_instance
* check_remote_datawriter_dispose_instance

This is a single-process test that creates two participants.  One
participant uses a custom plugin to exercise both positive and
negative scenarios.
