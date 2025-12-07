/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 * 
 * @file jni_list.cpp
 * @brief JNI bindings for FastList with TTL support
 */

#include <jni.h>
#include "jni_common.h"
#include "fc_list.h"

using namespace fastcollection;
using namespace fastcollection::jni;

extern "C" {

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeCreate
 * Signature: (Ljava/lang/String;JZ)J
 */
JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeCreate
  (JNIEnv* env, jclass clazz, jstring filePath, jlong initialSize, jboolean createNew) {
    try {
        std::string path = jstringToString(env, filePath);
        FastList* list = new FastList(path, static_cast<size_t>(initialSize), createNew);
        return reinterpret_cast<jlong>(list);
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return 0;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return 0;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeDestroy
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeDestroy
  (JNIEnv* env, jclass clazz, jlong handle) {
    FastList* list = reinterpret_cast<FastList*>(handle);
    delete list;
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeAdd
 * Signature: (J[BI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeAdd
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data, jint ttlSeconds) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) {
            throwException(env, "Invalid data array");
            return JNI_FALSE;
        }
        
        return list->add(dataAccess.data(), dataAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeAddAt
 * Signature: (JI[BI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeAddAt
  (JNIEnv* env, jobject obj, jlong handle, jint index, jbyteArray data, jint ttlSeconds) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) {
            throwException(env, "Invalid data array");
            return JNI_FALSE;
        }
        
        return list->add(static_cast<size_t>(index), dataAccess.data(), dataAccess.length(), ttlSeconds) 
               ? JNI_TRUE : JNI_FALSE;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeAddFirst
 * Signature: (J[BI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeAddFirst
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data, jint ttlSeconds) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) {
            throwException(env, "Invalid data array");
            return JNI_FALSE;
        }
        
        return list->addFirst(dataAccess.data(), dataAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeGet
 * Signature: (JI)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeGet
  (JNIEnv* env, jobject obj, jlong handle, jint index) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        std::vector<uint8_t> result;
        
        if (list->get(static_cast<size_t>(index), result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeGetFirst
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeGetFirst
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        std::vector<uint8_t> result;
        
        if (list->getFirst(result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeGetLast
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeGetLast
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        std::vector<uint8_t> result;
        
        if (list->getLast(result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeSet
 * Signature: (JI[BI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeSet
  (JNIEnv* env, jobject obj, jlong handle, jint index, jbyteArray data, jint ttlSeconds) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) {
            throwException(env, "Invalid data array");
            return JNI_FALSE;
        }
        
        return list->set(static_cast<size_t>(index), dataAccess.data(), dataAccess.length(), ttlSeconds) 
               ? JNI_TRUE : JNI_FALSE;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeRemove
 * Signature: (JI)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeRemove
  (JNIEnv* env, jobject obj, jlong handle, jint index) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        std::vector<uint8_t> result;
        
        if (list->remove(static_cast<size_t>(index), &result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeRemoveFirst
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeRemoveFirst
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        std::vector<uint8_t> result;
        
        if (list->removeFirst(&result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeRemoveLast
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeRemoveLast
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        std::vector<uint8_t> result;
        
        if (list->removeLast(&result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeContains
 * Signature: (J[B)Z
 */
JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeContains
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return JNI_FALSE;
        
        return list->contains(dataAccess.data(), dataAccess.length()) ? JNI_TRUE : JNI_FALSE;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeIndexOf
 * Signature: (J[B)I
 */
JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeIndexOf
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return -1;
        
        return static_cast<jint>(list->indexOf(dataAccess.data(), dataAccess.length()));
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return -1;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeGetTTL
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeGetTTL
  (JNIEnv* env, jobject obj, jlong handle, jint index) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        return static_cast<jlong>(list->getTTL(static_cast<size_t>(index)));
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return 0;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeSetTTL
 * Signature: (JII)Z
 */
JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeSetTTL
  (JNIEnv* env, jobject obj, jlong handle, jint index, jint ttlSeconds) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        return list->setTTL(static_cast<size_t>(index), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeRemoveExpired
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeRemoveExpired
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        return static_cast<jint>(list->removeExpired());
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return 0;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeClear
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeClear
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        list->clear();
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeSize
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeSize
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        return static_cast<jint>(list->size());
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return 0;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeIsEmpty
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeIsEmpty
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        return list->isEmpty() ? JNI_TRUE : JNI_FALSE;
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
        return JNI_TRUE;
    }
}

/*
 * Class:     com_abhikarta_fastcollection_FastCollectionList
 * Method:    nativeFlush
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionList_nativeFlush
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        list->flush();
    } catch (const FastCollectionException& e) {
        throwException(env, e.what());
    }
}

} // extern "C"
