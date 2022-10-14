# QoS XML Handler

Optional utility to allow one to define the QoS of DDS entities using XML. Uses
an old coding style because it was previously part of the CIAO DDS4CCM
implementation.

## `dds_qos.hpp`

Note that if `dds_qos.hpp` is regenerated without any intervention, the include
guard will fail the `missing_include_guard` check of lint.pl. The include guard
will have to be modified to pass the check. Also a del_qos_profile operation
has been added manually in the past, on regeneration this has to be added back
manually.
