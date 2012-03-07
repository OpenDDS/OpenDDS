eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";

use strict;
use warnings;

use Cwd;
use File::Path;
use File::Find;

my %infos = ();
my %excludes = ();
my $output = '$DDS_ROOT/coverage_report';
my $gcov_tool = 'gcov';
my $info_groups = 0;
my $verbose = 0;
my $source_root = $DDS_ROOT;
my $run_dir = $DDS_ROOT;
my $limit;

sub traverse {
    my $params = shift;
    my $dir = shift;
    my $depth = shift;

    if (!defined($dir)) {
      $dir = $run_dir;
    }
    if (!defined($depth)) {
      $depth = 1;
    }

    my $dh;
    opendir($dh, $dir);
    my @entries = grep { !/^\.\.?$/ } readdir($dh);
    closedir($dh);
    foreach my $entry (@entries) {
        my $full_entry = "$dir/$entry";
        $full_entry =~ s/\/\//\//g;
        if (-d $full_entry) {
            if ($excludes{$entry}) {
                next;
            }
            if ($params->{dir_function}) {
                &{$params->{dir_function}}($params, $full_entry);
            }

            if (!defined($params->{depth}) || $params->{depth} > $depth) {
                traverse($params, $full_entry, $depth + 1);
            }
        }
        elsif ($params->{file_function}) {
            &{$params->{file_function}}($params, $full_entry);
        }
    }
}

sub removeFiles
{
    my $params = shift;
    my $file = shift;

    my $file_pattern = ".+";
    if (defined($params->{file_pattern})) {
        $file_pattern = $params->{file_pattern};
    }
    my $extension = $params->{extension};
    if ($file =~ /$file_pattern\.$extension$/) {
        unlink($file);
    }
}

sub findNoCoverage
{
    my $params = shift;
    my $dir = shift;
    
    my $dh;
    opendir($dh, $dir);
    my @entries = readdir($dh);
    closedir($dh);
    # collect all *.gdno and *.gcda files
    my @gcno_entries = ();
    my %gcda_entries = ();
    foreach my $entry (@entries) {
        if ($entry =~ s/\.gcno$//) {
            push(@gcno_entries, $entry);
        }
        elsif ($entry =~ s/\.gcda$//) {
            $gcda_entries{$entry} = 1;
        }
    }
    foreach my $prefix (@gcno_entries) {
        if (!define($gcda_entries{$prefix})) {
            print {$params->{no_cov_fh}} "$dir/$prefix\n";
        }
    }
}

sub createInfo
{
    my $params = shift;
    my $dir = shift;

    print "find *.gcda files in $dir\n" if $verbose;
    # try all these locations to find *.gcda files
    my @obj_dirs = ( "$dir/.obj", "$dir/.shobj", "$dir" );
    foreach my $obj_dir (@obj_dirs) {
        unless (-d $obj_dir) {
            next;
        }

        my $dh;
        opendir($dh, $obj_dir);
        # collect all *.gcda files
        my @entries = sort grep { /\.gcda$/ } readdir($dh);
        closedir($dh);
        if (scalar(@entries) > 0) {
            my $info = "$dir.info";
            my $initial_info = $info;
            if (defined($limit)) {
                $initial_info .= ".tmp";
            }
            my $status = system("lcov -c --gcov-tool $gcov_tool -b $dir -d $obj_dir -o $initial_info");
            if ($status) {
                print {$params->{no_cov_fh}} "$dir\n";
            }
            elsif (defined($limit)) {
                $status = system("lcov --gcov-tool $gcov_tool -o $info -e $initial_info \"$limit\"");
                unlink($initial_info);
                # if lcov failed, or if there is no coverage
                # in the code we care about, then remove the file
                if ($status || -z $info) {
                    if ($dir =~ /^$limit/) {
                        print {$params->{no_cov_fh}} "$dir\n";
                    }
                    unlink($info);
                    $status = 1;
                }
            }
            $infos{"$info"} = 1 if $status == 0;

            # don't need to keep looking for *.gcda files
            last;
        }
    }
}

