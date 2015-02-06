package PerlDDS::Cross_Sync_Common;

use strict;
use FileHandle;
use Socket;
use Sys::Hostname;
use IO::Socket;
use IO::Select;
use strict;

### Constants

use constant CLIENT => 0;
use constant SERVER  => 1;

###############################################################################

### Constructor and Destructor

sub allocate {

    my $proto = shift;
    my $class = ref ($proto) || $proto;

    my ($verbose, $server_port
        , $client_port, $schedule_file) = @_;

    $verbose ||= 0;
    $server_port ||= 12345;
    $client_port ||= 12346;
    $schedule_file ||= "test_list.txt";

    $verbose = 1;

    my $self = {
        VERBOSE => $verbose,
        SERVER_PORT => $server_port,
        CLIENT_PORT => $client_port,
        SCHEDULE_FILE => $schedule_file
        };

    # Check if this test was intended to be run.
    my $env_key = "CROSS_GRP";
    if ($ENV{$env_key}) {
        $self->{TEST_INSTANCE} = $ENV{$env_key};
    }
    else {
        if ($verbose) {
            print STDERR "ENV CROSS_GRP undefined.\n";
        }
        return;
    }

    bless ($self, $class);

    if ($self->{TEST_INSTANCE} && ($self->_initialize () == 0)) {
        return $self;
    }
}

# Blocking API.
# Semantics: Unblocked when either system synched up or error
# Return values:
#  Error:   -1
#  Passive: 0 (client in a client-server relationship)
#  Active:  1 (server )
# If return value is Passive, assume peer ready.
sub wait {
    my $self = shift;
    my $total_timeout = shift || 60 * 60 * 2; # default: 2 hrs in seconds
    my $server_port = shift || 55555; #default port
    my $verbose = $self->{VERBOSE};

    if (!$self->{TEST_INSTANCE}) {
        return -1;
    }

    my $my_name = $self->{SELF};
    if ($verbose) {
        print "Hostname: $my_name\n";
    }
    my $role = $self->{ROLE};

    # sync up here.
    if ($role == CLIENT) {
        if ($verbose) {
            print "Taking client role.\n";
        }

        my $sock;

        if ($verbose) {
            my $peer = $self->{PEER};
            print "client> connecting to $peer:$server_port\n";
        }
        my $remaining_time = $total_timeout;
        while (($remaining_time > 1)  && !$sock) {
            $sock = new IO::Socket::INET (PeerHost => $self->{PEER}
                                          , PeerPort => $server_port
                                          , Proto => 'tcp',);
            if (!$sock) {
                print ". \n";
                $remaining_time -= sleep (5);
            }
        }
        if (!$sock) {
            if ($verbose) {
                print STDERR "Client connection could not be established: $!\n";
            }
            return -1;
        }
        if ($verbose) {
            print "client $my_name> Connection established.\n";
        }

        my $client_port = $self->{CLIENT_PORT};
        my $send_buf = "$my_name $client_port ";
        if ($verbose) {
            print "client> sending \"$send_buf\"\n";
        }
        print $sock "$send_buf\015\012";

        my $buf = <$sock>;
        if ($verbose) {
            print "client> Received \"$buf\"\n";
        }
        my @argv = split (/ +/, $buf);

        if ((scalar(@argv) >= 3) && ($argv[0] =~ /$self->{PEER}/)) {
            if ($verbose) {
                print "client> Peer from $argv[0]:$argv[1] just checked in.\n";
            }
            $self->{SERVER_PORT} = $argv[1];

            if ($verbose) {
                print "client> sending \"$buf\"\n";
            }
            # ACK that the client end is taking off.
            print $sock "$buf \015\012";
            return 0;
        } else {
            if ($verbose) {
                print "client: Closing handle $sock.\n";
            }
            close ($sock);
            return -1;
        }
    }
    else { # server
        if ($verbose) {
            print "Taking server role ($my_name).\n";
        }

        my $main_sock = new IO::Socket::INET (LocalHost => ""
                                              , LocalPort => $server_port
                                              , Listen => 1
                                              , Proto => 'tcp'
                                              , Reuse => 1,);
        if (!$main_sock) {
            if ($verbose) {
                print STDERR "Server socket could not be created: $!\n";
            }
            return -1;
        }

        my $read_set = new IO::Select;
        $read_set->add ($main_sock);

        my $sock;
        my $new_sock;
        while (1) {
            my ($new_readables)
                = IO::Select->select ($read_set
                                      , undef, undef
                                      , $total_timeout);

            if (scalar ($new_readables) == 0) {
                if ($verbose) {
                    print "server> Timed out waiting for client.\n";
                }
                return -1;
            }

            if ($verbose) {
                print "server> Activity detected on some fd.\n";
            }

            foreach $sock (@$new_readables) {
                if ($sock == $main_sock) {
                    if ($verbose) {
                        print "server> Adding a client.\n";
                    }
                    $self->{PEER_FD} = $sock->accept ();
                    $read_set->add ($self->{PEER_FD});
                    $sock->autoflush();
                } else {
                    if ($verbose) {
                        print "server> Reading from a client.\n";
                    }
                    my $buf = <$sock>;
                    if ($verbose) {
                        print "server> received \"$buf\"\n";
                    }
                    my @argv = split (/ +/, $buf);
                    if (($buf =~ /$self->{PEER}/) && (scalar(@argv) >= 2)) {
                        $self->{CLIENT_PORT} = $argv[1];
                        if ($verbose) {
                            print "server> Peer from $argv[0]:$argv[1] just checked in.\n";
                        }
                        return 1;
                    } else {
                        if ($verbose) {
                            print "server> Closing handle $sock.\n";
                        }
                        $read_set->remove ($sock);
                        close ($sock);
                        return -1;
                    }
                }
            }
        }
    }

    # decide the active side
}

