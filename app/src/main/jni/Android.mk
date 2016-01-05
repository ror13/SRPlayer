# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)
ARCH := $(APP_ABI)


include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg-prebuilt
LOCAL_SRC_FILES := ffmpeg-build/$(TARGET_ARCH_ABI)/libffmpeg-neon.so
LOCAL_EXPORT_C_INCLUDES := ffmpeg-build/$(TARGET_ARCH_ABI)/include
LOCAL_EXPORT_LDLIBS := ffmpeg-build/$(TARGET_ARCH_ABI)/libffmpeg-neon.so
LOCAL_PRELINK_MODULE := true
include $(PREBUILT_SHARED_LIBRARY)



include $(CLEAR_VARS)
LOCAL_ALLOW_UNDEFINED_SYMBOLS=false
LOCAL_MODULE := CFfmpeg
LOCAL_SRC_FILES := CFfmpeg.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/ffmpeg-build/$(TARGET_ARCH_ABI)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/dependencies/include
LOCAL_LDFLAGS := -llog  -landroid  -lz -lm -lutils -lstagefright  -g $(LOCAL_PATH)/ffmpeg-build/$(TARGET_ARCH_ABI)/libffmpeg-neon.so -L$(LOCAL_PATH)/dependencies/lib
#-L$(LOCAL_PATH)/ffmpeg-build/$(TARGET_ARCH_ABI)
LOCAL_SHARED_LIBRARY := ffmpeg-prebuilt
include $(BUILD_SHARED_LIBRARY)




#$(call import-module,cpufeatures)
