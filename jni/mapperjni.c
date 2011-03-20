
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <mapper/mapper.h>

#include "Mapper_Device.h"

#define jlong_ptr(a) ((jlong)(uintptr_t)(a))
#define ptr_jlong(a) ((void *)(uintptr_t)(a))

JNIEnv *genv=0;
int bailing=0;

/**** Mapper.Device ****/

JNIEXPORT jlong JNICALL Java_Mapper_Device_mdev_1new
  (JNIEnv *env, jobject obj, jstring name, jint port)
{
    const char *cname = (*env)->GetStringUTFChars(env, name, 0);
    mapper_device d = mdev_new(cname, port, 0);
    (*env)->ReleaseStringUTFChars(env, name, cname);
    return jlong_ptr(d);
}

JNIEXPORT void JNICALL Java_Mapper_Device_mdev_1free
  (JNIEnv *env, jobject obj, jlong d)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    mdev_free(dev);
}

JNIEXPORT int JNICALL Java_Mapper_Device_mdev_1poll
  (JNIEnv *env, jobject obj, jlong d, jint timeout)
{
    genv = env;
    bailing = 0;
    mapper_device dev = (mapper_device)ptr_jlong(d);
    return mdev_poll(dev, timeout);
}

static void java_msig_input_cb(mapper_signal sig, void *v)
{
    if (bailing)
        return;

    mapper_db_signal props = msig_properties(sig);
    jobject listener = (jobject)props->user_data;
    if (listener) {
        jclass cls = (*genv)->GetObjectClass(genv, listener);
        if (cls) {
            jmethodID val = (*genv)->GetMethodID(genv, cls, "onInput", "()V");
            if (val) {
                (*genv)->CallVoidMethod(genv, listener, val);
                if ((*genv)->ExceptionOccurred(genv))
                    bailing = 1;
            }
        }
    }
}

JNIEXPORT jlong JNICALL Java_Mapper_Device_mdev_1add_1input
  (JNIEnv *env, jobject obj, jlong d, jstring name, jint length, jchar type, jstring unit, jobject minimum, jobject maximum, jobject listener)
{
    if (!d || !name || (length<=0) || (type!='f' && type!='i'))
        return 0;

    mapper_device dev = (mapper_device)ptr_jlong(d);

    const char *cname = (*env)->GetStringUTFChars(env, name, 0);
    const char *cunit = 0;
    if (unit) cunit = (*env)->GetStringUTFChars(env, unit, 0);

    union {
        float f;
        int i;
    } mn, mx;

    if (minimum) {
        jclass cls = (*env)->GetObjectClass(env, minimum);
        if (cls) {
            jfieldID val = (*env)->GetFieldID(env, cls, "value", "D");
            if (val) {
                if (type == 'f')
                    mn.f = (float)(*env)->GetDoubleField(env, minimum, val);
                else if (type == 'i')
                    mn.i = (int)(*env)->GetDoubleField(env, minimum, val);
            }
        }
    }

    if (maximum) {
        jclass cls = (*env)->GetObjectClass(env, maximum);
        if (cls) {
            jfieldID val = (*env)->GetFieldID(env, cls, "value", "D");
            if (val) {
                if (type == 'f')
                    mx.f = (float)(*env)->GetDoubleField(env, maximum, val);
                else if (type == 'i')
                    mx.i = (int)(*env)->GetDoubleField(env, maximum, val);
            }
        }
    }

    mapper_signal s = mdev_add_input(dev, cname, length, type, cunit,
                                     minimum ? &mn : 0,
                                     maximum ? &mx : 0,
                                     java_msig_input_cb,
                                     (*env)->NewGlobalRef(env, listener));

    (*env)->ReleaseStringUTFChars(env, name, cname);
    if (unit) (*env)->ReleaseStringUTFChars(env, unit, cunit);

    return jlong_ptr(s);
}

