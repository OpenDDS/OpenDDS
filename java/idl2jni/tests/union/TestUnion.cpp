#include "UnionTestJC.h"

extern "C" JNIEXPORT jobject JNICALL
Java_TestUnion_getUnionND(JNIEnv* jni, jclass, jobject src)
{
  NoDefault u;
  copyToCxx(jni, u, src);
  jobject u2;
  copyToJava(jni, u2, u, true);
  return u2;
}

extern "C" JNIEXPORT jobject JNICALL
Java_TestUnion_getUnionWD(JNIEnv* jni, jclass, jobject src)
{
  WithDefault u;
  copyToCxx(jni, u, src);
  jobject u2;
  copyToJava(jni, u2, u, true);
  return u2;
}