# Blocking API.
# I am ready. Will update peer on status.
# Return upon client ACK.
# Return values
#  0  OK
#  -1 Error
sub ready {
    my $self = shift;

    my $verbose = $self->{VERBOSE};
    my $my_name = $self->{SELF};
    my $peer = $self->{PEER_FD};

    my $server_port = $self->{SERVER_PORT};
    my $client_port = $self->{CLIENT_PORT};
    my $send_buf = "$my_name $server_port $client_port ";
    if ($verbose) {
        print "server> sending \"$send_buf\"\n";
    }
    print $peer "$send_buf\015\012";

    my $read_set = new IO::Select;
    $read_set->add ($peer);

    my $wait_timeout = 60 * 5; # max 5 minutes
    while ($wait_timeout > 1) {
        IO::Select->select ($read_set
                            , undef, undef
                            , $wait_timeout);
          if ($verbose) {
              print "server> Activity detected on client fd.\n";
          }

          my $buf = <$peer>;
          if ($verbose) {
              print "server> received \"$buf\"\n";
          }
          if ($buf =~ /$my_name/) {
              return 1;
          } else {
              if ($verbose) {
                  print "server> Client connection closed out.\n";
              }

              $read_set->remove ($peer);
              close ($peer);
              return -1;
          }
      }

    if ($verbose) {
        print "server> Client ACK timed out.\n";
    }
    return -1;
}

sub boot_ports {
    my $self = shift;

    return ($self->{SERVER_PORT}, $self->{CLIENT_PORT});
}

sub peer {
    my $self = shift;

    return $self->{PEER};
}

sub self {
    my $self = shift;

    return $self->{SELF};
}

sub _initialize {
    my $self = shift;

    return $self->_parse_schedule ();
}

sub _parse_schedule {

    my $self = shift;
    my $verbose = $self->{VERBOSE};

    my $in_fh = new FileHandle();

    open ($in_fh, "$self->{SCHEDULE_FILE}");
    if (!$in_fh) {
        if ($verbose) {
            print STDERR "Unable to open file for parsing.\n";
        }
        return -1;
    }
    my @argv;
    while(<$in_fh>) {
        if (!/ +\#/) {
            @argv = split ();
            if ($argv[0] == $self->{TEST_INSTANCE}) {
                last;
            }
        }
    }
    close ($in_fh);

    if (scalar(@argv) < 3) {
        if ($verbose) {
            print "Failed to parse schedule.\n";
        }
        return -1;
    }

    if ($argv[0] != $self->{TEST_INSTANCE}) {
        if ($verbose) {
            print "Failed to find the group instance in the schedule.\n";
        }
        return -1;
    }

    my $my_name = hostname();
    $my_name =~ s/(.*?)\..*/$1/; # get just the machine name
    if ($argv[1] =~ /$my_name/) {
        $self->{ROLE} = CLIENT;
        $self->{PEER} = $argv[2];
        $self->{SELF} = $argv[1];
    } elsif ($argv[2] =~ /$my_name/) {
        $self->{ROLE} = SERVER;
        $self->{PEER} = $argv[1];
        $self->{SELF} = $argv[2];
    } else {
        return -1;
    }

    return 0;
}

sub DESTROY {
    my $self = shift;

    unlink $self->{PUB_INI_TMP};
    unlink $self->{SUB_INI_TMP};
    close ($self->{PEER_FD});
}

1;
