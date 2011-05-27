// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/debug.h"

#include "dds/InfoRepo/FederatorC.h"

#include "ace/OS_main.h"

#include <string>
#include <iostream>


namespace { // Anonymous namespace for file scope.

void
usage_and_exit( int value = 0)
{
  std::cout << std::endl
    << "USAGE: federation <cmd> <args> ..." << std::endl
    << std::endl
    << " The command line is a simplistic command/arguments format.  The first" << std::endl
    << " argument will *always* be the command and following arguments are" << std::endl
    << " parameters used by the command and vary by command." << std::endl
    << std::endl
    << " The supported commands and their arguments are:" << std::endl
    << std::endl
    << "   join     <target> <peer> [ <federationDomain> ]" << std::endl
    << "   leave    <target>" << std::endl
    << "   shutdown <target>" << std::endl
    << std::endl
    << " Where <federationDomain> is an optional domain value to be used in" << std::endl
    << " place of the default federation domain value." << std::endl
    << std::endl
    << " Where <target> and <peer> are endpoint specifications" << std::endl
    << " which can include a hostname and port.  If no hostname is specified," << std::endl
    << " the default 'localhost' will be used." << std::endl
    << std::endl
    << " A connection will be attempted by constructing an IOR as:" << std::endl
    << std::endl
    << "   corbaloc:iiop:<target>/Federator" << std::endl
    << std::endl
    << " The commands perform the following actions:" << std::endl
    << std::endl
    << "   join     - calls the 'join_federation()' method on the destination" << std::endl
    << "              passing the source and federation domain as arguments." << std::endl
    << "              If the federation domain is not given on the command line," << std::endl
    << "              the default federation domain will be used." << std::endl
    << "   leave    - gracefully withdraws the target from the federation and" << std::endl
    << "              then causes its process to terminate.  This will have the" << std::endl
    << "              effect of removing associations which are terminated at" << std::endl
    << "              applications attached to the target repository." << std::endl
    << "   shutdown - shuts down the repository process without withdrawing" << std::endl
    << "              from the federation.  This will have the effect of" << std::endl
    << "              leaving the existing associations in place, and any" << std::endl
    << "              application which had been attached to this repository" << std::endl
    << "              will then failover to another repository within the" << std::endl
    << "              federation." << std::endl
    << "   help     - prints this usage message and halts." << std::endl
    << std::endl
    << "   Any unrecongnized commands or no command will result in the same" << std::endl
    << "   action as the 'help' command." << std::endl
    << std::endl;

  ACE_OS::exit( value);
}

/**
 * @class Options
 *
 * @brief Encapsulate command line arguments.
 *
 * The command line is a simplistic command/arguments format.  The first
 * argument will *always* be the command and following arguments are
 * parameters used by the command and vary by command.
 *
 * See the usage_and_exit() routine for additional detail.
 *
 */
class Options {
  public:
    /// Commands that can be performed by this command.
    enum Command { JOIN, LEAVE, SHUTDOWN};

    /// Construct with command line arguments only.
    Options( int argc, char** argv);

    /// Virtual destructor.
    virtual ~Options() { }

    /// Access the specified action.
    Command command() const { return this->command_; }

    /// Access the specified node.
    const std::string& target() const { return this->target_; }

    /// Access the specified peer.
    const std::string& peer() const { return this->peer_; }

    /// Access the specified federationDomain.
    int federationDomain() const { return this->federationDomain_; }

  private:
    /// Command to perform.
    Command command_;

    /// Node to call on to start connection.
    std::string target_;

    /// Node to pass in as parameter of connection.
    std::string peer_;

