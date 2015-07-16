/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DDS_DATAREADER_QOS_DEFAULT.h"
#include "DDS_DATAREADER_QOS_USE_TOPIC_QOS.h"
#include "DDS_DATAWRITER_QOS_DEFAULT.h"
#include "DDS_DATAWRITER_QOS_USE_TOPIC_QOS.h"
#include "DDS_PARTICIPANT_QOS_DEFAULT.h"
#include "DDS_PUBLISHER_QOS_DEFAULT.h"
#include "DDS_SUBSCRIBER_QOS_DEFAULT.h"
#include "DDS_TOPIC_QOS_DEFAULT.h"
#include "idl2jni_runtime.h"

#include "dds/DCPS/Marked_Default_Qos.h"

#include "DdsDcpsInfrastructureJC.h"

jobject JNICALL Java_DDS_DATAREADER_1QOS_1DEFAULT_get(JNIEnv *jni, jclass)
{
  jobject obj;
  copyToJava(jni, obj, DATAREADER_QOS_DEFAULT, true);
  return obj;
}

jobject JNICALL Java_DDS_DATAREADER_1QOS_1USE_1TOPIC_1QOS_get(JNIEnv *jni,
                                                              jclass)
{
  jobject obj;
  copyToJava(jni, obj, DATAREADER_QOS_USE_TOPIC_QOS, true);
  return obj;
}

jobject JNICALL Java_DDS_DATAWRITER_1QOS_1DEFAULT_get(JNIEnv *jni, jclass)
{
  jobject obj;
  copyToJava(jni, obj, DATAWRITER_QOS_DEFAULT, true);
  return obj;
}

jobject JNICALL Java_DDS_DATAWRITER_1QOS_1USE_1TOPIC_1QOS_get(JNIEnv *jni,
                                                              jclass)
{
  jobject obj;
  copyToJava(jni, obj, DATAWRITER_QOS_USE_TOPIC_QOS, true);
  return obj;
}

jobject JNICALL Java_DDS_PARTICIPANT_1QOS_1DEFAULT_get(JNIEnv *jni, jclass)
{
  jobject obj;
  copyToJava(jni, obj, PARTICIPANT_QOS_DEFAULT, true);
  return obj;
}

jobject JNICALL Java_DDS_TOPIC_1QOS_1DEFAULT_get(JNIEnv *jni, jclass)
{
  jobject obj;
  copyToJava(jni, obj, TOPIC_QOS_DEFAULT, true);
  return obj;
}

jobject JNICALL Java_DDS_PUBLISHER_1QOS_1DEFAULT_get(JNIEnv *jni, jclass)
{
  jobject obj;
  copyToJava(jni, obj, PUBLISHER_QOS_DEFAULT, true);
  return obj;
}

jobject JNICALL Java_DDS_SUBSCRIBER_1QOS_1DEFAULT_get(JNIEnv *jni, jclass)
{
  jobject obj;
  copyToJava(jni, obj, SUBSCRIBER_QOS_DEFAULT, true);
  return obj;
}
