/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_VERSION_H
#define OPENDDS_VERSION_H

#define OPENDDS_MAJOR_VERSION 3
#define OPENDDS_MINOR_VERSION 21
#define OPENDDS_MICRO_VERSION 0
#define OPENDDS_VERSION_METADATA "dev"
#define OPENDDS_IS_RELEASE 0
#define OPENDDS_VERSION "3.21.0-dev"

#define OPENDDS_VERSION_AT_LEAST(MAJOR, MINOR, MICRO) (\
  (OPENDDS_MAJOR_VERSION > (MAJOR)) || \
  (OPENDDS_MAJOR_VERSION == (MAJOR) && OPENDDS_MINOR_VERSION >= (MINOR)) || \
  (OPENDDS_MAJOR_VERSION == (MAJOR) && OPENDDS_MINOR_VERSION == (MINOR) && \
    (OPENDDS_MICRO_VERSION >= (MICRO)) \
  ) \
)

#define OPENDDS_VERSION_EXACTLY(MAJOR, MINOR, MICRO) (OPENDDS_MAJOR_VERSION == (MAJOR) && \
  OPENDDS_MINOR_VERSION == (MINOR) && OPENDDS_MICRO_VERSION == (MICRO))

#define OPENDDS_VERSION_LESS_THAN(MAJOR, MINOR, MICRO) \
  !OPENDDS_VERSION_AT_LEAST((MAJOR), (MINOR), (MICRO))

// NOTE: These are deprecated
// lint.pl ignores nonprefixed_public_macros on next line
#define DDS_MAJOR_VERSION (OPENDDS_MAJOR_VERSION)
// lint.pl ignores nonprefixed_public_macros on next line
#define DDS_MINOR_VERSION (OPENDDS_MINOR_VERSION)
// lint.pl ignores nonprefixed_public_macros on next line
#define DDS_MICRO_VERSION (OPENDDS_MICRO_VERSION)
// lint.pl ignores nonprefixed_public_macros on next line
#define DDS_VERSION (OPENDDS_VERSION)

#endif // OPENDDS_VERSION_H
