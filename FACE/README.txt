This directory contains only public headers for use by FACE (Future Airborne
Capability Environment) applications.  Those applications expect to
(for example) #include "FACE/TS.hpp", which will resolve to the header in this
directory when $DDS_ROOT is on the preprocessor search path.

Other aspects of OpenDDS's support for FACE:
- Code generation in opendds_idl (source code in dds/idl) with -Gface option
- Library implementation of FACE TSS functions in dds/FACE
