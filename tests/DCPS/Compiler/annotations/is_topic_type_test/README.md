# `is_topic_type` Test

Test of the `@topic`/`@nested`/`@default_nested` logic handled by
`is_topic_type`, located in `dds/idl/be_global.cpp`. Has two subtests, one
where `--default-nested` was passed to `opendds_idl` (`dn`) and one where it
wasn't (`wo_dn`). `-Gitl` is passed in both cases and the ITL files are used by
`run_test.pl` to assert what types should and shouldn't be topic types.
