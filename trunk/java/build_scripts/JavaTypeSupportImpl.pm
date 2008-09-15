package JavaTypeSupportImpl;

use File::Path;
use File::Basename;

sub generate {
    my $pkg = shift;
    my $aref = shift;
    my %extra;
    $extra{'cpp'} = '#include "idl2jni_jni.h"' . "\n";
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
  return reinterpret_cast<jlong> (CORBA::Object::_duplicate (new ${type}TypeSupportImpl));
}
EOT
    }
    return \%extra;
}

1;
