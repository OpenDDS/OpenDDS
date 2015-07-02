/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ZEROCOPYSEQBASE_H
#define ZEROCOPYSEQBASE_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

//The TAO_IDL compiler-generated code expects to find this constant
//in the in the global namespace.  It's not worth the overhead of a
//TAO_IDL change to fix this right now but it eventually should be in
//the OpenDDS::DCPS namespace.
enum { DCPS_ZERO_COPY_SEQ_DEFAULT_SIZE = 20 };

#endif /* ZEROCOPYSEQBASE_H  */