    /// Domain value for federation.
    int federationDomain_;
};

Options::Options( int argc, char** argv)
 : federationDomain_( OpenDDS::Federator::DEFAULT_FEDERATIONDOMAIN)
{
  if( --argc) {
    // First argument is the command.
    switch( **++argv) {
      case 'j': case 'J': this->command_ = JOIN;     break;
      case 'l': case 'L': this->command_ = LEAVE;    break;
      case 's': case 'S': this->command_ = SHUTDOWN; break;

      // Unknown command.
      default: std::cout << "Unknown command: " << *argv << std::endl;

      // Deliberately fall through from the default case.
      case 'h': case 'H': usage_and_exit( -8); break;
    }

    // Next argument is the target for all commands.
    if( --argc) this->target_ = *++argv;
    else        usage_and_exit( -9);

    if( this->command_ == JOIN) {
      // Its the join command, next argument is the peer.
      if( --argc) this->peer_ = *++argv;
      else        usage_and_exit( -10);

      // Optional federation domain value.
      if( --argc) this->federationDomain_ = ACE_OS::strtol( *++argv, 0, 0);
    }

  } else {
    usage_and_exit( -7);
  }
}

} // End of anonymous namespace.

int
main( int argc, char** argv)
{
  int status = 0;
  try {
    // Initialize an ORB.
    CORBA::ORB_var orb = CORBA::ORB_init( argc, argv);

    // Grab our information from the command line.
    Options options( argc, argv);

    // Resolve the target reference.
    std::string iorString( "corbaloc:iiop:");
    iorString += options.target();
    iorString += OpenDDS::Federator::FEDERATOR_IORTABLE_KEY;
    if( OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG(( LM_INFO,
        ACE_TEXT("(%P|%t) INFO: federation: ")
        ACE_TEXT("attempting to resolve and connect to repository at: %s.\n"),
        iorString.c_str()
      ));
    }

    CORBA::Object_var obj = orb->string_to_object( iorString.c_str() );

    OpenDDS::Federator::Manager_var target
      = OpenDDS::Federator::Manager::_narrow( obj.in() );
    if( CORBA::is_nil( target.in())) {
      ACE_ERROR(( LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: federation: could not narrow %s.\n"),
        iorString.c_str()
      ));
      return -4;
    }

    // Join command needs the peer reference resolved as well.
    OpenDDS::Federator::Manager_var peer;
    if( options.command() == Options::JOIN) {
      iorString  = "corbaloc:iiop:";
      iorString += options.peer();
      iorString += OpenDDS::Federator::FEDERATOR_IORTABLE_KEY;
      if( OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG(( LM_INFO,
          ACE_TEXT("(%P|%t) INFO: federation: ")
          ACE_TEXT("attempting to resolve and connect to repository at: %s.\n"),
          iorString.c_str()
        ));
      }

      obj = orb->string_to_object( iorString.c_str() );

      peer = OpenDDS::Federator::Manager::_narrow( obj.in() );
      if( CORBA::is_nil( peer.in())) {
        ACE_ERROR(( LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: federation: could not narrow %s.\n"),
          iorString.c_str()
        ));
        return -5;
      }
    }

    switch( options.command()) {
      case Options::JOIN:
        {
          if( OpenDDS::DCPS::DCPS_debug_level > 0) {
            ACE_DEBUG(( LM_INFO,
              ACE_TEXT("(%P|%t) INFO: federation: federating.\n")
            ));
          }
          target->join_federation( peer.in(), options.federationDomain());
        }
        break;

      case Options::LEAVE:
        {
          if( OpenDDS::DCPS::DCPS_debug_level > 0) {
            ACE_DEBUG(( LM_INFO,
              ACE_TEXT("(%P|%t) INFO: federation: leaving and shutting down.\n")
            ));
          }
          target->leave_and_shutdown();
        }
        break;

      case Options::SHUTDOWN:
        {
          if( OpenDDS::DCPS::DCPS_debug_level > 0) {
            ACE_DEBUG(( LM_INFO,
              ACE_TEXT("(%P|%t) INFO: federation: shutting down.\n")
            ));
          }
          target->shutdown();
        }
        break;

      default:
        ACE_ERROR(( LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: federation: unknown command requested.\n ")
        ));
        usage_and_exit( -6);
        break;
    }

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception ("(%P|%t) ABORT: federation: CORBA problem detected.\n");
    status = -1;

  } catch (const std::exception& ex) {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ABORT: federation: %s exception caught in main().\n"), ex.what()));
    status = -2;

  } catch(...) {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ABORT: federation: unspecified exception caught in main() - panic.\n")));
    status = -3;

  }

  return status;
}

