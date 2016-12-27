/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/debug.h"

#include "dds/InfoRepo/FederatorC.h"

#include "ace/Arg_Shifter.h"
#include "ace/OS_main.h"
#include "ace/OS_NS_string.h"

#include <string>
#include <iostream>

namespace { // Anonymous namespace for file scope.

void
usage_and_exit(int value = 0)
{
  std::cout << std::endl
            << "USAGE: repoctl [opts] <cmd> <args> ..." << std::endl
            << std::endl
            << " The command line is a simplistic command/arguments format.  The first" << std::endl
            << " argument will *always* be the command and following arguments are" << std::endl
            << " parameters used by the command and vary by command." << std::endl
            << std::endl
            << " The following options may be passed to any of the commands:" << std::endl
            << std::endl
            << "   -h       - prints this usage message." << std::endl
            << "   -v       - verbose output." << std::endl
            << std::endl
            << " The supported commands and their arguments are:" << std::endl
            << std::endl
            << "   join     <target> <peer> [ <federationDomain> ]" << std::endl
            << "   kill     <target>" << std::endl
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
            << "   corbaloc:iiop:<target>/DCPSInfoRepo, or:" << std::endl
            << "   corbaloc:iiop:<target>/Federator" << std::endl
            << std::endl
            << " The commands perform the following actions:" << std::endl
            << std::endl
            << "   join     - calls the 'join_federation()' method on the destination" << std::endl
            << "              passing the source and federation domain as arguments." << std::endl
            << "              If the federation domain is not given on the command line," << std::endl
            << "              the default federation domain will be used." << std::endl
            << "   kill     - shuts down the repository process." << std::endl
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
            << std::endl
            << "   Any unrecongnized commands or no command will result in the same" << std::endl
            << "   action as the '-h' option." << std::endl
            << std::endl;

  ACE_OS::exit(value);
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
  enum Command { JOIN, KILL, LEAVE, SHUTDOWN };

  /// Construct with command line arguments only.
  Options(int argc, ACE_TCHAR** argv);

  virtual ~Options() { }

  /// Access the commmand line invocation name.
  const std::string& name() const {
    return this->name_;
  }

  /// Access the verbose option.
  const bool& verbose() const {
    return this->verbose_;
  }

  /// Access the specified action.
  Command command() const {
    return this->command_;
  }

  /// Access the specified node.
  const std::string& target() const {
    return this->target_;
  }

  /// Access the specified peer.
  const std::string& peer() const {
    return this->peer_;
  }

  /// Access the specified federationDomain.
  int federationDomain() const {
    return this->federationDomain_;
  }

private:
  /// Process name we were started with (the command line command).
  std::string name_;

  /// Verbose output.
  bool verbose_;

  /// Command to perform.
  Command command_;

  /// Node to call on to start connection.
  std::string target_;

  /// Node to pass in as parameter of connection.
  std::string peer_;

  /// Domain value for federation.
  int federationDomain_;
};

Options::Options(int argc, ACE_TCHAR** argv)
  : verbose_(false)
  , federationDomain_(OpenDDS::Federator::DEFAULT_FEDERATIONDOMAIN)
{
  this->name_ = ACE_TEXT_ALWAYS_CHAR(argv[0]);

  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left()) {
    const ACE_TCHAR *arg = arg_shifter.get_current();

    if (*arg == ACE_TEXT('-')) {
      switch (*++arg) {
      case 'h':
        usage_and_exit(-8);

      case 'v':
        this->verbose_ = true;
      }

      arg_shifter.consume_arg();
    }

    arg_shifter.ignore_arg();
  }

