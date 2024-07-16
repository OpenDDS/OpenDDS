package IDLBase;

# ************************************************************
# Description   : Assist in determining the output from idl2jni.
#                 Much of the file processing code was lifted
#                 directly from the IDL compiler for opalORB.
# Author        : Chad Elliott
# Create Date   : 7/1/2008
# ************************************************************

# ************************************************************
# Pragmas
# ************************************************************

use strict;

use CommandHelper;
use FileHandle;

our @ISA = qw(CommandHelper);

# ************************************************************
# Data Section
# ************************************************************

my %types  = ('const'           => 0x01,
              'enum'            => 0x07,
              'native'          => 0x06,
              'struct'          => 0x07,
              'union'           => 0x07,
              'interface'       => 0x1f,
              'local interface' => 0x2f,
              'typedef'         => 0x06,
              'simple typedef'  => 0x04,
              );

my %idl_keywords = ('abstract' => 1,
                    'any' => 1,
                    'attribute' => 1,
                    'boolean' => 1,
                    'case' => 1,
                    'char' => 1,
                    'component' => 1,
                    'const' => 3,
                    'context' => 1,
                    'custom' => 1,
                    'default' => 1,
                    'double' => 1,
                    'emits' => 1,
                    'enum' => 2,
                    'eventtype' => 1,
                    'exception' => 2,
                    'factory' => 1,
                    'FALSE' => 1,
                    'finder' => 1,
                    'fixed' => 1,
                    'float' => 1,
                    'getraises' => 1,
                    'home' => 1,
                    'import' => 1,
                    'in' => 1,
                    'inout' => 1,
                    'interface' => 2,
                    'local' => 4,
                    'long' => 1,
                    'manages' => 1,
                    'module' => 2,
                    'multiple' => 1,
                    'native' => 3,
                    'Object' => 1,
                    'octet' => 1,
                    'oneway' => 1,
                    'out' => 1,
                    'primarykey' => 1,
                    'private' => 1,
                    'provides' => 1,
                    'public' => 1,
                    'publishes' => 1,
                    'raises' => 1,
                    'readonly' => 1,
                    'sequence' => 1,
                    'setraises' => 1,
                    'short' => 1,
                    'string' => 1,
                    'struct' => 2,
                    'supports' => 1,
                    'switch' => 1,
                    'TRUE' => 1,
                    'truncatable' => 1,
                    'typedef' => 3,
                    'typeid' => 1,
                    'typeprefix' => 1,
                    'union' => 2,
                    'unsigned' => 1,
                    'uses' => 1,
                    'ValueBase' => 1,
                    'valuetype' => 2,
                    'void' => 1,
                    'wchar' => 1,
                    'wstring' => 1,
                   );

my $tsreg  = 'TypeSupport\.idl';

# ************************************************************
# "Friendly" interface for TYPESUPPORTHelper.pm
# ************************************************************

sub do_cached_parse {
  my($self, $file, $flags) = @_;

  # By default opendds_idl considers all types not to be topic types (aka
  # "nested") unless --no-default-nested was passed or annotations say
  # otherwise.
  $self->{'default_nested'} = 1;

  ## Set up the macros and include paths supplied in the command flags
  my %macros = (
    __OPENDDS_IDL => 1,
    __OPENDDS_MPC => 1,
    OPENDDS_HIDE_DYNAMIC_DATA => 1,
  );
  my %mparams;
  my @include;
  if (defined $flags) {
    if (-e "$ENV{TAO_ROOT}/tao/idl_features.h") {
      $macros{"__TAO_IDL_FEATURES"} = "\"tao/idl_features.h\"";
      $macros{"TAO_IDL_HAS_EXPLICIT_INTS"} = 1;
    }
    foreach my $arg (split /\s+/, $flags) {
      if ($arg =~ /^\-D(\w+)(?:=(.+))?/) {
        $macros{$1} = $2 || 1;
      }
      elsif ($arg =~ /^\-I(.+)/) {
        push(@include, $1);
      }
      elsif ($arg =~ /^--(no-)?default-nested$/) {
        $self->{'default_nested'} = !$1;
      }
    }
  }

  return $self->cached_parse($file, \@include, \%macros, \%mparams);
}


