#ifndef EXCEPTION_STREAMS_H
#define EXCEPTION_STREAMS_H

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

