eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

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

my %excludes = ();
my $output = '$DDS_ROOT/coverage_report';
my $gcov_tool = 'gcov';
my $verbose = 0;
my $source_root = $DDS_ROOT;
my $run_dir = $DDS_ROOT;
my $limit;
my $rem_info_files = 1;
my $collect_cov = 1;
my $capture_info = 1;
my $limit_capture_info = 1;
my $create_final_cov = 1;

sub traverse {
    my $params = shift;
    my $dir = shift;
    my $depth = shift;

    if (!defined($dir)) {
      $dir = "$run_dir";
    }
    if (!defined($depth)) {
      $depth = 1;
    }

    my $dh;
    opendir($dh, $dir);
    my @entries = grep { !/^\.\.?$/ } readdir($dh);
    closedir($dh);
    my $continue = 1;
    foreach my $entry (@entries) {
        my $full_entry = "$dir/$entry";
        $full_entry =~ s/\/\//\//g;
        if (-d $full_entry) {
            if ($excludes{$entry}) {
                next;
            }
            if ($params->{dir_function}) {
                $continue = &{$params->{dir_function}}($params, $full_entry);
            }

            if (($continue == 1) &&
                (!defined($params->{depth}) || $params->{depth} > $depth)) {
                $continue = traverse($params, $full_entry, $depth + 1);
            }
        }
        elsif ($params->{file_function}) {
            $continue = &{$params->{file_function}}($params, $full_entry);
        }
        if ($continue == -1) {
            return -1;
        }
        else {
            $continue = 1;
        }
    }

    return 1;
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

sub findFileRelativePath
{
    my $params = shift;
    my $file = shift;

    my $to_find = $params->{to_find};
    if ($file =~ /$to_find$/) {
        $params->{found_path} = $file;
        return -1;
    }

    return 1;
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
    if ((scalar(@gcno_entries) > 0) &&
        (scalar(keys(%gcda_entries)) == 0)) {
        # just record the whole directory
        print {$params->{no_cov_fh}} "$dir/\n";
    }
    else {
        foreach my $prefix (@gcno_entries) {
            if (!defined($gcda_entries{$prefix})) {
                print {$params->{no_cov_fh}} "$dir/$prefix\n";
            }
        }
    }
    return 1;
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

sub clean_path {
    my $path = shift;
    $path =~ s/\\+/\//g;
    $path =~ s/\/+/\//g;
    $path =~ s/\/\.\//\//g;
    $path =~ s/\/[^\/]+\/\.\.\//\//g;
    $path =~ s/\/\.$//g;
    $path =~ s/\/$//g;
    $path =~ s/^\.\///g;
    return $path;
}

sub for_search {
    my $search_str = shift;
    clean_path($search_str);
    $search_str =~ s/\//\\\//g;
    $search_str =~ s/\./\\\./g;
    return $search_str;
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
    elsif ($arg eq "-no_rem_info_files") {
        $rem_info_files = 0;
    }
    elsif ($arg eq "-no_collect_cov") {
        $collect_cov = 0;
    }
    elsif ($arg eq "-no_capture_info") {
        $capture_info = 0;
    }
    elsif ($arg eq "-no_limit_capture_info") {
        $limit_capture_info = 0;
    }
    elsif ($arg eq "-no_create_final_cov") {
        $create_final_cov = 0;
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

if (!defined($source_root) || $source_root eq "") {
    $source_root = $run_dir;
}
elsif (!defined($run_dir) || $run_dir eq "") {
    $run_dir = $source_root;
}

if ($cleanup_only) {
    print "Coverage: removing *.gcda\n" if $verbose;
    # traverse $run_dir and remove all *.gcda files
    traverse( { 'file_function' => \&removeFiles, 'extension' => 'gcda' });

    exit 1;
}

if (defined($limit)) {
    $limit = "$source_root/$limit";
    clean_path($limit);
}

print "  source_root=$source_root\n" if $verbose;
print "  run_dir=$run_dir\n" if $verbose;
print "  verbose=$verbose\n" if $verbose;
print "  output=$output\n" if $verbose;
print "  limit=$limit\n" if $verbose && defined($limit);

if ($rem_info_files) {
    print "Coverage: removing *.info\n" if $verbose;
    # traverse $run_dir and remove all *.info files
    traverse( { 'file_function' => \&removeFiles, 'extension' => 'info' });
}

my $no_cov_filename = "$run_dir/file_dirs_with_no_coverage.lst";
if ($collect_cov) {
    print "Coverage: identify no coverage classes\n" if $verbose;
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
    close(NO_COV_FILE);
}

my $original_dir;
# use lcov to convert *.gcda files into a *.info file
my $operating_dir = "$run_dir/dds";
my $lcov_base = "--base-directory $source_root/dds ";
my $lcov_dir = "--directory $operating_dir ";
if ($source_root eq $run_dir) {
    $original_dir = getcwd();
    chdir($operating_dir);
    $operating_dir = ".";
    $lcov_base = "";
    $lcov_dir = "--directory $operating_dir ";
}

my $output1 = "$run_dir/to_clean_cov.info";
my $output2 = "";
if (defined($limit)) {
    $output2 = $output1;
    $output1 = "$run_dir/to_limit_cov.info";
}

my $status = 0;
if ($capture_info) {
    my $dds_output = "$run_dir/dds_cov.info";
    print "Coverage: collect dds coverage data\n" if $verbose;
    $status = system("lcov --capture --gcov-tool $gcov_tool $lcov_base" .
                     "--directory $operating_dir --output-file $dds_output " .
                     "--list-full-path --ignore-errors gcov,source --follow");
    if (!$status) {
        system("find $operating_dir -name \"*.gcda\" |" .
               " xargs tar czf old_gcda.tgz");
        system("find $operating_dir -name \"*.gcda\" | xargs rm ");
        if (defined($original_dir)) {
            chdir("..");
            $lcov_base = "--base-directory $source_root/dds ";
        }
        else {
            $operating_dir = $run_dir;
        }
        my $test_output = "$run_dir/test.info";
        print "Coverage: collect test coverage data\n" if $verbose;
        $status =
            system("lcov --capture --gcov-tool $gcov_tool " .
                   "--directory $operating_dir --output-file $test_output " .
                   "--list-full-path --ignore-errors gcov,source --follow");
        if (!$status) {
            print "Coverage: combine coverage data\n" if $verbose;
            $status =
                system("lcov --add-tracefile $dds_output --add-tracefile " .
                       "$test_output --output-file $output1");
        }
    }
}

if ($limit_capture_info) {
    if (!$status && defined($limit)) {
        $status = system("lcov --gcov-tool $gcov_tool --output-file $output2 " .
                         "--list-full-path --ignore-errors gcov,source --follow --extract $output1 \"$limit/*\" ");
    }
}
# try to clean up any file paths that lcov got confused on because of relative paths
my $record = "";
my $search_path;
if (defined($limit)) {
    $search_path = $limit;
}
else {
    $search_path = $source_root;
}
my $search_str = for_search($search_path);

if ($create_final_cov) {

    if (!open(CLEANED_INFO_FILE, ">", "$operating_dir/final_cov.info")) {
        print STDERR __FILE__, ": Cannot write to $operating_dir/final_cov.info\n";
        exit 1;
    }
    if (!open(DROPPED_INFO_FILE, ">", "$operating_dir/dropped_cov.info")) {
        print STDERR __FILE__, ": Cannot write to $operating_dir/dropped_cov.info\n";
        exit 1;
    }
    if (!open(TO_CLEAN_INFO_FILE, "<", "$operating_dir/to_clean_cov.info")) {
        print STDERR __FILE__, ": Cannot read from $operating_dir/to_clean_cov.info\n";
        exit 1;
    }
    my $valid = 1;
    my $record_file = "";
    while (<TO_CLEAN_INFO_FILE>) {
        my $line = $_;
        if ($line =~ /^\s*end_of_record\s*$/) {
            $record .= $line;
            if ($valid) {
                print CLEANED_INFO_FILE "$record";
            }
            elsif ($verbose) {
                print DROPPED_INFO_FILE "$record";
            }
            $record = "";
            $valid = 1;
            next;
        }
        if ($line =~ /^\s*SF:$search_str\/(\S+)\s*$/) {
            $record_file = $1;
            unless (-f "$search_path\/$record_file") {
                $valid = 0;
                my $rel_path = $record_file;
                my $dir_ss = "";
                while (1) {
                    $dir_ss .= "[^\/]\/";
                    if ($rel_path !~ /^($dir_ss)/) {
                        last;
                    }
                    my $base = $1;
                    my $orig_line = $line;
                    if ($line =~ s/^(\s*SF:$search_str\/)$base$base/$1$base/) {
                        print "cleaned up:\n  $orig_line\nbecomes:\n  $line\n" if $verbose;
                        $valid = 1;
                        last;
                    }
                }
                if (!$valid) {
                    $record_file =~ /.*?([^\/]+)$/;
                    my $params = {
                        'file_function' => \&findFileRelativePath,
                        'to_find' => $1 };
                    traverse($params, $search_path);
                    if (defined($params->{found_path})) {
                        my $orig_line = $line;
                        my $found_path = $params->{found_path};
                        $line =~ s/^(\s*SF:)$search_str(.*)$/$1$found_path\//;
                        print "changed:\n  $orig_line\nbecomes:\n  $line\n" if $verbose;
                        $valid = 1;
                    }
                }
            }
            if (!$valid && $verbose) {
                print "/n/nERROR: file=$record_file could not be found" .
                    " under $search_path, dropping!.\n";
                $record_file =~ /.*?([^\/]+)$/;
                system("find $search_path -name $1");
                print "\n\n";
            }
        }
        $record .= "$line";
    }
    close(TO_CLEAN_INFO_FILE);
    if ($record ne "") {
        print CLEANED_INFO_FILE "$record\n";
    }
    close(CLEANED_INFO_FILE);
}

if ($collect_cov) {
    if (!open(NO_COV_FILE, "<", "$no_cov_filename")) {
        print STDERR __FILE__, ": Cannot open $no_cov_filename for limiting coverage to root\n";
        next;
    }
    if (<NO_COV_FILE>) {
        print "Coverage: see $no_cov_filename for files with no coverage data\n";
    }
    close(NO_COV_FILE);
}

if (!$status) {
    if (-d $output) {
        print "Coverage: removing old coverage at $output\n" if $verbose;
        rmtree($output, 0, 1);
    }
    my $prefix = "";
    if (defined($limit)) {
        $prefix = $limit;
        $prefix =~ s/\/[^\/]+\/?$//;
        $prefix = "--prefix $prefix";
    }
    my $command = "genhtml $prefix --output-directory $output --demangle-cpp $operating_dir/final_cov.info";
    print "Coverage: generating html <$command>\n" if $verbose;
    $status = system ($command) == 0;
}

chdir($original_dir) if (defined($original_dir));

exit $status;
