eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use strict;

use FindBin ;
use lib "$FindBin::Bin" ;
use lib "$ENV{ACE_ROOT}/bin";
use File::Basename;
use PerlACE::Run_Test;

use Getopt::Long qw( :config bundling) ;
use Pod::Usage ;

use DCPS::IDLTemplate ;
use DCPS::CPPTemplate ;
use DCPS::HTemplate ;

#
# Basic options.
#
my $debug ;
my $man ;
my $help ;
my $verbose ;

#
# Specific options.
#
my $backup = 0 ;
my $typestyle = "struct" ;
my $subdir ;
my $exportmacro ;
my $pchfile ;
my $file ;
my $cpp_only = 0;
my @extensions;
my $ext_only;
my $ext_args;

########################################################################
#
# Process the command line.
#
GetOptions( "verbose!"    => \$verbose,
            "v"           => \$verbose,
            "help|?"      => \$help,
            "man"         => \$man,
            "debug|d"     => \$debug,
            "dir|S=s"     => \$subdir,
            "export|X=s"  => \$exportmacro,
            "pch=s"       => \$pchfile,
            "idl=s"       => \$file,
            "timestamp|t" => \$backup,
            "backup!"     => \$backup,
            "cpp_only=i"  => \$cpp_only,
            "extension=s" => \@extensions,
            "extension_args=s" => \$ext_args,
            "extensions_only" => \$ext_only,

) or pod2usage( 0) ;
pod2usage( 1)             if $help ;
pod2usage( -verbose => 2) if $man ;

#
# Grab the reset of the command line.
#
my $idlfile  = shift || $file ;

#
# We MUST have enough information to process.
#
pod2usage( 1) if not $idlfile ;

#
# Parse the idl file for the pragmas that mark the DCPS types
#
my @types = @ARGV;
if( open(IDLINFILE, $idlfile)) {
  while (<IDLINFILE>) {
    if (/^ *# *pragma *DCPS_DATA_TYPE *"(.*)"/) {
      push (@types, $1);
    }
  }
  close IDLINFILE ;
} else {
  &error( "Failed to open IDL file for input: $idlfile\n") ;
  exit 1 ;
}
if (not scalar @types) {
  &error( "No DCPS types found in $idlfile\n") ;
  exit 1 ;
}



#
########################################################################

#
# Generate output filenames.
#
my $idlbase = basename($idlfile);
$idlbase =~ s/\.idl//;

my $idloutputfile = $idlbase . "TypeSupport.idl" ;
my $houtputfile   = $idlbase . "TypeSupportImpl.h" ;
my $cppoutputfile = $idlbase . "TypeSupportImpl.cpp" ;


my %extra_text;

#
# Invoke extensions
#
for my $ext (@extensions) {
  my @path = split /\/|\\/, $ext;
  my $file = pop @path;
  my $inc = join ('/', @path);
  $inc = '.' if $inc eq '';
  my @INCsave = @INC;
  unshift @INC, $inc;
  require "$file.pm";
  @INC = @INCsave;
  my $results = $file->generate (\@types, $ext_args,
                                 [$idloutputfile, $houtputfile, $cppoutputfile]
                                 );
  for my $filetype ('idl', 'h', 'cpp') {
    $extra_text{$filetype} .= $$results{$filetype};
  }
}


#
# Rename output files if we need to backup.
#
my @now = localtime ;
my $timestamp = "" ;
if( $backup) {
  $timestamp
    = sprintf "_%04d%02d%02d%02d%02d%02d",
        (1900+$now[5]), (1+$now[4]), $now[3],
        $now[2], $now[1], $now[0] ;

  # What _I_ would use:
  # $timestamp = "_" . time ;

  my $idlbackupfile = $idloutputfile . $timestamp ;
  my $hbackupfile   = $houtputfile   . $timestamp ;
  my $cppbackupfile = $cppoutputfile . $timestamp ;

  if ($cpp_only == 0) {
    rename $idloutputfile, $idlbackupfile
      or die "Failed to backup $idloutputfile to $idlbackupfile\n"
      if -r $idloutputfile ;
    rename $houtputfile, $hbackupfile
      or die "Failed to backup $houtputfile to $hbackupfile\n"
      if -r $houtputfile ;
  }
  rename $cppoutputfile, $cppbackupfile
    or die "Failed to backup $cppoutputfile to $cppbackupfile\n"
    if -r $cppoutputfile ;
}

#
########################################################################

########################################################################
#
# Set up the single file contents and go through each type adding to the
# contents along the way.
#

my $idl_content = DCPS::IDLTemplate::header();
$idl_content =~ s/<%SUBDIR%>/$subdir/g;
$idl_content =~ s/<%IDLFILE%>/$idlfile/g;

my $h_content = DCPS::HTemplate::header();
$h_content =~ s/<%IDLBASE%>/$idlbase/g;
$h_content =~ s/<%UPPERIDLBASE%>/uc($idlbase)/ge;

