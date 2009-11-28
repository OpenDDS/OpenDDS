eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}'
    & eval 'exec perl -w -S $0 $argv:q'
    if 0;

# ******************************************************************
#      Author: Chad Elliott
#        Date: 6/25/2008
# Description: A script that updates MPC and C++ source files to use
#              the new dcps_ts.pl output.
#         $Id$
# ******************************************************************

# ******************************************************************
# Pragma Section
# ******************************************************************

use strict;
use FileHandle;
use File::Basename;

# ******************************************************************
# Data Section
# ******************************************************************

my @files;
my %suffix = ('.idl' => 'TypeSupport',
              '.h'   => 'TypeSupportImpl',
              '.cpp' => 'TypeSupportImpl');
              
# ******************************************************************
# Subroutine Section
# ******************************************************************

sub edit_file {
  my($file, $func) = @_;
  my $fh = new FileHandle();
  my $status = 0;

  if (open($fh, $file)) {
    my $lines = &$func($fh);
    close($fh);

    ## Write out the new contents of the file
    if (open($fh, ">$file")) {
      foreach my $line (@$lines) {
        print $fh $line;
      }
      close($fh);
    }
    else {
      print STDERR "ERROR: Unable to write $file\n";
      $status++;
    }
  }
  else {
    print STDERR "ERROR: Unable to read $file\n";
    $status++;
  }

  return $status;
}

