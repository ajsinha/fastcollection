/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 * 
 * @file jni_collections.cpp
 * @brief JNI bindings for FastMap, FastSet, FastQueue, and FastStack with TTL support
 */

#include <jni.h>
#include "jni_common.h"
#include "fc_map.h"
#include "fc_set.h"
#include "fc_queue.h"
#include "fc_stack.h"

using namespace fastcollection;
using namespace fastcollection::jni;

extern "C" {

// ============================================================================
// FastCollectionMap JNI Methods
// ============================================================================

JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeCreate
  (JNIEnv* env, jclass clazz, jstring filePath, jlong initialSize, jboolean createNew) {
    try {
        std::string path = jstringToString(env, filePath);
        FastMap* map = new FastMap(path, static_cast<size_t>(initialSize), createNew);
        return reinterpret_cast<jlong>(map);
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeDestroy
  (JNIEnv* env, jclass clazz, jlong handle) {
    delete reinterpret_cast<FastMap*>(handle);
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativePut
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jbyteArray value, jint ttlSeconds) {
    try {
        FastMap* map = reinterpret_cast<FastMap*>(handle);
        JByteArrayAccess keyAccess(env, key);
        JByteArrayAccess valueAccess(env, value);
        
        if (!keyAccess.valid()) return JNI_FALSE;
        
        return map->put(keyAccess.data(), keyAccess.length(),
                       valueAccess.data(), valueAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativePutIfAbsent
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jbyteArray value, jint ttlSeconds) {
    try {
        FastMap* map = reinterpret_cast<FastMap*>(handle);
        JByteArrayAccess keyAccess(env, key);
        JByteArrayAccess valueAccess(env, value);
        
        if (!keyAccess.valid()) return JNI_FALSE;
        
        return map->putIfAbsent(keyAccess.data(), keyAccess.length(),
                               valueAccess.data(), valueAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeGet
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key) {
    try {
        FastMap* map = reinterpret_cast<FastMap*>(handle);
        JByteArrayAccess keyAccess(env, key);
        std::vector<uint8_t> result;
        
        if (!keyAccess.valid()) return nullptr;
        
        if (map->get(keyAccess.data(), keyAccess.length(), result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeRemove
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key) {
    try {
        FastMap* map = reinterpret_cast<FastMap*>(handle);
        JByteArrayAccess keyAccess(env, key);
        
        if (!keyAccess.valid()) return JNI_FALSE;
        
        return map->remove(keyAccess.data(), keyAccess.length()) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeContainsKey
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key) {
    try {
        FastMap* map = reinterpret_cast<FastMap*>(handle);
        JByteArrayAccess keyAccess(env, key);
        
        if (!keyAccess.valid()) return JNI_FALSE;
        
        return map->containsKey(keyAccess.data(), keyAccess.length()) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeGetTTL
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key) {
    try {
        FastMap* map = reinterpret_cast<FastMap*>(handle);
        JByteArrayAccess keyAccess(env, key);
        
        if (!keyAccess.valid()) return 0;
        
        return static_cast<jlong>(map->getTTL(keyAccess.data(), keyAccess.length()));
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return 0;
    }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeSetTTL
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint ttlSeconds) {
    try {
        FastMap* map = reinterpret_cast<FastMap*>(handle);
        JByteArrayAccess keyAccess(env, key);
        
        if (!keyAccess.valid()) return JNI_FALSE;
        
        return map->setTTL(keyAccess.data(), keyAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeRemoveExpired
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastMap* map = reinterpret_cast<FastMap*>(handle);
        return static_cast<jint>(map->removeExpired());
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeClear
  (JNIEnv* env, jobject obj, jlong handle) {
    try { reinterpret_cast<FastMap*>(handle)->clear(); }
    catch (const std::exception& e) { throwException(env, e.what()); }
}

JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeSize
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return static_cast<jint>(reinterpret_cast<FastMap*>(handle)->size()); }
    catch (const std::exception& e) { throwException(env, e.what()); return 0; }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeIsEmpty
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return reinterpret_cast<FastMap*>(handle)->isEmpty() ? JNI_TRUE : JNI_FALSE; }
    catch (const std::exception& e) { throwException(env, e.what()); return JNI_TRUE; }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionMap_nativeFlush
  (JNIEnv* env, jobject obj, jlong handle) {
    try { reinterpret_cast<FastMap*>(handle)->flush(); }
    catch (const std::exception& e) { throwException(env, e.what()); }
}

// ============================================================================
// FastCollectionSet JNI Methods
// ============================================================================

JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeCreate
  (JNIEnv* env, jclass clazz, jstring filePath, jlong initialSize, jboolean createNew) {
    try {
        std::string path = jstringToString(env, filePath);
        FastSet* set = new FastSet(path, static_cast<size_t>(initialSize), createNew);
        return reinterpret_cast<jlong>(set);
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeDestroy
  (JNIEnv* env, jclass clazz, jlong handle) {
    delete reinterpret_cast<FastSet*>(handle);
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeAdd
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data, jint ttlSeconds) {
    try {
        FastSet* set = reinterpret_cast<FastSet*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return JNI_FALSE;
        
        return set->add(dataAccess.data(), dataAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeRemove
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data) {
    try {
        FastSet* set = reinterpret_cast<FastSet*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return JNI_FALSE;
        
        return set->remove(dataAccess.data(), dataAccess.length()) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeContains
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data) {
    try {
        FastSet* set = reinterpret_cast<FastSet*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return JNI_FALSE;
        
        return set->contains(dataAccess.data(), dataAccess.length()) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeGetTTL
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data) {
    try {
        FastSet* set = reinterpret_cast<FastSet*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return 0;
        
        return static_cast<jlong>(set->getTTL(dataAccess.data(), dataAccess.length()));
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return 0;
    }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeSetTTL
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data, jint ttlSeconds) {
    try {
        FastSet* set = reinterpret_cast<FastSet*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return JNI_FALSE;
        
        return set->setTTL(dataAccess.data(), dataAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeRemoveExpired
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return static_cast<jint>(reinterpret_cast<FastSet*>(handle)->removeExpired()); }
    catch (const std::exception& e) { throwException(env, e.what()); return 0; }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeClear
  (JNIEnv* env, jobject obj, jlong handle) {
    try { reinterpret_cast<FastSet*>(handle)->clear(); }
    catch (const std::exception& e) { throwException(env, e.what()); }
}

JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeSize
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return static_cast<jint>(reinterpret_cast<FastSet*>(handle)->size()); }
    catch (const std::exception& e) { throwException(env, e.what()); return 0; }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeIsEmpty
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return reinterpret_cast<FastSet*>(handle)->isEmpty() ? JNI_TRUE : JNI_FALSE; }
    catch (const std::exception& e) { throwException(env, e.what()); return JNI_TRUE; }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionSet_nativeFlush
  (JNIEnv* env, jobject obj, jlong handle) {
    try { reinterpret_cast<FastSet*>(handle)->flush(); }
    catch (const std::exception& e) { throwException(env, e.what()); }
}

// ============================================================================
// FastCollectionQueue JNI Methods
// ============================================================================

JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativeCreate
  (JNIEnv* env, jclass clazz, jstring filePath, jlong initialSize, jboolean createNew) {
    try {
        std::string path = jstringToString(env, filePath);
        FastQueue* queue = new FastQueue(path, static_cast<size_t>(initialSize), createNew);
        return reinterpret_cast<jlong>(queue);
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativeDestroy
  (JNIEnv* env, jclass clazz, jlong handle) {
    delete reinterpret_cast<FastQueue*>(handle);
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativeOffer
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data, jint ttlSeconds) {
    try {
        FastQueue* queue = reinterpret_cast<FastQueue*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return JNI_FALSE;
        
        return queue->offer(dataAccess.data(), dataAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativeOfferFirst
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data, jint ttlSeconds) {
    try {
        FastQueue* queue = reinterpret_cast<FastQueue*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return JNI_FALSE;
        
        return queue->offerFirst(dataAccess.data(), dataAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativePoll
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastQueue* queue = reinterpret_cast<FastQueue*>(handle);
        std::vector<uint8_t> result;
        
        if (queue->poll(result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativePollLast
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastQueue* queue = reinterpret_cast<FastQueue*>(handle);
        std::vector<uint8_t> result;
        
        if (queue->pollLast(result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativePeek
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastQueue* queue = reinterpret_cast<FastQueue*>(handle);
        std::vector<uint8_t> result;
        
        if (queue->peek(result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativePeekTTL
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return static_cast<jlong>(reinterpret_cast<FastQueue*>(handle)->peekTTL()); }
    catch (const std::exception& e) { throwException(env, e.what()); return 0; }
}

JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativeRemoveExpired
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return static_cast<jint>(reinterpret_cast<FastQueue*>(handle)->removeExpired()); }
    catch (const std::exception& e) { throwException(env, e.what()); return 0; }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativeClear
  (JNIEnv* env, jobject obj, jlong handle) {
    try { reinterpret_cast<FastQueue*>(handle)->clear(); }
    catch (const std::exception& e) { throwException(env, e.what()); }
}

JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativeSize
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return static_cast<jint>(reinterpret_cast<FastQueue*>(handle)->size()); }
    catch (const std::exception& e) { throwException(env, e.what()); return 0; }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativeIsEmpty
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return reinterpret_cast<FastQueue*>(handle)->isEmpty() ? JNI_TRUE : JNI_FALSE; }
    catch (const std::exception& e) { throwException(env, e.what()); return JNI_TRUE; }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionQueue_nativeFlush
  (JNIEnv* env, jobject obj, jlong handle) {
    try { reinterpret_cast<FastQueue*>(handle)->flush(); }
    catch (const std::exception& e) { throwException(env, e.what()); }
}

// ============================================================================
// FastCollectionStack JNI Methods
// ============================================================================

JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativeCreate
  (JNIEnv* env, jclass clazz, jstring filePath, jlong initialSize, jboolean createNew) {
    try {
        std::string path = jstringToString(env, filePath);
        FastStack* stack = new FastStack(path, static_cast<size_t>(initialSize), createNew);
        return reinterpret_cast<jlong>(stack);
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativeDestroy
  (JNIEnv* env, jclass clazz, jlong handle) {
    delete reinterpret_cast<FastStack*>(handle);
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativePush
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data, jint ttlSeconds) {
    try {
        FastStack* stack = reinterpret_cast<FastStack*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return JNI_FALSE;
        
        return stack->push(dataAccess.data(), dataAccess.length(), ttlSeconds) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativePop
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastStack* stack = reinterpret_cast<FastStack*>(handle);
        std::vector<uint8_t> result;
        
        if (stack->pop(result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

JNIEXPORT jbyteArray JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativePeek
  (JNIEnv* env, jobject obj, jlong handle) {
    try {
        FastStack* stack = reinterpret_cast<FastStack*>(handle);
        std::vector<uint8_t> result;
        
        if (stack->peek(result)) {
            return vectorToJbyteArray(env, result);
        }
        return nullptr;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return nullptr;
    }
}

JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativePeekTTL
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return static_cast<jlong>(reinterpret_cast<FastStack*>(handle)->peekTTL()); }
    catch (const std::exception& e) { throwException(env, e.what()); return 0; }
}

JNIEXPORT jlong JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativeSearch
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data) {
    try {
        FastStack* stack = reinterpret_cast<FastStack*>(handle);
        JByteArrayAccess dataAccess(env, data);
        
        if (!dataAccess.valid()) return -1;
        
        return static_cast<jlong>(stack->search(dataAccess.data(), dataAccess.length()));
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return -1;
    }
}

JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativeRemoveExpired
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return static_cast<jint>(reinterpret_cast<FastStack*>(handle)->removeExpired()); }
    catch (const std::exception& e) { throwException(env, e.what()); return 0; }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativeClear
  (JNIEnv* env, jobject obj, jlong handle) {
    try { reinterpret_cast<FastStack*>(handle)->clear(); }
    catch (const std::exception& e) { throwException(env, e.what()); }
}

JNIEXPORT jint JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativeSize
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return static_cast<jint>(reinterpret_cast<FastStack*>(handle)->size()); }
    catch (const std::exception& e) { throwException(env, e.what()); return 0; }
}

JNIEXPORT jboolean JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativeIsEmpty
  (JNIEnv* env, jobject obj, jlong handle) {
    try { return reinterpret_cast<FastStack*>(handle)->isEmpty() ? JNI_TRUE : JNI_FALSE; }
    catch (const std::exception& e) { throwException(env, e.what()); return JNI_TRUE; }
}

JNIEXPORT void JNICALL Java_com_abhikarta_fastcollection_FastCollectionStack_nativeFlush
  (JNIEnv* env, jobject obj, jlong handle) {
    try { reinterpret_cast<FastStack*>(handle)->flush(); }
    catch (const std::exception& e) { throwException(env, e.what()); }
}

} // extern "C"