sub get_typesupport_info {
  my($self, $tsfile) = @_;
  return undef unless exists $self->{'strs'}->{$tsfile};
  return $self->{'strs'}->{$tsfile}->[1];
}

sub get_type_bits {
  my($self, $type) = @_;
  return $types{$type};
}


# ************************************************************
# Public Interface Section
# ************************************************************

sub get_output {
  my($self, $file, $flags) = @_;

  my @filenames;
  my %seen;

  ## Parse the IDL file and get back the types and names.
  my($data, $fwd) = $self->do_cached_parse($file, $flags);

  ## Get the file names based on the type and name of each entry
  my @tmp;
  foreach my $ent (@$data) {
    my($type, $scope) = @$ent;
    push @tmp, @{$self->get_filenames($flags, $type, $scope)};
  }
  @filenames = grep(!$seen{$_}++, @tmp); # remove duplicates

  ## Return the file name list and a map of dependencies.
  return \@filenames, $self->get_dependencies(\@filenames, $fwd)
}

sub get_filenames {
  ## This method is called with the base directory and filename and
  ## expects an array of filenames to be tied to the source file.
  return [];
}

sub get_tied {
  my($self, $file, $files) = @_;
  my $tied = [];

  my $ts = $tsreg;
  $ts =~ s/\\//g;
  $file =~ s/\.idl$//;

  foreach my $f (@$files) {
    if ($f eq "$file$ts") {
      push(@$tied, $f);
      last;
    }
  }

  return $tied, $self->get_component_name();
}

sub get_component_name {
  ## This method is called with no arguments and expects a string
  ## containing the component name to tie generated files together.
  return "";
}

sub get_dependencies {
  my($self, $filenames, $fwdarray) = @_;

  ## There aren't any additional dependencies if there were no forward
  ## declarations.
  return undef if (scalar(@$fwdarray) == 0);

  my %dependencies;
  ## MPC v4.1.41 and older does not provide the ProjectCreator to the
  ## CommandHelper.  If we don't have the creator, the best we can
  ## do is say that the .java files depends on other .java files.
  if (exists $self->{'creator'}) {
    ## If we have been given a ProjectCreator, we can use the .java files
    ## in the forward array to create .class file names which is what the
    ## .java files are truly dependent upon.
    my @genfiles;
    foreach my $file (@$fwdarray) {
      my $of = $self->{'creator'}->get_first_custom_output($file, 'java_files');
      push(@genfiles, $of) if (defined $of && $of ne '');
    }

    ## Now that we have the .class files that each of the files listed in
    ## @$filenames will be dependent upon, we must go through each file
    ## and add it to the dependency map.  The files upon which each file
    ## will be dependent must be custom tailored to remove the .class
    ## that the file itself will create to avoid circular dependencies.
    foreach my $file (@$filenames) {
      my $of = $self->{'creator'}->get_first_custom_output($file, 'java_files');
      if (defined $of && $of ne '') {
        my $re = $self->{'creator'}->escape_regex_special($of);
        my @arr = grep(!/^$re$/, @genfiles);
        $dependencies{$file} = \@arr;
      }
      else {
        $dependencies{$file} = \@genfiles;
      }
    }
  }
  else {
    ## This dependency map says that every generated file will depend on
    ## other generated files based solely on forward declared elements.
    %dependencies = map { $_ => $fwdarray } @$filenames;
  }
  return {$self->get_component_name() => \%dependencies};
}

# ************************************************************
# File Processing Subroutine Section
# ************************************************************

sub get_scope {
  my($self, $state) = @_;
  my @scope = ();

  foreach my $entry (@$state) {
    push @scope, $$entry[1];
  }

  return \@scope;
}

sub locate_file {
  my($self, $file) = @_;

  ## Look through the idl2jni files to see if we can find our files actual
  ## location according to the MPC file.  It is possible that the
  ## TypeSupport.idl will not be generated in the same directory as the source
  ## idl file.
  my $base = $self->{'creator'}->mpc_basename($file);
  my @comps = $self->{'creator'}->get_component_list('idl2jni_files');
  foreach my $comp (@comps) {
    if ($self->{'creator'}->mpc_basename($comp) eq $base) {
      ## There should only be one that matches.  Some build tools, such as
      ## Visual Studio, ignore duplicate file names (even if they are in
      ## different directories).
      return $comp;
    }
  }

  ## Give back what we were given.  That's all we can do here.
  return $file;
}

