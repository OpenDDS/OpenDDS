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

sub reduce
{
    my $input_file = shift;
    my $file = shift;

    if (!open(INPUT, "<", "$input_file")) {
        print STDERR __FILE__, ": Cannot open $input_file for limiting coverage to root\n";
        next;
    }
    if (!open(OUTPUT, ">", "$file")) {
        print STDERR __FILE__, ": Cannot write to $file for limiting coverage to root\n";
        close(INPUT);
        next;
    }
    my $record = "";
    my $print = 0;
    while (<INPUT>) {
        $record .= $_;
        if (/^\s*SF:$limit/) {
            $print = 1;
        }
        if (/^\s*end_of_record\s*$/) {
            # complete record found, only write it if it is under the $source_root
            if ($print) {
                print OUTPUT "$record";
            }
            $record = "";
            $print = 0;
        }
    }
    close(INPUT);
    close(OUTPUT);
    unlink($input_file);
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
        my @entries = grep { /\.gcda$/ } readdir($dh);
        closedir($dh);
        if (scalar(@entries) > 0) {
            my $info = "$dir.info";
            my $initial_info = $info;
            if (defined($limit)) {
                $initial_info .= ".tmp";
            }
            my $status = system("lcov -c --gcov-tool $gcov_tool -b $dir -d $obj_dir -o $initial_info");
            if (!$status && defined($limit)) {
                $status = system("lcov --gcov-tool $gcov_tool -o $info -e $initial_info \"$limit\"");
                unlink($initial_info);
                # if lcov failed, or if there is no coverage
                # in the code we care about, then remove the file
                if ($status || -z $info) {
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

my $cleanup_only = 0;
for (my $i = 0; $i <= $#ARGV; ++$i) {
    my $arg = $ARGV[$i];
    if ($arg eq "-cleanup_only") {
        $cleanup_only = 1;
    }
    elsif ($arg eq "-v" || $arg eq "-verbose") {
        $verbose = 1;
    }
    elsif ($arg eq "-o" || $arg eq "-output") {
        $output = $ARGV[$i] if (++$i <= $#ARGV);
    }
    elsif ($arg eq "-gcov_tool") {
        $gcov_tool = $ARGV[$i] if (++$i <= $#ARGV);
    }
    elsif ($arg eq "-info_groups") {
        $info_groups = $ARGV[$i] if (++$i <= $#ARGV);
    }
    elsif ($arg eq "-x" || $arg eq "-exclude") {
        $excludes{$ARGV[$i]} = 1 if (++$i <= $#ARGV);
    }
    elsif ($arg eq "-limit") {
        $limit = $ARGV[$i] if (++$i <= $#ARGV);
    }
    elsif ($arg eq "-source_root") {
        $source_root = $ARGV[$i] if (++$i <= $#ARGV);
    }
    elsif ($arg eq "-run_dir") {
        $run_dir = $ARGV[$i] if (++$i <= $#ARGV);
    }
    else {
        print "Ignoring unkown argument: $ARGV[$i]\n";
    }
}

if (defined($limit)) {
    $limit = "$source_root/$limit/*";
    $limit =~ s/\\+/\//g;
    $limit =~ s/\/+/\//g;
    $limit =~ s/\/\.\//\//g;
    $limit =~ s/\/[^\/]+\/\.\.\//\//g;
    $limit =~ s/\/\./\//g;
#    $limit =~ s/\//\\\//g;
}
print "source_root=$source_root\n" if $verbose;
print "run_dir=$run_dir\n" if $verbose;
print "verbose=$verbose\n" if $verbose;
print "output=$output\n" if $verbose;
print "limit=$limit\n" if $verbose && defined($limit);

if ($cleanup_only) {
    print "Coverage removing *.gcda\n" if $verbose;
    # traverse $run_dir and remove all *.gcda files
    traverse( { 'file_function' => \&removeFiles, 'extension' => 'gcda' });

    exit 1;
}

print "Coverage: removing *.info\n" if $verbose;
# traverse $run_dir and remove all *.info files
traverse( { 'file_function' => \&removeFiles, 'extension' => 'info' });

print "Coverage: collect coverage data\n" if $verbose;
# createInfo accounts for .*obj directories, so don't traverse
$excludes{".obj"} = 1;
$excludes{".shobj"} = 1;
$excludes{".cov_temp_obj"} = 1;
# use lcov to convert *.gcda files into a *.info file
traverse({ 'dir_function' => \&createInfo });

print "Coverage: combine all coverage data\n" if $verbose;
my $status = 0;
if (!combineInfos($run_dir)) {
    if (-d $output) {
        rmtree($output, 1, 1);
    }
    my $command = "genhtml -o $output $run_dir/all_cov.info";
    print "Coverage: generating html\n" if $verbose;
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
