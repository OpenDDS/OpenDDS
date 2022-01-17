# Helper class to change to a new directory and then restore the original
# directory automatically when the object falls out of scope.

package ChangeDir;

use strict;
use warnings;

use Cwd qw(getcwd);

sub cd {
    my $self = shift();
    my $dir = shift();
    my $back = shift() ? ' back' : '';

    if (defined($dir)) {
      print("Changing$back to $dir\n") if $self->{verbose};
      chdir($dir) or die("Couldn't change$back to directory $self->{dir}: $!");
    }
}

sub new {
    my $proto = shift();
    my $class = ref($proto) || $proto;
    my $dir = shift();
    my %opt_args = @_;

    $dir = undef if (defined($dir) && $dir eq '.');

    my $self = bless({
        dir => $dir,
        original_dir => $opt_args{orignal_dir} // getcwd(),
        verbose => $opt_args{verbose},
    }, $class);

    $self->cd($dir);

    return $self;
}

sub DESTROY {
    local($., $@, $!, $^E, $?);
    my $self = shift();
    if (defined($self->{dir}) && $self->{original_dir}) {
      $self->cd($self->{original_dir}, 1);
    }
};

1;