my $cpp_content = DCPS::CPPTemplate::header();
my $pchinclude = "";
$pchinclude = "#include \"${pchfile}\"" if $pchfile ;
$cpp_content =~ s/<%PCHINCLUDE%>/$pchinclude/g ;
$cpp_content =~ s/<%IDLBASE%>/$idlbase/g;

my $count = 0;
foreach my $type (@types) {
  #
  # Break out any scope from the type.
  #
  my $scopepath ;
  my $basetype ;
  if($type=~/^(.*)::([^:]*)$/) {
    $scopepath = $1 ;
    $basetype  = $2 ;
  } else {
    $basetype = $type ;
  }
  $scopepath .= "::" if $scopepath ;
  &console( "Scope path set to: $scopepath") if $scopepath ;
  &console( "Base type set to: $basetype") ;

  #
  # Determine any subdirectory substitutions.
  #
  if( $subdir) {
    $subdir .= "/" ;
    &console( "Subdirectory set to: $subdir") ;
  } else {
    $subdir = "" ;
    &console( "Subdirectory not used.") ;
  }

  #
  # Module substitutions are bracketed.
  #
  my $modulestart    = "" ;
  my $moduleend      = "" ;
  my $namespacestart = "" ;
  my $namespaceend   = "" ;
  my $module         = "" ;

  # default to the same modulepath as the scope path (new for v0.12 5/9/07)
  &console( "Defaulting Module scope to the path scope $scopepath minus ::.") ;
  my $modulepath = $scopepath;
  $modulepath =~ s/\:\:$//;

  if( $modulepath) {
    $module         = "$modulepath\:\:" ;
    &console( "Module set to: $module") ;

    #
    # Each element of the module path is its own scope.
    #
    foreach my $entry (split( '::', $modulepath)) {
      $modulestart    .= "module $entry {\n" ;
      $moduleend      .= "};\n" ;
      $namespacestart .= "namespace $entry {\n" ;
      $namespaceend   .= "}; // $entry\n" ;
    }
    &console( "Module start substitution set to: $modulestart") ;
    &console( "Namespace start substitution set to: $namespacestart") ;

  } else {
    &console( "No enclosing Module scope.") ;
  }

  #
  # Uppercase type name for macros.
  #
  my $uppertype = $basetype ;
  $uppertype =~ y/a-z/A-Z/ ;

  #
  # Export directive.
  #
  my $export = "" ;
  $export = "${exportmacro}" if $exportmacro ;

  #
  # POA attachment.
  #
  my $poa = "POA_${module}" ;

  if ($cpp_only == 0 && !$ext_only) {
    ########################################################################
    #
    # Generate the IDL output file.
    #

    #
    # Slurp the entire template.
    #
    my $idl_template = DCPS::IDLTemplate::contents() ;

    #
    # Convert template tags into the specified values.
    #
    $idl_template =~ s/<%TYPE%>/$basetype/g ;
    $idl_template =~ s/<%SCOPE%>/$scopepath/g ;
    $idl_template =~ s/<%SUBDIR%>/$subdir/g ;
    $idl_template =~ s/<%IDLFILE%>/$idlfile/g ;
    $idl_template =~ s/<%MODULESTART%>/$modulestart/g ;
    $idl_template =~ s/<%MODULEEND%>/$moduleend/g ;

    $idl_content .= $idl_template;
    #
    ########################################################################

    ########################################################################
    #
    # Generate the C++ header output file.
    #

    #
    # Slurp the entire template.
    #
    my $h_template = DCPS::HTemplate::contents() ;

    #
    # Convert template tags into the specified values.
    #
    $h_template =~ s/<%TYPE%>/$basetype/g ;
    $h_template =~ s/<%SCOPE%>/$scopepath/g ;
    $h_template =~ s/<%EXPORT%>/$export/g ;
    $h_template =~ s/<%POA%>/$poa/g ;
    $h_template =~ s/<%MODULE%>/$module/g ;
    $h_template =~ s/<%NAMESPACESTART%>/$namespacestart/g ;
    $h_template =~ s/<%NAMESPACEEND%>/$namespaceend/g ;

    $h_content .= $h_template;
    #
    ########################################################################

  } # endif $cpp_only != 0

  if (!$ext_only) {
    ########################################################################
    #
    # Generate the C++ implementation output file.
    #

    #
    # Slurp the entire template.
    #
    my $cpp_template = DCPS::CPPTemplate::contents() ;

    #
    # Convert template tags into the specified values.
    #
    $cpp_template =~ s/<%TYPE%>/$basetype/g ;
    $cpp_template =~ s/<%SCOPE%>/$scopepath/g ;
    $cpp_template =~ s/<%MODULE%>/$module/g ;
    $cpp_template =~ s/<%NAMESPACESTART%>/$namespacestart/g ;
    $cpp_template =~ s/<%NAMESPACEEND%>/$namespaceend/g ;
    $cpp_template =~ s/<%COUNT%>/$count/g;

    $cpp_content .= $cpp_template;

  }

  $count++;
}

