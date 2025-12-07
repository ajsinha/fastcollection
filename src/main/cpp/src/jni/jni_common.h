/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 * 
 * @file jni_common.h
 * @brief Common JNI utilities for FastCollection bindings
 */

#ifndef FASTCOLLECTION_JNI_COMMON_H
#define FASTCOLLECTION_JNI_COMMON_H

#include <jni.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

namespace fastcollection {
namespace jni {

// Exception class name
constexpr const char* EXCEPTION_CLASS = "com/kuber/fastcollection/FastCollectionException";

/**
 * @brief Throw a Java exception from native code
 */
inline void throwException(JNIEnv* env, const char* message) {
    jclass exClass = env->FindClass(EXCEPTION_CLASS);
    if (exClass != nullptr) {
        env->ThrowNew(exClass, message);
    } else {
        // Fallback to RuntimeException
        jclass runtimeEx = env->FindClass("java/lang/RuntimeException");
        env->ThrowNew(runtimeEx, message);
    }
}

/**
 * @brief Convert Java string to C++ string
 */
inline std::string jstringToString(JNIEnv* env, jstring jstr) {
    if (jstr == nullptr) return "";
    
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

/**
 * @brief Convert Java byte array to native byte vector
 */
inline std::vector<uint8_t> jbyteArrayToVector(JNIEnv* env, jbyteArray jarray) {
    if (jarray == nullptr) return std::vector<uint8_t>();
    
    jsize length = env->GetArrayLength(jarray);
    std::vector<uint8_t> result(length);
    
    jbyte* elements = env->GetByteArrayElements(jarray, nullptr);
    std::memcpy(result.data(), elements, length);
    env->ReleaseByteArrayElements(jarray, elements, JNI_ABORT);
    
    return result;
}

/**
 * @brief Convert native byte vector to Java byte array
 */
inline jbyteArray vectorToJbyteArray(JNIEnv* env, const std::vector<uint8_t>& vec) {
    jbyteArray result = env->NewByteArray(static_cast<jsize>(vec.size()));
    if (result == nullptr) return nullptr;
    
    env->SetByteArrayRegion(result, 0, static_cast<jsize>(vec.size()), 
                            reinterpret_cast<const jbyte*>(vec.data()));
    return result;
}

/**
 * @brief Get native byte array data without copying
 */
inline const uint8_t* getByteArrayData(JNIEnv* env, jbyteArray jarray, jint* length_out) {
    if (jarray == nullptr) {
        *length_out = 0;
        return nullptr;
    }
    *length_out = env->GetArrayLength(jarray);
    return reinterpret_cast<const uint8_t*>(env->GetByteArrayElements(jarray, nullptr));
}

/**
 * @brief Release byte array data
 */
inline void releaseByteArrayData(JNIEnv* env, jbyteArray jarray, const uint8_t* data) {
    if (data != nullptr && jarray != nullptr) {
        env->ReleaseByteArrayElements(jarray, const_cast<jbyte*>(reinterpret_cast<const jbyte*>(data)), JNI_ABORT);
    }
}

/**
 * @brief Get handle from Java object's nativeHandle field
 */
template<typename T>
T* getHandle(JNIEnv* env, jobject obj) {
    jclass clazz = env->GetObjectClass(obj);
    jfieldID handleField = env->GetFieldID(clazz, "nativeHandle", "J");
    jlong handle = env->GetLongField(obj, handleField);
    return reinterpret_cast<T*>(handle);
}

/**
 * @brief Set handle in Java object's nativeHandle field
 */
template<typename T>
void setHandle(JNIEnv* env, jobject obj, T* handle) {
    jclass clazz = env->GetObjectClass(obj);
    jfieldID handleField = env->GetFieldID(clazz, "nativeHandle", "J");
    env->SetLongField(obj, handleField, reinterpret_cast<jlong>(handle));
}

/**
 * @brief RAII helper for JNI byte array access
 */
class JByteArrayAccess {
public:
    JByteArrayAccess(JNIEnv* env, jbyteArray array) 
        : env_(env), array_(array), data_(nullptr), length_(0) {
        if (array_ != nullptr) {
            length_ = env_->GetArrayLength(array_);
            data_ = reinterpret_cast<uint8_t*>(env_->GetByteArrayElements(array_, nullptr));
        }
    }
    
    ~JByteArrayAccess() {
        if (data_ != nullptr && array_ != nullptr) {
            env_->ReleaseByteArrayElements(array_, reinterpret_cast<jbyte*>(data_), JNI_ABORT);
        }
    }
    
    const uint8_t* data() const { return data_; }
    jsize length() const { return length_; }
    bool valid() const { return data_ != nullptr; }
    
private:
    JNIEnv* env_;
    jbyteArray array_;
    uint8_t* data_;
    jsize length_;
};

} // namespace jni
} // namespace fastcollection

#endif // FASTCOLLECTION_JNI_COMMON_H
