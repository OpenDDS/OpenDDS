$JAVA_HOME/bin/idlj -v -fall -pkgPrefix $1 idlj $1.idl
../../bin/idl2jni -v $1.idl -DUSE_LOCAL
$JAVA_HOME/bin/javac -classpath ../../lib/i2jrt.jar $1/*.java
$ACE_ROOT/bin/mpc.pl -type gnuace -include ../.. -base idl2jni -value_project libout=. -value_template TAO_IDLFLAGS=-DUSE_LOCAL
make -f GNUmakefile.$1