sub cached_parse {
  my($self, $file, $includes, $macros, $mparams) = @_;

  ## Convert all $(...) to the value of the current environment variable.
  ## It's not 100%, but it's the best we can do.
  while($file =~ /\$\(([^\)]+)\)/) {
    my $val = $ENV{$1} || '';
    $file =~ s/\$\([^\)]+\)/$val/;
  }

  ## If we have already processed this file, we will just delete the
  ## stored data and return it.
  return delete $self->{'files'}->{$file}, delete $self->{'forwards'}->{$file}
           if (defined $self->{'files'}->{$file});

  ## If the file is a DDS type support idl file, we will remove the
  ## TypeSupport portion and process the file from which it was created.
  ## In the process, we will store up the "contents" of the type support
  ## idl file for use below.
  ##
  ## If the type support file had previously been preprocessed, we will
  ## just parse the preprocessed string and continue on as usual.
  my $actual = $file;
  my $ts = defined $self->{'strs'}->{$actual} ||
           ($actual =~ /$tsreg$/ && -r $actual) ?
                   undef : ($actual =~ s/$tsreg$/.idl/);

  ## MPC v4.1.41 and older does not provide the ProjectCreator to the
  ## CommandHelper.  If we have the ProjectCreator and the original file was
  ## a TypeSupport.idl, but does not yet exist.  We will attempt to find the
  ## location of the idl file from which the TypeSupport.idl will be generated.
  if (exists $self->{'creator'} && $file ne $actual) {
    $actual = $self->locate_file($actual);
  }

  my($data, $forwards, $ts_str, $ts_pragma) =
       $self->parse($actual, $includes, $macros, $mparams);

  if ($ts) {
    ## The file passed into this method was the type support file.  Store
    ## the data processed from the non-type support file and parse the
    ## string that was obtained during the original parsing and return
    ## that data.
    $self->{'files'}->{$actual} = $data;
    $self->{'forwards'}->{$actual} = $forwards;
    ($data) = $self->parse($file, $includes, $macros, $mparams, $ts_str);
  }
  elsif ($ts_str) {
    ## The file passed in was not a type support, but contained #pragma's
    ## that indicate a type support file will be generated, we will store
    ## that text for later use (in preprocess).
    my $key = $file;
    $key =~ s/\.idl$/$tsreg/;
    $key =~ s/\\//g;
    $self->{'strs'}->{$key} = [$ts_str, $ts_pragma];
  }

  return $data, $forwards;
}