JNIEXPORT jlong JNICALL Java_Mapper_Device_mdev_1add_1hidden_1input
  (JNIEnv *env, jobject obj, jlong d, jstring name, jint length, jchar type, jstring unit, jobject minimum, jobject maximum, jobject listener)
{
    if (!d || !name || (length<=0) || (type!='f' && type!='i'))
        return 0;

    mapper_device dev = (mapper_device)ptr_jlong(d);

    const char *cname = (*env)->GetStringUTFChars(env, name, 0);
    const char *cunit = 0;
    if (unit) cunit = (*env)->GetStringUTFChars(env, unit, 0);

    union {
        float f;
        int i;
    } mn, mx;

    if (minimum) {
        jclass cls = (*env)->GetObjectClass(env, minimum);
        if (cls) {
            jfieldID val = (*env)->GetFieldID(env, cls, "value", "D");
            if (val) {
                if (type == 'f')
                    mn.f = (float)(*env)->GetDoubleField(env, minimum, val);
                else if (type == 'i')
                    mn.i = (int)(*env)->GetDoubleField(env, minimum, val);
            }
        }
    }

    if (maximum) {
        jclass cls = (*env)->GetObjectClass(env, maximum);
        if (cls) {
            jfieldID val = (*env)->GetFieldID(env, cls, "value", "D");
            if (val) {
                if (type == 'f')
                    mx.f = (float)(*env)->GetDoubleField(env, maximum, val);
                else if (type == 'i')
                    mx.i = (int)(*env)->GetDoubleField(env, maximum, val);
            }
        }
    }

    mapper_signal s = mdev_add_hidden_input(dev, cname, length, type, cunit,
                                            minimum ? &mn : 0,
                                            maximum ? &mx : 0,
                                            java_msig_input_cb,
                                            (*env)->NewGlobalRef(env,
                                                                 listener));

    (*env)->ReleaseStringUTFChars(env, name, cname);
    if (unit) (*env)->ReleaseStringUTFChars(env, unit, cunit);

    return jlong_ptr(s);
}

JNIEXPORT jlong JNICALL Java_Mapper_Device_mdev_1add_1output
  (JNIEnv *env, jobject obj, jlong d, jstring name, jint length, jchar type, jstring unit, jobject minimum, jobject maximum)
{
    if (!d || !name || (length<=0) || (type!='f' && type!='i'))
        return 0;

    mapper_device dev = (mapper_device)ptr_jlong(d);

    const char *cname = (*env)->GetStringUTFChars(env, name, 0);
    const char *cunit = 0;
    if (unit) cunit = (*env)->GetStringUTFChars(env, unit, 0);

    union {
        float f;
        int i;
    } mn, mx;

    if (minimum) {
        jclass cls = (*env)->GetObjectClass(env, minimum);
        if (cls) {
            jfieldID val = (*env)->GetFieldID(env, cls, "value", "D");
            if (val) {
                if (type == 'f')
                    mn.f = (float)(*env)->GetDoubleField(env, minimum, val);
                else if (type == 'i')
                    mn.i = (int)(*env)->GetDoubleField(env, minimum, val);
            }
        }
    }

    if (maximum) {
        jclass cls = (*env)->GetObjectClass(env, maximum);
        if (cls) {
            jfieldID val = (*env)->GetFieldID(env, cls, "value", "D");
            if (val) {
                if (type == 'f')
                    mx.f = (float)(*env)->GetDoubleField(env, maximum, val);
                else if (type == 'i')
                    mx.i = (int)(*env)->GetDoubleField(env, maximum, val);
            }
        }
    }

    mapper_signal s = mdev_add_output(dev, cname, length, type, cunit,
                                      minimum ? &mn : 0,
                                      maximum ? &mx : 0);

    (*env)->ReleaseStringUTFChars(env, name, cname);
    if (unit) (*env)->ReleaseStringUTFChars(env, unit, cunit);

    return jlong_ptr(s);
}

JNIEXPORT void JNICALL Java_Mapper_Device_mdev_1remove_1input
  (JNIEnv *env, jobject obj, jlong d, jlong s)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    mapper_signal sig = (mapper_signal)ptr_jlong(s);
    mdev_remove_input(dev, sig);
}

JNIEXPORT void JNICALL Java_Mapper_Device_mdev_1remove_1output
  (JNIEnv *env, jobject obj, jlong d, jlong s)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    mapper_signal sig = (mapper_signal)ptr_jlong(s);
    mdev_remove_output(dev, sig);
}

JNIEXPORT jint JNICALL Java_Mapper_Device_mdev_1num_1inputs
  (JNIEnv *env, jobject obj, jlong d)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    return mdev_num_inputs(dev);
}