  if (--argc) {
    // First argument is the command.
    switch (**++argv) {
    case ACE_TEXT('j'):
    case ACE_TEXT('J'):
      this->command_ = JOIN;
      break;
    case ACE_TEXT('k'):
    case ACE_TEXT('K'):
      this->command_ = KILL;
      break;
    case ACE_TEXT('l'):
    case ACE_TEXT('L'):
      this->command_ = LEAVE;
      break;
    case ACE_TEXT('s'):
    case ACE_TEXT('S'):
      this->command_ = SHUTDOWN;
      break;

      // Unknown command.
    default:
      std::cout << "Unknown command: " << *argv << std::endl;
      usage_and_exit(-8);
    }

    // Next argument is the target for all commands.
    if (--argc) this->target_ = ACE_TEXT_ALWAYS_CHAR(*++argv);

    else        usage_and_exit(-9);

    if (this->command_ == JOIN) {
      // Its the join command, next argument is the peer.
      if (--argc) this->peer_ = ACE_TEXT_ALWAYS_CHAR(*++argv);

      else        usage_and_exit(-10);

      // Optional federation domain value.
      if (--argc) this->federationDomain_ = ACE_OS::strtol(*++argv, 0, 0);
    }

  } else {
    usage_and_exit(-7);
  }
}

} // End of anonymous namespace.

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  int status = 0;
  const char* exename = "repoctl";

  try {
    // Initialize an ORB.
    CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);

    // Grab our information from the command line.
    Options options(argc, argv);
    exename = ACE_OS::strdup( options.name().c_str());

    OpenDDS::DCPS::DCPSInfo_var ir;
    OpenDDS::Federator::Manager_var target;
    OpenDDS::Federator::Manager_var peer;

    if (options.command() == Options::KILL) {
      // Resolve the ir reference.
      std::string iorString("corbaloc:iiop:");
      iorString += options.target();
      iorString += "/";
      iorString += OpenDDS::Federator::REPOSITORY_IORTABLE_KEY;

      if (options.verbose()) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) INFO: %C: ")
                   ACE_TEXT("attempting to resolve and connect to repository at: %C.\n"),
                   exename,
                   iorString.c_str()));
      }

      CORBA::Object_var obj = orb->string_to_object(iorString.c_str());

      ir = OpenDDS::DCPS::DCPSInfo::_narrow(obj.in());

      if (CORBA::is_nil(ir.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: %C: could not narrow %C.\n"),
                   exename,
                   iorString.c_str()));
        return -4;
      }

    } else {
      // Resolve the target reference.
      std::string iorString("corbaloc:iiop:");
      iorString += options.target();
      iorString += "/";
      iorString += OpenDDS::Federator::FEDERATOR_IORTABLE_KEY;

      if (options.verbose()) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) INFO: %C: ")
                   ACE_TEXT("attempting to resolve and connect to repository at: %C.\n"),
                   exename,
                   iorString.c_str()));
      }

      CORBA::Object_var obj = orb->string_to_object(iorString.c_str());

      target = OpenDDS::Federator::Manager::_narrow(obj.in());

      if (CORBA::is_nil(target.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: %C: could not narrow %C.\n"),
                   exename,
                   iorString.c_str()));
        return -5;
      }

      // Join command needs the peer reference resolved as well.
      if (options.command() == Options::JOIN) {
        iorString  = "corbaloc:iiop:";
        iorString += options.peer();
        iorString += "/";
        iorString += OpenDDS::Federator::FEDERATOR_IORTABLE_KEY;

        if (options.verbose()) {
          ACE_DEBUG((LM_INFO,
                     ACE_TEXT("(%P|%t) INFO: %C: ")
                     ACE_TEXT("attempting to resolve and connect to repository at: %C.\n"),
                     exename,
                     iorString.c_str()));
        }

        obj = orb->string_to_object(iorString.c_str());

        peer = OpenDDS::Federator::Manager::_narrow(obj.in());

        if (CORBA::is_nil(peer.in())) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: %C: could not narrow %C.\n"),
                     exename,
                     iorString.c_str()));
          return -6;
        }
      }
    }

    switch (options.command()) {
    case Options::JOIN: {
      if (options.verbose()) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) INFO: %C: federating.\n"), exename));
      }

      target->join_federation(peer.in(), options.federationDomain());
    }
    break;

    case Options::LEAVE: {
      if (options.verbose()) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) INFO: %C: leaving and shutting down.\n"), exename));
      }

      target->leave_and_shutdown();
    }
    break;

    case Options::SHUTDOWN: {
      if (options.verbose()) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) INFO: %C: shutting down.\n"), exename));
      }

      target->shutdown();
    }
    break;

    case Options::KILL: {
      if (options.verbose()) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) INFO: %C: shutting down.\n"), exename));
      }

      ir->shutdown();
    }
    break;

    default:
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: %C: unknown command requested.\n "), exename));
      usage_and_exit(-7);
      break;
    }

  } catch (const CORBA::Exception& ex) {
    std::string message = "(%P|%t) ABORT: ";
    message += exename;
    message += " : CORBA problem detected.\n";
    ex._tao_print_exception(message.c_str());
    status = -1;

  } catch (const std::exception& ex) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ABORT: %C: %C exception caught in main().\n"),
               ex.what(), exename));
    status = -2;

  } catch (...) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ABORT: %C: unspecified exception caught in main() - panic.\n"),
               exename));
    status = -3;

  }

  return status;
}
