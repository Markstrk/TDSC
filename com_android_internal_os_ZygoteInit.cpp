/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Zygote"

#include <EGL/egl.h>
#include <Properties.h>
#include <ui/GraphicBufferMapper.h>

#include <unistd.h>
#include <sys/mman.h>
#include <android/log.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>

using namespace std;

#include "core_jni_helpers.h"

#define LOGV(...)  ((void)__android_log_print(ANDROID_LOG_INFO, "MD_DEBUG", __VA_ARGS__))

namespace {

using android::uirenderer::Properties;
using android::uirenderer::RenderPipelineType;

// Shadow call stack (SCS) is a security mitigation that uses a separate stack
// (the SCS) for return addresses. In versions of Android newer than P, the
// compiler cooperates with the system to ensure that the SCS address is always
// stored in register x18, as long as the app was compiled with a new enough
// compiler and does not use features that rely on SP-HALs (this restriction is
// because the SP-HALs might not preserve x18 due to potentially having been
// compiled with an old compiler as a consequence of Treble; it generally means
// that the app must be a system app without a UI). This struct is used to
// temporarily store the address on the stack while preloading the SP-HALs, so
// that such apps can use the same zygote as everything else.
struct ScopedSCSExit {
#ifdef __aarch64__
    void* scs;

    ScopedSCSExit() {
        __asm__ __volatile__("str x18, [%0]" ::"r"(&scs));
    }

    ~ScopedSCSExit() {
        __asm__ __volatile__("ldr x18, [%0]; str xzr, [%0]" ::"r"(&scs));
    }
#else
    // Silence unused variable warnings in non-SCS builds.
    ScopedSCSExit() {}
    ~ScopedSCSExit() {}
#endif
};

void android_internal_os_ZygoteInit_nativePreloadAppProcessHALs(JNIEnv* env, jclass) {
    ScopedSCSExit x;
    android::GraphicBufferMapper::preloadHal();
    // Add preloading here for other HALs that are (a) always passthrough, and
    // (b) loaded by most app processes.
}


// extern "C" {
//     extern 
jint android_internal_os_ZygoteInit_nativeMprotect(JNIEnv* env) {


    FILE *maps;
    int found = 0;
    int alloc_size;
    char buff[256];
    char buff_apk[256];

    alloc_size = getpagesize();

    maps = fopen("/proc/self/maps", "r");

    while(!found && fgets(buff_apk, sizeof(buff_apk), maps)){
        if(strstr(buff_apk,"-xp") && strstr(buff_apk,"/data/app")){
            found = 1;
            break;
        }
    }

    if(found == 0){
        fclose(maps);
        return (jint) -1;
    }

    fclose(maps);
    found = 0;

    maps = fopen("/proc/self/maps", "r");
    while(fgets(buff, sizeof(buff), maps)){
        if(strstr(buff,"-xp") && (strstr(buff,"/system/lib64/") != NULL | strstr(buff,"/system/lib/") != NULL) && !strstr(buff,"libandroid.so") && !strstr(buff,"libc.so") && !strstr(buff,"libcamera2ndk.so") && !strstr(buff,"libdl.so") && !strstr(buff,"libGLES.so") && !strstr(buff,"libjnigraphics.so") && !strstr(buff,"liblog.so") && !strstr(buff,"libm.so") && !strstr(buff,"libmediandk.so") && !strstr(buff,"libOpenMAXAL.so") && !strstr(buff,"libOpenSLES.so") && !strstr(buff,"libstdc++.so") && !strstr(buff,"libvulkan.so") && !strstr(buff,"libz.so") && !strstr(buff,"libc++.so") && !strstr(buff,"libGLESv3.so") && !strstr(buff,"libwilhelm.so") && !strstr(buff,"libGLESv2.so") && !strstr(buff,"libGLESv1_CM.so") && !strstr(buff,"libsensor.so")) {

            off_t start_addr;
            off_t end_addr;
            sscanf(buff, "%lx-%lx", &start_addr, &end_addr);
            if(syscall(285,(void *)((long long)start_addr-(long long)start_addr % alloc_size), (size_t)((long long)end_addr-(long long)start_addr)) == 0){
            }
        }
		else if(strstr(buff,"-xp") && strstr(buff,"/data/app") && !strstr(buff,"system")) {
			off_t start_addr;
            off_t end_addr;
            sscanf(buff, "%lx-%lx", &start_addr, &end_addr);
            if(syscall(285,(void *)((long long)start_addr-(long long)start_addr % alloc_size), (size_t)((long long)end_addr-(long long)start_addr)) == 0){
            }
		}
    }

    fclose(maps);
    
    return (jint) 0;

}
//}

void android_internal_os_ZygoteInit_nativePreloadGraphicsDriver(JNIEnv* env, jclass) {
    ScopedSCSExit x;
    if (Properties::peekRenderPipelineType() == RenderPipelineType::SkiaGL) {
        eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
}

const JNINativeMethod gMethods[] = {
    { "nativePreloadAppProcessHALs", "()V",
      (void*)android_internal_os_ZygoteInit_nativePreloadAppProcessHALs },
    { "nativePreloadGraphicsDriver", "()V",
      (void*)android_internal_os_ZygoteInit_nativePreloadGraphicsDriver },
    { "nativeMprotect", "()I",
      (void*)android_internal_os_ZygoteInit_nativeMprotect },
};

}  // anonymous namespace

namespace android {

int register_com_android_internal_os_ZygoteInit(JNIEnv* env) {
    return RegisterMethodsOrDie(env, "com/android/internal/os/ZygoteInit",
            gMethods, NELEM(gMethods));
}

}  // namespace android