JNIEXPORT jint JNICALL Java_Mapper_Device_mdev_1num_1outputs
  (JNIEnv *env, jobject obj, jlong d)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    return mdev_num_outputs(dev);
}

JNIEXPORT jlong JNICALL Java_Mapper_Device_mdev_1get_1input_1by_1name
  (JNIEnv *env, jobject obj, jlong d, jstring name, jobject index)
{
    int i;
    mapper_device dev = (mapper_device)ptr_jlong(d);
    const char *cname = (*env)->GetStringUTFChars(env, name, 0);
    mapper_signal sig = mdev_get_input_by_name(dev, cname, &i);
    (*env)->ReleaseStringUTFChars(env, name, cname);

    if (sig && index) {
        jclass cls = (*env)->GetObjectClass(env, index);
        if (cls) {
            jfieldID val = (*env)->GetFieldID(env, cls, "value", "I");
            if (val)
                (*env)->SetIntField(env, index, val, i);
        }
    }

    return jlong_ptr(sig);
}

JNIEXPORT jlong JNICALL Java_Mapper_Device_mdev_1get_1output_1by_1name
  (JNIEnv *env, jobject obj, jlong d, jstring name, jobject index)
{
    int i;
    mapper_device dev = (mapper_device)ptr_jlong(d);
    const char *cname = (*env)->GetStringUTFChars(env, name, 0);
    mapper_signal sig = mdev_get_output_by_name(dev, cname, &i);
    (*env)->ReleaseStringUTFChars(env, name, cname);

    if (sig && index) {
        jclass cls = (*env)->GetObjectClass(env, index);
        if (cls) {
            jfieldID val = (*env)->GetFieldID(env, cls, "value", "I");
            if (val)
                (*env)->SetIntField(env, index, val, i);
        }
    }

    return jlong_ptr(sig);
}

JNIEXPORT jlong JNICALL Java_Mapper_Device_mdev_1get_1input_1by_1index
  (JNIEnv *env, jobject obj, jlong d, jint index)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    mapper_signal sig = mdev_get_input_by_index(dev, index);
    return jlong_ptr(sig);
}

JNIEXPORT jlong JNICALL Java_Mapper_Device_mdev_1get_1output_1by_1index
  (JNIEnv *env, jobject obj, jlong d, jint index)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    mapper_signal sig = mdev_get_output_by_index(dev, index);
    return jlong_ptr(sig);
}

JNIEXPORT void JNICALL Java_Mapper_Device_mdev_1set_1property
  (JNIEnv *env, jobject obj, jlong d, jstring key, jobject value)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    const char *ckey = (*env)->GetStringUTFChars(env, key, 0);
    jclass cls = (*env)->GetObjectClass(env, value);
    if (cls) {
        jfieldID typeid = (*env)->GetFieldID(env, cls, "type", "C");
        if (typeid) {
            char type;
            lo_arg a, *pa=&a;
            jfieldID valf = 0;
            jobject o;
            type = (*env)->GetCharField(env, value, typeid);
            switch (type)
            {
            case 'i':
                valf = (*env)->GetFieldID(env, cls, "_i", "I");
                a.i = (*env)->GetIntField(env, value, valf);
                break;
            case 'f':
                valf = (*env)->GetFieldID(env, cls, "_f", "F");
                a.f = (*env)->GetFloatField(env, value, valf);
                break;
            case 'd':
                valf = (*env)->GetFieldID(env, cls, "_d", "D");
                a.d = (*env)->GetDoubleField(env, value, valf);
                break;
            case 's':
            case 'S':
                valf = (*env)->GetFieldID(env, cls, "_s", "Ljava/lang/String;");
                o = (*env)->GetObjectField(env, value, valf);
                pa = (lo_arg*)(*env)->GetStringUTFChars(env, o, 0);
                break;
            }
            if (valf) {
                mdev_set_property(dev, ckey, type, pa);
                if (pa != &a)
                    (*env)->ReleaseStringUTFChars(env, o, (const char*)pa);
            }
        }
    }
    (*env)->ReleaseStringUTFChars(env, key, ckey);
}