sub combineInfos
{
    my @infoKeys = sort { $a cmp $b } keys(%infos);
    my $size = scalar(@infoKeys);
    print "combining $size info files\n" if $verbose;

    my $index = 0;
    my $group;
    if ($info_groups != 0 && $info_groups < $size) {
        my @groupedInfos = ();
        while ($index < $size) {
            my $addstr = "";
            my $groupStart = $index;
            if ($size - $groupStart < $info_groups) {
                $info_groups = $size - $groupStart;
            }
            while (($index - $groupStart) < $info_groups) {
                $addstr .= " -a $infoKeys[$index]";
                $index += 1;
            }
            $group += 1;
            my $covFile = "$run_dir/intermediate_cov" . $group . ".info";

            print "\ncombining $info_groups info files $covFile <$addstr>\n if $verbose";
            my $status = system("lcov -d $source_root $addstr -o $covFile");
            if ($status == 0) {
                push(@groupedInfos, $covFile);
            }
            else {
                print STDERR "Failed to create intermediate info file=$covFile <$addstr>\n";
            }
        }
        @infoKeys = ();
        push(@infoKeys, @groupedInfos);
    }

    # combining either all info files or the intermediate files
    my $addstr = "";
    foreach my $info (@infoKeys) {
        $addstr .= " -a $info";
    }
    my $covFile = "$run_dir/all_cov.info";
    print "\ncreating $covFile <$addstr>\n" if $verbose;
    my $status = system("lcov -d $source_root $addstr -o $covFile");
    if ($status != 0) {
        print STDERR "Failed to create $covFile <$addstr>\nWill not generate html\n";
    }
    return $status;
}

sub yesterday
{
    my $day = shift;

    if ($day eq "sun") { return "sat" };
    if ($day eq "mon") { return "sun" };
    if ($day eq "tue") { return "mon" };
    if ($day eq "wed") { return "tue" };
    if ($day eq "thu") { return "wed" };
    if ($day eq "fri") { return "thu" };
    if ($day eq "sat") { return "fri" };

    print STDERR "Failed to identify $day as day of the week, cannot determine yesterday\n";
    exit 0;
}