$idl_content .= $extra_text{'idl'};
$h_content   .= $extra_text{'h'};
$cpp_content .= $extra_text{'cpp'};

my $h_footer = DCPS::HTemplate::footer();
$h_footer =~ s/<%IDLBASE%>/$idlbase/g ;
$h_content .= $h_footer;

#
########################################################################

#
# Put the results in the file.
#
if ($ext_only) {
  exit 0;
}

if (open(IDLFILE, ">$idloutputfile")) {
  print IDLFILE $idl_content;
  close(IDLFILE);
  console("IDL file $idloutputfile written.");

}
else {
  error("Failed to open IDL file for output: $idloutputfile\n");
  exit(1);
}

if ($cpp_only == 0) {
  #
  # Put the results in the file.
  #
  if (open(HFILE, ">$houtputfile")) {
    print HFILE $h_content;
    close(HFILE);
    console("Header file $houtputfile written.") ;

  }
  else {
    error("Failed to open header file for output: $houtputfile\n");
    exit(1);
  }
}

#
# Put the results in the file.
#
if (open(CPPFILE, ">$cppoutputfile")) {
  print CPPFILE $cpp_content;
  close(CPPFILE);
  console("CPP file $cppoutputfile written.");
}
else {
  error("Failed to open CPP file for output: $cppoutputfile\n");
  exit(1);
}

#
# Done
#
exit 0 ;

########################################################################
#
# Subroutine to send console messages with consistent information.
#
sub console {
  print "\n*** dcps_ts.pl: @_\n" if $verbose ;
}
#
########################################################################

########################################################################
#
# Subroutine to send error messages with consistent information.
#
sub error {
  print STDERR "\n*** dcps_ts.pl: ERROR - " . join( ' ', @_)  . "\n" ;
}
#
########################################################################

__END__

=head1 NAME

dcps_ts.pl - generate DDS DCPS TypeSupport for a specified IDL file.

=head1 SYNOPSIS

perl ./dcps_ts.pl [options] [IDLfile]

 --help      - print a brief usage message and exit
 --man       - print manual page and exit
 --verbose   - execute in a wordy fashion
 --debug     - execute additional debug statements
 --dir=S     - subdirectory for input and output files
 --export=S  - export macro to use
 --pch=S     - PreCompiled Header file to be included
 --idl=S     - IDL defining types to be supported
 --timestamp - append a timestamp to generated filenames
 --nobackup  - do NOT append a timestamp to generated filenames
 --cpp_only  - only generate the .cpp file
 --extension - followed by =path/to/module, adds a Perl module for
               additional functionality.  The file path/to/module.pm must
               exist as a valid Perl module and have a generate() method
               which takes two arguments: 1. the package name and
               2. an array-reference where the array will contain the list
               of DCPS struct names.
               The generate() method should return a hash-reference.
               See the code for how this is used.


=head1 OPTIONS

=over 8

=item B<--verbose | -v>

Enables verbose execution.

=item B<--noverbose>

Force quiet operation.

=item B<--debug | -d>

Enables debug statements in the test.

=item B<--help | -h>

Prints a brief usage message and exits.

=item B<--man>

Prints the manual page and exits.

=item B<--dir | -S> = string

Indicates the subdirectory in which the IDL file is located.  This is
used to modify the include directives in the generated code.

=item B<--export | -X> = string

Indicates the export macro to be used when generating C++ implementation
code.  If not specified, there will be no export macro placed in the
generated code.

=item B<--pch> = string

Indicates the Precompiled Header file to be included in the generated C++
implementation code.  If not specified, there will be no inclue placed 
in the generated code.

=item B<--idl> = string

Specifies the IDL file where the types are defined.  This option can be used
if the IDL is not specified positionaly.  If the IDL file is specified,
this option is ignored.

=item B<--timestamp | -t>

Causes a timestamp to be appended to the generated filenames. 

=item B<--nobackup>

Forces no timestamp to be appended to the generated filenames.

=item B<--extension> = module

Allows additional functionality to be added at runtime through another
Perl module.

=back

=head1 DESCRIPTION

This script generates IDL files and implementation code required for
supporting IDL specified types in DDS.  The I<TypeSupport.idl> file will
contain all of the required IDL interface and type definitions in order
to support a type for use by the DDS DCPS interfaces.  The generated IDL
file can then be compiled with the IDL compiler to generate the stubs
and skeletons required by the implementations generated by this script.

=head1 EXAMPLES

  ./dcps_ts.pl -v -S subdir -X FooLib_export -M Mine Xyz::Foo FooDef.idl

=cut

