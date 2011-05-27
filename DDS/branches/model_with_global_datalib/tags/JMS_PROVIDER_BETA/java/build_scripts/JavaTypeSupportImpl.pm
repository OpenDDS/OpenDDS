package JavaTypeSupportImpl;

use File::Path;
use File::Basename;

sub generate {
    my $pkg = shift;
    my $aref = shift;
    my $ext_args = shift;
    my $file_names = shift;

    my $cpp_file = undef;
    if ($ext_args =~ /cpp_file=(\S+)/) {
      $cpp_file = $1;
    }

    my %extra;
    $extra{'cpp'} = '#include "idl2jni_jni.h"' . "\n\n";
    $extra{'cpp'}.= '#include "' . $file_names->[1] . "\"\n\n" if $cpp_file;
    for my $type (@$aref) {
        my @path = split ('::', $type);
        my $file = join ('/', @path) . 'TypeSupportImpl.java';
        my $jniclass = join ('_', @path);
        my @pkg = @path;
        my $class = pop @pkg;
        my $jpackage = (scalar @pkg == 0) ? ''
            : 'package ' . join ('.', @pkg) . ";\n";
        mkpath (dirname ($file), 0, 0777);
        open (JAVA, ">$file") or die "Failed to open $file\n";
        print JAVA <<EOT;
$jpackage
public class ${class}TypeSupportImpl extends _${class}TypeSupportTAOPeer {

    public ${class}TypeSupportImpl() {
        super(_jni_init());
    }

    private static native long _jni_init();

}
EOT
        close JAVA;
        $extra{'cpp'} .= <<EOT;
extern "C" JNIEXPORT jlong JNICALL
Java_${jniclass}TypeSupportImpl__1jni_1init (JNIEnv *, jclass)
{
  return reinterpret_cast<jlong> (static_cast<CORBA::Object_ptr> (new ${type}TypeSupportImpl));
}

EOT
    } # for each type...

    if ($cpp_file) {
      open (CPP, ">$cpp_file") or die "Failed to open $file\n";
      print CPP $extra{'cpp'};
      close CPP;
      delete $extra{'cpp'};
    }

    return \%extra;
}

1;