sub parse {
  my($self, $file, $includes, $macros, $mparams, $str) = @_;
  my @forwards;

  ## Preprocess the file into one huge string
  my $ts_str;
  my $ts_pragma;
  my $included;
  ($str, $ts_str, $ts_pragma) =
     $self->preprocess($file, $includes, $macros, $mparams) if (!defined $str);

  ## Track the nested value over the lifetime of this file.  The $cnested
  ## variable is used to track the current value set by the user, which may
  ## or may not get pushed onto the nested stack.  The top of the stack is
  ## the actual setting for the current scope.
  my @nested = ($self->{'default_nested'});
  my $cnested = $nested[$#nested];

  ## Keep track of const's and typedef's with these variables
  my $single;
  my $stype;
  my $simple;
  my $seq = 0;
  my $typedef_requires_holder;

  ## Keep track of whether or not an interface is local
  my $local;

  ## Keep track of forward declartions.
  my $forward;

  ## Tokenize the string and save the data
  my @data;
  my @state;
  while(length($str) != 0) {
    ## Remove starting white-space
    $str =~ s/^\s+//;

    ## Now check the start of the string for a particular type
    if ($str =~ s/^(("([^\\"]*|\\[abfnrtvx\\\?'"]|\\[0-7]{1,3})*"\s*)+)//) {
      ## String literal
    }
    elsif ($str =~ s/^((L"([^\\"]*|\\[abfnrtvx\\\?'"]|\\[0-7]{1,3}|\\u[0-9a-fA-F]{1,4})*"\s*)+)//) {
      ## Wstring literal
    }
    elsif ($str =~ s/^L'(.|\\.|\\[0-7]{1,3}|\\x[a-f\d]{1,2}|\\u[a-f\d]{1,4})'//i) {
      ## Wchar literal
    }
    elsif ($str =~ s/^@([A-Za-z0-9:_]+)(?:\s*\(([^\)]*)\))?//) {
      ## Annotation
      my $name = $1;
      if ($name eq 'default_nested' || $name eq 'nested') {
        $cnested = (defined $2 && $2 eq 'FALSE') ? 0 : 1;
      } elsif ($name eq 'topic') {
        # TODO: Take platform member into consideration
        $cnested = 0;
      }
    }
    elsif ($str =~ s/^([a-z_][\w]*)//i) {
      my $name    = $1;
      my $keyword = $idl_keywords{$name};
      if ($keyword) {
        if ($keyword == 2) {
          ## It's a keyword that requires an opening '{'
          push(@state, [$name]);
          $forward = 1;
        }
        elsif ($keyword == 3) {
          ## This is either a const, a typedef, or a native.  If it's a
          ## native, then we do not need to wait for an additional type
          ## ($stype).
          $single = $name;
          $stype = 1 if ($name ne 'native');
          $simple = 1;
          $typedef_requires_holder = undef;
        }
        elsif ($keyword == 4) {
          ## The interface will be local
          $local = 1;
        }
        else {
          ## This is not a keyword that we care about.  We need to
          ## reset this flag so that we know that in a typedef, we have
          ## found the original type part.
          $stype = undef;
          if (!defined $typedef_requires_holder && $name eq 'sequence') {
            $typedef_requires_holder = 1;
          }
        }
      }
      else {
        ## We're not going to do any checks on the word here.  If it is
        ## invalid (i.e., starts with more than one underscore, differs
        ## only in case from keyword, etc.) we'll let the real tool catch
        ## it.
        if (defined $single) {
          ## If we are not inside the type part of the sequence
          if ($seq == 0) {
            ## If we are waiting for the original type in the typedef,
            ## then we need to skip this word.
            if ($stype) {
              ## However, we only want to reset $stype if this is the end
              ## of the type.  If there is a fully qualified scoped name,
              ## it will be separated into parts at the double colon.
              $stype = undef if ($str =~ /^\s+/);
            }
            else {
              ## Otherwise, we will save the const or typedef in the data
              ## section.

              ## If this is a simple typedef, we need to prefix it with
              ## the word 'simple' so that we know which files will be
              ## generated from it.
              $single = 'simple ' . $single
                     if ($simple && $single eq 'typedef' && $str !~ /^\s*\[/);

              ## Get the scope and put the entry in the data array
              my $scope = $self->get_scope(\@state);
              push @$scope, $name;
              push(@data, [$single, $scope]);

              ## Reset this so that we don't continue adding entries
              $single = undef;
            }
          }
        }
        elsif ($#state >= 0 && !defined $state[$#state]->[1]) {
          $state[$#state]->[1] = $name;
        }
      }
    }
    elsif ($str =~ s/^([\-+]?(\d+(\.(\d+)?)?|\.\d+)d)//i) {
      ## Fixed literal
    }
    elsif ($str =~ s/^(-?(((\d+\.\d*)|(\.\d+))(e[+-]?\d+)?[lf]?|\d+e[+-]?\d+[lf]?))//i) {
      ## Floating point literal
    }
    elsif ($str =~ s/^(\-(0x[a-f0-9]+|0[0-7]*|\d+))//i) {
      ## Integer literal
    }
    elsif ($str =~ s/^((0x[a-f0-9]+|0[0-7]*|\d+))//i) {
      ## Unsigned integer literal
    }
    elsif ($str =~ s/^'(.|\\.|\\[0-7]{1,3}|\\x[a-f\d]{1,2})'//) {
      ## Character literal
    }
    elsif ($str =~ s/^(<<|>>|::|=)//) {
      ## Special symbols
    }
    elsif (length($str) != 0) {
      ## Generic character
      my $c = substr($str, 0, 1);
      substr($str, 0, 1) = '';

      ## We have not determined if this is a forward declaration yet.  If
      ## we see a semi-colon before an opening curly brace, then it's a
      ## forward declaration and we need to drop it.
      if ($forward) {
        if ($c eq '{') {
          push(@nested, $cnested);

          ## If this was previously forward declared, we can remove it
          ## now that it has been fully declared as we no longer need to
          ## create a dependency on it.
          my $scope = $self->get_scope(\@state);
          my $file = join('/', @$scope) . $self->get_file_ext();
          if (grep(/^$file$/, @forwards)) {
            @forwards = grep(!/^$file$/, @forwards);
          }
          $forward = undef;

          ## If it is not nested, it is a topic type.
          if (!$nested[$#nested]) {
            ## If this is a struct or union, we need to append to the type
            ## support string and "pragma"
            if ($#state >= 0 &&
                ($state[$#state]->[0] eq 'struct' ||
                 $state[$#state]->[0] eq 'union')) {
              my($tst, $tsp) = $self->generate_ts_string(join('::', @$scope));
              $ts_str .= $tst;
              $ts_pragma .= $tsp;
            }
          }
        }
        elsif ($c eq ';') {
          ## This is a forward declaration.  Add it to the list of
          ## forward declarations to return back with the rest of the data.
          my $scope = $self->get_scope(\@state);
          push(@forwards, join('/', @$scope) . $self->get_file_ext());

          pop(@state);
          $forward = undef;
        }
      }

      ## We've found a closing brace
      if ($c eq '}') {
        pop(@nested);
        $cnested = $nested[$#nested];

        ## See if the start of the scope is something that we support
        my $entry = pop(@state);
        if (defined $$entry[0] && $types{$$entry[0]}) {
          ## If the local flag is set, then this must be a local interface
          if ($local) {
            $$entry[0] = 'local ' . $$entry[0];
            $local = undef;
          }

          ## Save the scope in the entry array
          my $scope = $self->get_scope(\@state);
          push @$scope, $$entry[1];
          splice(@$entry, 1, 0, $scope);

          ## Save the entry in the data array
          push(@data, $entry);
        }
      }
      elsif ($c eq '<') {
        ## Keep track of the sequence type opening
        $seq++;

        ## A sequence typedef is not simple
        $simple = undef if $typedef_requires_holder;
      }
      elsif ($c eq '>') {
        ## Keep track of the sequence type closing
        $seq--;
      }
    }
  }

  return \@data, \@forwards, $ts_str, $ts_pragma;
}

# ************************************************************
# Preprocessor Subroutine Section
# ************************************************************

sub generate_ts_string {
  my($self, $dtype) = @_;

  my $ts_str = '';
  my $ts_pragma = "$dtype;";

  ## Get the data type and remove the scope portion
  my @ns;
  if ($dtype =~ s/(.*):://) {
    @ns = split(/::/, $1);
  }

  ## For now, we will assume that all parts of the scope
  ## name are modules.  If idl2jni is extended to support
  ## types declared within interfaces, this code will need
  ## to change.
  foreach my $ns (@ns) {
    $ts_str .= "module $ns { ";
  }

  $ts_str .= "native ${dtype}Seq; " .
             "local interface ${dtype}TypeSupport {}; " .
             "local interface ${dtype}DataWriter {}; " .
             "local interface ${dtype}DataReader {}; ";

  ## Close the namespaces (module or interface it works the
  ## same).
  foreach my $ns (@ns) {
    $ts_str .= " };";
  }

  return $ts_str, $ts_pragma;
}

sub preprocess {
  my($self, $file, $includes, $macros, $mparams, $included) = @_;
  my $fh = new FileHandle();
  my $contents = '';
  my $skip = [];
  my $ts_str = '';
  my $ts_pragma = '';

  if (open($fh, $file)) {
    my $line;
    my $saved = '';
    my $in_comment;
    while(<$fh>) {
      ## Get the starting and ending position of a string
      my $qs = index($_, '"');
      my $qe = rindex($_, '"', length($_) - 1);

      ## Look for the starting point of a C++ comment
      my $cs = index($_, '//');
      if ($cs < $qs || $cs > $qe) {
        ## If it's not inside of a string, remove it
        $_ =~ s/\/\/.*//;
      }

      ## Look for the starting point of a C-style comment
      $cs = index($_, '/*');
      if ($cs < $qs || $cs > $qe) {
        ## Remove the one line c comment if it's not inside of a string
        $_ =~ s/\/\*.*\*\///;
      }

      ## Check for multi-lined c comments
      if (($cs < $qs || $cs > $qe) && $_ =~ s/\/\*.*//) {
        $in_comment = 1;
      }
      elsif ($in_comment) {
        if ($_ =~ s/.*\*\///) {
          ## We've found the end of the C-style comment
          $in_comment = undef;
        }
        else {
          ## We're still in the C-style comment, so just empty it out.
          $_ = '';
        }
      }

      if (/(.*)\\\s*$/) {
        ## If this is a concatenation line, save it for later
        $saved .= $1;
        $saved =~ s/\s+$/ /;
      }
      else {
        $line = $saved . $_;
        $saved = '';

        ## Remove trailing white space
        $line =~ s/\s+$//;

        ## Check for a preprocessor directive.  We support if/ifdef/ifendif
        ## and various others.
        if ($line =~ s/^\s*#\s*//) {
          my $pline = $line;
          $line = '';
          if ($pline =~ /^if\s+(.*)/) {
            ## If we're currently skipping text due to some other #if,
            ## add another skip so that when we find the matching #endif,
            ## we don't stop skipping text.
            if ($$skip[scalar(@$skip) - 1]) {
              push(@$skip, 1);
            }
            else {
              ## Send the #if off to be evaluated
              push(@$skip, !$self->evaluate_if($macros, $1));
            }
          }
          elsif ($pline =~ /^ifdef\s+(.*)/) {
            ## If we're currently skipping text due to some other #if,
            ## add another skip so that when we find the matching #endif,
            ## we don't stop skipping text.
            my $expr = $1;
            if ($$skip[scalar(@$skip) - 1]) {
              push(@$skip, 1);
            }
            else {
              ## Check for the macro definition.  If it's defined, we're
              ## not going to skip the next set of text.
              if (defined $macros->{$expr}) {
                push(@$skip, 0);
              }
              else {
                push(@$skip, 1);
              }
            }
          }
          elsif ($pline =~ /^ifndef\s+(.*)/) {
            ## If we're currently skipping text due to some other #if,
            ## add another skip so that when we find the matching #endif,
            ## we don't stop skipping text.
            my $expr = $1;
            if ($$skip[scalar(@$skip) - 1]) {
              push(@$skip, 1);
            }
            else {
              ## Check for the macro definition.  If it's defined, we're
              ## are going to skip the next set of text.
              if (defined $macros->{$expr}) {
                push(@$skip, 1);
              }
              else {
                push(@$skip, 0);
              }
            }
          }
          elsif ($pline =~ /^else$/) {
            ## Make sure we have a corresponding #if
            if (defined $$skip[0]) {
              ## We know that there is at least one element in the $skip
              ## array.  But, if there is more than one element, we have to
              ## check the #if in front of this one in the stack to see if
              ## we can stop skipping text.  If the #if in front of this
              ## one (or even farther in front) is causing skipping, we
              ## need to continue skipping even after processing this #else.
              if (scalar(@$skip) == 1 || !$$skip[scalar(@$skip) - 2]) {
                $$skip[scalar(@$skip) - 1] ^= 1;
              }
            }
            else {
              ## #else without a #if
              last;
            }
          }
          elsif ($pline =~ /^endif$/) {
            if (defined $$skip[0]) {
              pop(@$skip);
            }
            else {
              ## #endif without a #if
              last;
            }
          }
          elsif (!$$skip[scalar(@$skip) - 1]) {
            ## If we're not skipping text, see if the preprocessor
            ## directive was an include.
            if ($pline =~ /^include\s+(.*)$/) {
              my $expr = $1;
              if (defined $macros->{$expr}) {
                $expr = $macros->{$expr};
              }
              if ($expr =~ /^(["<])(.*)([>"])$/) {
                my $s     = $1;
                my $file  = $2;
                my $e     = $3;

                ## Make sure that we have matching include file delimiters
                if (!(($s eq '<' && $e eq '>') || $s eq $e)) {
                  ## Unmatched character
                }
                else {
                  $self->include_file($file, $includes, $macros, $mparams);
                }
              }
            }
            elsif ($pline =~ /^define\s+(([a-z_]\w+)(\(([^\)]+)\))?)(\s+(.*))?$/i) {
              my $name   = $2;
              my $params = $4;
              my $value  = $6 || 1;

              ## Define the macro and save the parameters (if there were
              ## any).  We will use it later on in the replace_macros()
              ## method.
              $macros->{$name} = $value;
              if (defined $params) {
                my @params = split(/\s*,\s*/, $params);
                $mparams->{$name} = \@params;
              }
            }
            elsif ($pline =~ /^pragma\s+(.*)/) {
              my $arg = $1;
              if ($arg =~ /^DCPS_DATA_TYPE\s+"(.*)"$/) {
                my($tst, $tsp) = $self->generate_ts_string($1);
                $ts_str .= $tst;
                $ts_pragma .= $tsp;
              }
            }
          }
        }
        elsif ($line =~ s/^import\s+([^;]+)\s*;//) {
          ## The import keyword is similar to #include, so we're handling
          ## it here.  This is probably not fool proof, but it will
          ## probably handle most situations where it's used.
          my $file = $1;
          $file =~ s/\s+$//;
          $file .= '.idl';

          $self->include_file($file, $includes, $macros, $mparams);
        }

        if (!$$skip[scalar(@$skip) - 1] && !$included) {
          $contents .= ' ' . $self->replace_macros($macros, $mparams, $line);
        }
      }
    }
    close($fh);
  }
  elsif (defined $self->{'strs'}->{$file} && !$included) {
    $contents = $self->{'strs'}->{$file}->[0];
    delete $self->{'strs'}->{$file};
  }

  return $contents, $ts_str, $ts_pragma;
}

sub include_file {
  my($self, $file, $includes, $macros, $mparams) = @_;

  ## Look for the include file in the user provided include paths
  foreach my $incpath ('.', @$includes) {
    if (-r "$incpath/$file") {
      return $self->preprocess(($incpath eq '.' ? '' : "$incpath/") . $file,
                               $includes, $macros, $mparams, 1);
    }
  }

  return '';
}

sub evaluate_if {
  my($self, $macros, $value) = @_;
  my $status = 1;

  ## Remove leading and trailing spaces
  $value =~ s/^\s+//;
  $value =~ s/\s+$//;

  ## Split up parenthesis
  if (index($value, '(') == 0) {
    my $count  = 0;
    my $length = length($value);
    for(my $i = 0; $i < $length; $i++) {
      my $c = substr($value, $i, 1);
      if ($c eq '(') {
        $count++;
      }
      elsif ($c eq ')') {
        $count--;
        if ($count == 0) {
          my $val = substr($value, 1, $i - 1);
          my $ret = $self->evaluate_if($macros, $val);
          substr($value, 0, $i + 1) = $ret;
          last;
        }
      }
    }
  }

  ## Handle OR and AND by recursively calling this method using the
  ## built-in process of these operators.
  if ($value =~ /(\|\||&&)/) {
    my $op   = $1;
    my $loc  = index($value, $op);
    my $part = substr($value, 0, $loc);
    my $rest = substr($value, $loc + 2);
    if ($op eq '||') {
      $status = $self->evaluate_if($macros, $part) ||
                $self->evaluate_if($macros, $rest);
    }
    else {
      $status = $self->evaluate_if($macros, $part) &&
                $self->evaluate_if($macros, $rest);
    }
  }
  else {
    ## For #if, we only support defined, macro and numeric values.
    ## All others are considered a syntax error.
    if ($value =~ /^(!)?\s*defined\s*\(?\s*([_a-z]\w*)\s*\)?$/i) {
      my $not   = $1;
      my $macro = $2;
      $status = (defined $macros->{$macro} ? 1 : 0);
      $status = !$status if ($not);
    }
    elsif ($value =~ /^([_a-z]\w*)\s*([=!]=)\s*(\d+)$/i) {
      my $macro = $1;
      my $not   = ($2 eq '!=');
      my $val   = $3;
      $status = (defined $macros->{$macro} &&
                 $macros->{$macro} eq $val ? 1 : 0);
      $status = !$status if ($not);
    }
    elsif ($value =~ /^([_a-z]\w*)$/i) {
      my $macro = $1;
      $status = (defined $macros->{$macro} &&
                 $self->evaluate_if($macros, $macros->{$macro}));
    }
    elsif ($value =~ /^\d+$/) {
      $status = ($value ? 1 : 0);
    }
    else {
      ## Syntax error in #if
      $status = 0;
    }
  }
  return $status;
}

sub replace_macros {
  my($self, $macros, $mparams, $line) = @_;
  foreach my $macro (keys %$macros) {
    ## For each macro provided by the user, see if it has been used
    ## anywhere in this line.
    while ($line =~ /\b$macro\b/) {
      ## Replace the corresponding parameter names with the correct
      ## values obtained above.
      my(@strings, @dstrings);
      my $escaped = ($line =~ s/\\\"/\01/g);
      $escaped |= ($line =~ s/\\\'/\02/g);
      while($line =~ s/('[^']+')/\04/) {
        push(@strings, $1);
      }
      while($line =~ s/("[^"]+")/\03/) {
        push(@dstrings, $1);
      }

      ## It has been used, so save the value for later
      my $val = $macros->{$macro};

      if (defined $mparams->{$macro}) {
        ## See the user provided macro takes parameters
        if ($line =~ /\b($macro\s*\()/) {
          ## Gather up the macro parameters
          my $start  = $1;
          my $length = length($line);
          my $count  = 1;
          my $uses   = index($line, $start);
          my $usee   = $length;
          my $p      = $uses + length($start);
          my @params;
          for(my $i = $p; $i < $length; $i++) {
            my $c = substr($line, $i, 1);
            if ($c eq '(') {
              $count++;
            }
            elsif ($c eq ',' || $c eq ')') {
              if ($c eq ')') {
                $count--;
                if ($count == 0) {
                  $usee = $i + 1;
                }
                else {
                  ## This isn't the end of the parameters, so keep going
                  next;
                }
              }
              elsif ($count > 1) {
                ## This is a not a parameter marker since we are inside a
                ## set of parenthesis.
                next;
              }

              ## We've reached the last parenthesis, so add this to the
              ## list of parameters after stripping off leading and
              ## trailing white space.
              my $param = substr($line, $p, ($i - $p));
              $param =~ s/^\s+//;
              $param =~ s/\s+$//;
              push(@params, $param);

              ## Set the starting point for the next parameter to the
              ## character just after the current closing parenthesis.
              $p = $i + 1;
            }
          }

          my $i = 0;
          foreach my $param (@{$mparams->{$macro}}) {
            my $pval = $params[$i];
            $val =~ s/\b$param##/$pval/g;
            $val =~ s/##$param\b/$pval/g;
            $val =~ s/\b$param\b/$pval/g;
            $i++;
          }

          ## Replace the macro call with the expanded macro value
          substr($line, $uses, $usee - $uses) = $val;
        }
      }
      else {
        ## There were no macro parameters, so just do a simple search and
        ## replace.
        $line =~ s/\b$macro\b/$val/g;
      }

      ## We will need to leave the loop if we do not see any instances of
      ## the current macro.  We save the indicator so that we can replace
      ## strings before leaving.
      my $leave = ($line !~ /\b$macro\b/);

      ## Replace the escaped characters with the right values.
      foreach my $dstring (@dstrings) {
        $line =~ s/\03/$dstring/;
      }
      foreach my $string (@strings) {
        $line =~ s/\04/$string/;
      }
      if ($escaped) {
        $line =~ s/\01/\\"/g;
        $line =~ s/\02/\\'/g;
      }

      last if ($leave);
    }
  }
  return $line;
}

1;