my %days = ();
my $cleanup_only = 0;
for (my $i = 0; $i <= $#ARGV; ++$i) {
    my $arg = $ARGV[$i];
    if ($arg eq "-cleanup_only") {
        $cleanup_only = 1;
    }
    elsif ($arg eq "-v" || $arg eq "-verbose") {
        $verbose = 1;
    }
    elsif (($arg eq "-o" || $arg eq "-output") && (++$i <= $#ARGV)) {
        $output = $ARGV[$i];
    }
    elsif (($arg eq "-gcov_tool") && (++$i <= $#ARGV)) {
        $gcov_tool = $ARGV[$i];
    }
    elsif (($arg eq "-info_groups") && (++$i <= $#ARGV)) {
        $info_groups = $ARGV[$i];
    }
    elsif (($arg eq "-x" || $arg eq "-exclude") && (++$i <= $#ARGV)) {
        $excludes{$ARGV[$i]} = 1;
    }
    elsif (($arg eq "-limit") && (++$i <= $#ARGV)) {
        $limit = $ARGV[$i];
    }
    elsif (($arg eq "-source_root") && (++$i <= $#ARGV)) {
        $source_root = $ARGV[$i];
    }
    elsif (($arg eq "-run_dir") && (++$i <= $#ARGV)) {
        $run_dir = $ARGV[$i];
    }
    elsif (($arg eq "-day") && (++$i <= $#ARGV)) {
        my $day = lc($ARGV[$i]);
        $days{$day} = 1;
    }
    else {
        print "Ignoring unkown argument: $ARGV[$i]\n";
    }
}

if (keys(%days) > 0) {
    my $now_str = localtime;
    #                  day     month date   hour   min  sec    year
    if ($now_str !~ /^(\S+)\s+\S+\s+\S+\s+(\d\d?):\d\d:\d\d\s+\S+/) {
        print "ERROR: couldn't parse localtime string, skipping coverage\n";
        exit 0;
    }
    my $day = lc($1);
    my $hour = $2;
    # before 6pm builds start, shift to previous day
    if ($hour < 18) {
        $day = yesterday($day);
    }

    if (!$days{$day}) {
        print "Skipping coverage on $day (build time frame)\n";
        exit 1;
    }
}

if ($cleanup_only) {
    print "Coverage removing *.gcda\n" if $verbose;
    # traverse $run_dir and remove all *.gcda files
    traverse( { 'file_function' => \&removeFiles, 'extension' => 'gcda' });

    exit 1;
}

print "Collect Coverage Data\n";

if (defined($limit)) {
    $limit = "$source_root/$limit/*";
    $limit =~ s/\\+/\//g;
    $limit =~ s/\/+/\//g;
    $limit =~ s/\/\.\//\//g;
    $limit =~ s/\/[^\/]+\/\.\.\//\//g;
    $limit =~ s/\/\./\//g;
}
print "source_root=$source_root\n" if $verbose;
print "run_dir=$run_dir\n" if $verbose;
print "verbose=$verbose\n" if $verbose;
print "output=$output\n" if $verbose;
print "limit=$limit\n" if $verbose && defined($limit);

print "Coverage: removing *.info\n" if $verbose;
# traverse $run_dir and remove all *.info files
traverse( { 'file_function' => \&removeFiles, 'extension' => 'info' });

print "Coverage: collect coverage data\n" if $verbose;

my $no_cov_filename = "$run_dir/file_dirs_with_no_coverage.lst";
if (-f $no_cov_filename) {
    # maintain the previous file so we know if new classes are added
    # to the list
    system("mv $no_cov_filename $no_cov_filename.bak");
}

if (!open(NO_COV_FILE, ">", "$no_cov_filename")) {
    print STDERR __FILE__, ": Cannot write to $no_cov_filename for indicating files with no coverage data\n";
    exit 1;
}
traverse({ 'dir_function' => \&findNoCoverage,
           'no_cov_fh' => \*NO_COV_FILE });
# createInfo accounts for .*obj directories, so don't traverse
$excludes{".obj"} = 1;
$excludes{".shobj"} = 1;
$excludes{".cov_temp_obj"} = 1;
# use lcov to convert *.gcda files into a *.info file
traverse({ 'dir_function' => \&createInfo,
           'no_cov_fh' => \*NO_COV_FILE });
close(NO_COV_FILE);

if (!open(NO_COV_FILE, "<", "$no_cov_filename")) {
    print STDERR __FILE__, ": Cannot open $no_cov_filename for limiting coverage to root\n";
    next;
}
if (<NO_COV_FILE>) {
    print "Coverage: see $no_cov_filename for files with no coverage data\n";
}
close(NO_COV_FILE);

print "Coverage: combine all coverage data\n" if $verbose;
my $status = 0;
if (!combineInfos($run_dir)) {
    if (-d $output) {
        rmtree($output, 0, 1);
    }
    my $prefix = "";
    if (defined($limit)) {
        $prefix = $limit;
        $prefix =~ s/\/[^\/]+\/\*$//;
        $prefix = "--prefix $prefix";
    }
    my $command = "genhtml $prefix -o $output $run_dir/all_cov.info";
    print "Coverage: generating html <$command>\n" if $verbose;
    $status = system ($command) == 0;
}

if ($info_groups) {
    print "Coverage: removing intermediate_cov*.info\n" if $verbose;
    # traverse $run_dir and remove all intermediate_cov*.info files
    traverse({ 'file_function' => \&removeFiles,
               'file_pattern' => '.?intermediate_cov.*',
               'extension' => 'info' });
}

exit $status;
