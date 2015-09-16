package ConvertFiles;

# ************************************************************
# Description   : Convert a directory tree from UNIX to DOS
# Author        : Chad Elliott
# Create Date   : 4/16/2004
# ************************************************************

# ************************************************************
# Pragmas
# ************************************************************

use strict;
use FileHandle;

# ************************************************************
# Subroutine Section
# ************************************************************

sub new {
  my($class)   = shift;
  my($verbose) = shift;
  my($self)    = bless {'verbose' => $verbose,
                       }, $class;
  return $self;
}


sub convert {
  my($self)       = shift;
  my($releasedir) = shift;
  my($status)     = 1;
  my($error)      = undef;
  my($fh)         = new FileHandle();

  if (opendir($fh, $releasedir)) {
    foreach my $file (grep(!/^\.\.?$/, readdir($fh))) {
      my($full) = "$releasedir/$file";
      if (-d $full) {
        ($status, $error) = $self->convert($full);
      }
      else {
        my($ifh) = new FileHandle();
        if (open($ifh, $full)) {
          my($abort_conversion) = 0;
          my(@lines) = ();
          while(<$ifh>) {
            $_ =~ s/\r?\n$//;
            if ($_ !~ /^[[:ascii:]]*$/) {
              $abort_conversion = 1;
              last;
            }
            push(@lines, $_);
          }
          close($ifh);

          if ($abort_conversion) {
            if ($self->{'verbose'}) {
              print "Not converting binary file: $full\n";
            }
          }
          else {
            if (open($ifh, ">$full")) {
              foreach my $line (@lines) {
                print $ifh "$line\r\n";
              }
              close($ifh);
            }
            else {
              $status = 0;
              $error  = "Conversion failed on $full";
            }
          }
        }
      }

      ## If we couldn't open a directory or couldn't convert a file
      ## we will exit from this loop.
      if (!$status) {
        last;
      }
    }
    closedir($fh);
  }
  else {
    $status = 0;
    $error  = "Unable to open directory $releasedir: $!";
  }

  return $status, $error;
}


1;