JNIEXPORT void JNICALL Java_Mapper_Device_mdev_1remove_1property
  (JNIEnv *env, jobject obj, jlong d, jstring key)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    const char *ckey = (*env)->GetStringUTFChars(env, key, 0);
    mdev_remove_property(dev, ckey);
    (*env)->ReleaseStringUTFChars(env, key, ckey);
}

JNIEXPORT jboolean JNICALL Java_Mapper_Device_mdev_1ready
  (JNIEnv *env, jobject obj, jlong d)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    return mdev_ready(dev);
}

JNIEXPORT jstring JNICALL Java_Mapper_Device_mdev_1name
  (JNIEnv *env, jobject obj, jlong d)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    const char *n = mdev_name(dev);
    if (n)
        return (*env)->NewStringUTF(env, n);
    return 0;
}

JNIEXPORT jint JNICALL Java_Mapper_Device_mdev_1port
  (JNIEnv *env, jobject obj, jlong d)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    return mdev_port(dev);
}

JNIEXPORT jstring JNICALL Java_Mapper_Device_mdev_1ip4
  (JNIEnv *env, jobject obj, jlong d)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    const struct in_addr* a = mdev_ip4(dev);
    if (a)
        return (*env)->NewStringUTF(env, inet_ntoa(*a));
    return 0;
}

JNIEXPORT jstring JNICALL Java_Mapper_Device_mdev_1interface
  (JNIEnv *env, jobject obj, jlong d)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    const char *iface = mdev_interface(dev);
    if (iface)
        return (*env)->NewStringUTF(env, iface);
    return 0;
}

JNIEXPORT jint JNICALL Java_Mapper_Device_mdev_1ordinal
  (JNIEnv *env, jobject obj, jlong d)
{
    mapper_device dev = (mapper_device)ptr_jlong(d);
    return mdev_ordinal(dev);
}

/**** Mapper.Device.Signal ****/

JNIEXPORT jstring JNICALL Java_Mapper_Device_00024Signal_msig_1full_1name
  (JNIEnv *env, jobject obj, jlong s)
{
    mapper_signal sig=(mapper_signal)ptr_jlong(s);
    char str[1024];

    if (sig) {
        msig_full_name(sig, str, 1024);
        return (*env)->NewStringUTF(env, str);
    }
    else
        return 0;
}

JNIEXPORT jstring JNICALL Java_Mapper_Device_00024Signal_msig_1name
  (JNIEnv *env, jobject obj, jlong s)
{
    mapper_signal sig=(mapper_signal)ptr_jlong(s);

    if (sig) {
        mapper_db_signal p = msig_properties(sig);
        return (*env)->NewStringUTF(env, p->name);
    }
    else
        return 0;
}