sub edit_mpc_file {
  my $fh = shift;
  my $saved = '';
  my(@lines, $line, %suffix_used);

  ## Empty out the files at the start of each new MPC file.
  @files = ();

  while(<$fh>) {
    if (/(.*)\\\s*$/) {
      ## If this is a concatenation line, save it for later
      $saved .= $1;
      $saved =~ s/\s+$/ /;
    }
    else {
      $line = $saved . $_;
      $saved = '';

      ## Just in case this line has a mix of << and >> we need to
      ## preserve it and add it back later.
      my $depends = '';
      my $oline = $line;
      if ($line =~ s/(\s*<<.*)(>>)/$2/) {
        $depends = $1;
        if ($depends =~ s/(\s+)$//) {
          my $spc = $1;
          $line =~ s/(>>)/$spc$1/;
        }
      }
      elsif ($line =~ s/(\s*<<.*)//) {
        $depends = $1;
      }

      ## Reset the suffix_used map upon the entrance of each new
      ## project.  This allows a single use of the generated files
      ## throughout all projects in a single MPC file.
      %suffix_used = () if ($line =~ /^\s*project.*{/);

      ## Check for an idl file that indicates that it will be
      ## generating other files.  We will assume that the user is only
      ## using this on DDS related idl files.
      if ($line =~ /^(\s*(.+\.idl))\s*>>\s*(.+)/) {
        my $f = $1;
        my $key = $2;
        my $files = $3;
        my @tmp = split(/\s+/, $files);

        ## Trim off leading and trailing white-space and pull off the
        ## extension.
        $key =~ s/^\s+$//;
        $key =~ s/\s+$//;
        $key =~ s/\.idl//;

        ## Save the file name and the files it used to create
        push(@files, [$key, \@tmp]);

        ## Add this file to the lines since it won't be added below
        push(@lines, "$f$depends\n");
      }
      else {
        ## Return the original line (in case dependencies were removed)
        $line = $oline;

        ## Go through all of the files entries that we may have
        ## accumulated.  If one of the files are used, we need to
        ## replace it with the new style generated file name (or remove
        ## it completely if the idl file generated multiple files).
        foreach my $ent (@files) {
          my($key, $files) = @$ent;
          foreach my $file (@$files) {
            ## See if we even need to bother with this line using a
            ## simple regular expression.
            if ($line =~ /\b$file\b/) {
              $file =~ /(\.[^\.]+)$/;
              my $ext = $1;
              my $suffix = $suffix{$ext};

              ## Replace the old file with the new generated file name.
              if ($line =~ s/^(\s*).+(\.[^\.]+)$/$1$key$suffix$2/) {
                ## If the suffix for this file and extension have
                ## already been used, we need to empty out the line so
                ## that there are not duplicates.
                $line = '' if ($suffix_used{$key}->{$ext});

                ## We've now seen the file and extension so there's no
                ## reason to stay in the loop.
                $suffix_used{$key}->{$ext} = 1;
                last;
              }
            }
          }
        }
        push(@lines, $line);
      }
    }
  }

  return \@lines;
}

sub edit_source_file {
  my $fh = shift;
  my(@lines, %suffix_used);
  while(<$fh>) {
    ## Go through all of the files entries that we may have
    ## accumulated.  If one of the files are used, we need to
    ## replace it with the new style generated file name (or remove
    ## it completely if the idl file generated multiple files).
    foreach my $ent (@files) {
      my($key, $files) = @$ent;
      foreach my $file (@$files) {
        $file =~ /(.+)(\.[^\.]+)$/;
        my $regex = $1;
        my $ext = $2;

        ## Save the ending portion of the regular expression so that we
        ## can use it in multiple places.
        my $eregex = ($ext eq '.idl' ? '[CS].h' : $ext);

        ## Add the ending portion of the regular expression
        $regex .= $eregex;

        ## See if we even need to bother with this line using a simple
        ## regular expression.
        if (/\b$regex\b/) {
          my $suffix = $suffix{$ext};

          ## Determine which ending we will be adding to the file name.
          ## If we're looking at an idl file, then the ending may be
          ## C.h or S.h.
          my $end = $ext;
          if ($ext eq '.idl' && /($eregex)/) {
            $end = $1;
          }

          ## Replace the include of the old file with the new one
          if ($_ =~ s/^(\s*#\s*include(?:\s*\/\*\*\/)?\s*["<])$regex([">])/$1$key$suffix$end$2/) {
            ## If the suffix for this file and extension have already
            ## been used, we need to empty out the line so that there
            ## are not duplicates.
            $_ = '' if ($suffix_used{$key}->{$ext});

            ## We've now seen the file and extension so there's no
            ## reason to stay in the loop.
            $suffix_used{$key}->{$ext} = 1;
            last;
          }
        }
      }
    }
    push(@lines, $_);
  }

  return \@lines;
}

sub edit {
  my $start  = shift;
  my $status = 0;

  if (-d $start) {
    my $fh = new FileHandle();
    if (opendir($fh, $start)) {
      ## Sort them by extension in reverse order so that the .mpc files
      ## come before .h and .cpp files.
      foreach my $file (sort { my $l = $a;
                               my $r = $b;
                               $l =~ s/.+(\.[^\.]+)$/$1/;
                               $r =~ s/.+(\.[^\.]+)$/$1/;
                               return $r cmp $l;
                             } (grep(!/^\.\.?$/, readdir($fh)))) {
        $status += edit("$start/$file") if ($file ne '.svn' && $file ne 'CVS');
      }
      closedir($fh);
    }
    else {
      print STDERR "ERROR: Unable to process $start\n";
      $status++;
    }
  }
  else {
    if ($start =~ /\.mp[bc]$/) {
      $status += edit_file($start, \&edit_mpc_file);
    }
    elsif ($start =~ /\.(cpp|cxx|cc|C|h|hpp|hxx|hh)$/) {
      $status += edit_file($start, \&edit_source_file);
    }
  }

  return $status;
}

# ******************************************************************
# Main Section
# ******************************************************************

if ($#ARGV == -1) {
  print 'Usage: ', basename($0), " [directories or files]\n\n",
        "Modifies MPC and C++ source files to utilize the new output ",
        "of dcps_ts.pl.\n";
  exit(1);
}

my $status = 0;
foreach my $file (@ARGV) {
  $status += edit($file);
}
exit($status);
