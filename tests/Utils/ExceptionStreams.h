#ifndef EXCEPTION_STREAMS_H
#define EXCEPTION_STREAMS_H

//#ifdef ACE_LACKS_IOSTREAMS_TOTALLY
// In case we are lacking iostreams, we will want to be able to stream to 
// standard iostreams in tests

std::ostream& operator<< (std::ostream &os, const CORBA::Exception &e)
{
    os << e._name () << " (" << e._rep_id () << ')';
    return os;
}

std::ostream& operator<< (std::ostream &os, const CORBA::Exception *e)
{
    os << e->_name () << " (" << e->_rep_id () << ')';
    return os;
}

//#endif

#endif