JNIEXPORT jboolean JNICALL Java_Mapper_Device_00024Signal_msig_1is_1output
  (JNIEnv *env, jobject obj, jlong s)
{
    mapper_signal sig=(mapper_signal)ptr_jlong(s);
    if (sig) {
        mapper_db_signal p = msig_properties(sig);
        return p->is_output;
    }
    return 0;
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_msig_1set_1minimum
  (JNIEnv *env, jobject obj, jlong s, jobject minimum)
{
    mapper_signal sig=(mapper_signal)ptr_jlong(s);
    if (sig) {
        mapper_db_signal p = msig_properties(sig);

        union {
            float f;
            int i;
        } mn;

        if (minimum) {
            jclass cls = (*env)->GetObjectClass(env, minimum);
            if (cls) {
                jfieldID val = (*env)->GetFieldID(env, cls, "value", "D");
                if (val) {
                    if (p->type == 'f')
                        mn.f = (float)(*env)->GetDoubleField(env, minimum, val);
                    else if (p->type == 'i')
                        mn.i = (int)(*env)->GetDoubleField(env, minimum, val);
                    msig_set_minimum(sig, &mn);
                }
            }
        }
        else
            msig_set_minimum(sig, 0);
    }
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_msig_1set_1maximum
  (JNIEnv *env, jobject obj, jlong s, jobject maximum)
{
    mapper_signal sig=(mapper_signal)ptr_jlong(s);
    if (sig) {
        mapper_db_signal p = msig_properties(sig);

        union {
            float f;
            int i;
        } mx;

        if (maximum) {
            jclass cls = (*env)->GetObjectClass(env, maximum);
            if (cls) {
                jfieldID val = (*env)->GetFieldID(env, cls, "value", "D");
                if (val) {
                    if (p->type == 'f')
                        mx.f = (float)(*env)->GetDoubleField(env, maximum, val);
                    else if (p->type == 'i')
                        mx.i = (int)(*env)->GetDoubleField(env, maximum, val);
                    msig_set_minimum(sig, &mx);
                }
            }
        }
        else
            msig_set_maximum(sig, 0);
    }
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_msig_1set_1property
  (JNIEnv *env, jobject obj, jlong s, jstring key, jobject value)
{
    mapper_signal sig = (mapper_signal)ptr_jlong(s);
    const char *ckey = (*env)->GetStringUTFChars(env, key, 0);
    jclass cls = (*env)->GetObjectClass(env, value);
    if (cls) {
        jfieldID typeid = (*env)->GetFieldID(env, cls, "type", "C");
        if (typeid) {
            char type;
            lo_arg a, *pa=&a;
            jfieldID valf = 0;
            jobject o;
            type = (*env)->GetCharField(env, value, typeid);
            switch (type)
            {
            case 'i':
                valf = (*env)->GetFieldID(env, cls, "_i", "I");
                a.i = (*env)->GetIntField(env, value, valf);
                break;
            case 'f':
                valf = (*env)->GetFieldID(env, cls, "_f", "F");
                a.f = (*env)->GetFloatField(env, value, valf);
                break;
            case 'd':
                valf = (*env)->GetFieldID(env, cls, "_d", "D");
                a.d = (*env)->GetDoubleField(env, value, valf);
                break;
            case 's':
            case 'S':
                valf = (*env)->GetFieldID(env, cls, "_s", "Ljava/lang/String;");
                o = (*env)->GetObjectField(env, value, valf);
                pa = (lo_arg*)(*env)->GetStringUTFChars(env, o, 0);
                break;
            }
            if (valf) {
                msig_set_property(sig, ckey, type, pa);
                if (pa != &a)
                    (*env)->ReleaseStringUTFChars(env, o, (const char*)pa);
            }
        }
    }
    (*env)->ReleaseStringUTFChars(env, key, ckey);
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_msig_1remove_1property
  (JNIEnv *env, jobject obj, jlong s, jstring key)
{
    mapper_signal sig = (mapper_signal)ptr_jlong(s);
    const char *ckey = (*env)->GetStringUTFChars(env, key, 0);
    msig_remove_property(sig, ckey);
    (*env)->ReleaseStringUTFChars(env, key, ckey);
}

JNIEXPORT jint JNICALL Java_Mapper_Device_00024Signal_msig_1query_1remote
  (JNIEnv *env, jobject obj, jlong s, jlong r)
{
    mapper_signal sig  = (mapper_signal)ptr_jlong(s);
    mapper_signal recv = (mapper_signal)ptr_jlong(r);
    return msig_query_remote(sig, recv);
}

static mapper_signal get_signal_from_jobject(JNIEnv *env, jobject obj)
{
    // TODO check device here
    jclass cls = (*env)->GetObjectClass(env, obj);
    if (cls) {
        jfieldID val = (*env)->GetFieldID(env, cls, "_signal", "J");
        if (val) {
            jlong s = (*env)->GetLongField(env, obj, val);
            return (mapper_signal)ptr_jlong(s);
        }
    }
    return 0;
}

static void throwIllegalArgumentTruncate(JNIEnv *env, mapper_signal sig)
{
    jclass newExcCls =
        (*env)->FindClass(env, "java/lang/IllegalArgumentException");
    if (newExcCls) {
        char msg[1024];
        snprintf(msg, 1024,
                 "Signal %s has integer type, floating-"
                 "point value would truncate.",
                 msig_properties(sig)->name);
        (*env)->ThrowNew(env, newExcCls, msg);
    }
}

static void throwIllegalArgumentLength(JNIEnv *env, mapper_signal sig, int al)
{
    jclass newExcCls =
        (*env)->FindClass(env, "java/lang/IllegalArgumentException");
    if (newExcCls) {
        char msg[1024];
        mapper_db_signal p = msig_properties(sig);
        snprintf(msg, 1024,
                 "Signal %s length is %d, but array argument has length %d.",
                 p->name, p->length, al);
        (*env)->ThrowNew(env, newExcCls, msg);
    }
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_update__I
  (JNIEnv *env, jobject obj, jint value)
{
    mapper_signal sig = get_signal_from_jobject(env, obj);
    if (!sig) return;

    mapper_db_signal props = msig_properties(sig);
    if (props->length != 1) {
        throwIllegalArgumentLength(env, sig, 1);
        return;
    }
    if (props->type == 'i')
        msig_update_int(sig, value);
    else if (props->type == 'f')
        msig_update_float(sig, (float)value);
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_update__F
  (JNIEnv *env, jobject obj, jfloat value)
{
    mapper_signal sig = get_signal_from_jobject(env, obj);
    if (!sig) return;

    mapper_db_signal props = msig_properties(sig);
    if (props->length != 1) {
        throwIllegalArgumentLength(env, sig, 1);
        return;
    }
    if (props->type == 'i')
        throwIllegalArgumentTruncate(env, sig);
    else if (props->type == 'f')
        msig_update_float(sig, value);
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_update__D
  (JNIEnv *env, jobject obj, jdouble value)
{
    mapper_signal sig = get_signal_from_jobject(env, obj);
    if (!sig) return;

    mapper_db_signal props = msig_properties(sig);
    if (props->length != 1) {
        throwIllegalArgumentLength(env, sig, 1);
        return;
    }
    if (props->type == 'i')
        throwIllegalArgumentTruncate(env, sig);
    else if (props->type == 'f')
        msig_update_float(sig, (float)value);
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_update___3I
  (JNIEnv *env, jobject obj, jintArray value)
{
    mapper_signal sig = get_signal_from_jobject(env, obj);
    if (!sig) return;

    mapper_db_signal props = msig_properties(sig);
    int length = (*env)->GetArrayLength(env, value);
    if (length != props->length) {
        throwIllegalArgumentLength(env, sig, length);
        return;
    }
    jint *array = (*env)->GetIntArrayElements(env, value, 0);
    if (array) {
        if (props->type == 'i') {
            msig_update(sig, array);
        }
        else if (props->type == 'f') {
            float *arraycopy = malloc(sizeof(float)*length);
            int i;
            for (i=0; i<length; i++)
                arraycopy[i] = (float)array[i];
            msig_update(sig, arraycopy);
            free(arraycopy);
        }
        (*env)->ReleaseIntArrayElements(env, value, array, JNI_ABORT);
    }
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_update___3F
  (JNIEnv *env, jobject obj, jfloatArray value)
{
    mapper_signal sig = get_signal_from_jobject(env, obj);
    if (!sig) return;

    mapper_db_signal props = msig_properties(sig);
    int length = (*env)->GetArrayLength(env, value);
    if (length != props->length) {
        throwIllegalArgumentLength(env, sig, length);
        return;
    }
    if (props->type != 'f') {
        throwIllegalArgumentTruncate(env, sig);
        return;
    }
    jfloat *array = (*env)->GetFloatArrayElements(env, value, 0);
    if (array) {
        msig_update(sig, array);
        (*env)->ReleaseFloatArrayElements(env, value, array, JNI_ABORT);
    }
}

JNIEXPORT void JNICALL Java_Mapper_Device_00024Signal_update___3D
  (JNIEnv *env, jobject obj, jdoubleArray value)
{
    mapper_signal sig = get_signal_from_jobject(env, obj);
    if (!sig) return;

    mapper_db_signal props = msig_properties(sig);
    int length = (*env)->GetArrayLength(env, value);
    if (length != props->length) {
        throwIllegalArgumentLength(env, sig, length);
        return;
    }
    if (props->type != 'f') {
        throwIllegalArgumentTruncate(env, sig);
        return;
    }
    jdouble *array = (*env)->GetDoubleArrayElements(env, value, 0);
    if (array) {
        float *arraycopy = malloc(sizeof(float)*length);
        int i;
        for (i=0; i<length; i++)
            arraycopy[i] = (float)array[i];
        (*env)->ReleaseDoubleArrayElements(env, value, array, JNI_ABORT);
        msig_update(sig, arraycopy);
        free(arraycopy);
    }
}
