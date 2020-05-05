#include "UnionTestJC.h"

extern "C" JNIEXPORT jobject JNICALL
Java_TestUnion_getUnion(JNIEnv* jni, jclass, jobject src)
{
  U u;
  copyToCxx(jni, u, src);
  jobject u2;
  copyToJava(jni, u2, u, true);
  return u2;
}
