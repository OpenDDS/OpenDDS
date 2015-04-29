#ifndef EXCEPTION_STREAMS_H
#define EXCEPTION_STREAMS_H

//In case we are lacking iostreams due to Safety Profile, include header
//for tests to still have access to iostreams
#ifdef OPENDDS_SAFETY_PROFILE
#include <iostream>
#endif //OPENDDS_SAFETY_PROFILE

#ifdef ACE_LACKS_IOSTREAM_TOTALLY

// In case we are lacking iostreams, we will want to be able to stream to
// standard iostreams in tests
namespace CORBA {

inline std::ostream& operator<< (std::ostream &os, const CORBA::Exception &e)
{
    os << e._name () << " (" << e._rep_id () << ')';
    return os;
}

inline std::ostream& operator<< (std::ostream &os, const CORBA::Exception *e)
{
    os << e->_name () << " (" << e->_rep_id () << ')';
    return os;
}

}
#endif

#endif

