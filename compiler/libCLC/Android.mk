##############################################################################
#
#    Copyright (c) 2005 - 2018 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################


LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../../Android.mk.def

GC_CLC_DIR  := .

ifdef FIXED_ARCH_TYPE

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(FIXED_ARCH_TYPE)/libCLC.so

LOCAL_MODULE         := libCLC
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
include $(BUILD_PREBUILT)

include $(AQROOT)/copy_installed_module.mk

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(FIXED_ARCH_TYPE)/libLLVM_viv.so

LOCAL_MODULE         := libLLVM_viv
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
include $(BUILD_PREBUILT)

include $(AQROOT)/copy_installed_module.mk

else
#
# libclCommon.a
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    $(GC_CLC_DIR)/common/gc_cl_common.c

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-D_GNU_SOURCE \
	-D__STDC_LIMIT_MACROS \
	-D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES := \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	$(AQROOT)/compiler/libVSC/include \
	$(LOCAL_PATH)/$(GC_CLC_DIR)/inc

LOCAL_MODULE         := libclCommon
LOCAL_MODULE_TAGS    := optional
include $(BUILD_STATIC_LIBRARY)

#
# libclCompiler.a
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    $(GC_CLC_DIR)/compiler/gc_cl_scanner.c \
    $(GC_CLC_DIR)/compiler/gc_cl_compiler.c \
    $(GC_CLC_DIR)/compiler/gc_cl_ir.c \
    $(GC_CLC_DIR)/compiler/gc_cl_gen_code.c \
    $(GC_CLC_DIR)/compiler/gc_cl_parser_misc.c \
    $(GC_CLC_DIR)/compiler/gc_cl_emit_code.c \
    $(GC_CLC_DIR)/compiler/gc_cl_parser.c \
    $(GC_CLC_DIR)/compiler/gc_cl_built_ins.c \
    $(GC_CLC_DIR)/compiler/gc_cl_scanner_misc.c \
    $(GC_CLC_DIR)/compiler/gc_cl_tune.c

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-DCL_USE_DEPRECATED_OPENCL_1_0_APIS \
	-DCL_USE_DEPRECATED_OPENCL_1_1_APIS \
	-D_GNU_SOURCE \
	-D__STDC_LIMIT_MACROS \
	-D__STDC_CONSTANT_MACROS \
	-Wno-macro-redefined \
	-Wno-enum-conversion

LOCAL_C_INCLUDES := \
	bionic \
	$(AQROOT)/sdk/inc \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	$(AQROOT)/compiler/libVSC/include \
	$(LOCAL_PATH)/$(GC_CLC_DIR)/inc

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) "<" 23),1)
LOCAL_C_INCLUDES += \
	external/stlport/stlport
endif

LOCAL_MODULE         := libclCompiler
LOCAL_MODULE_TAGS    := optional
include $(BUILD_STATIC_LIBRARY)

#
# libclPreprocessor.a
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    $(GC_CLC_DIR)/preprocessor/gc_cl_input_stream.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_base.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_preprocessor.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_expression.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_macro_expand.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_syntax_util.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_api.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_token.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_syntax.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_hide_set.c \
    $(GC_CLC_DIR)/preprocessor/gc_cl_macro_manager.c

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-D_GNU_SOURCE \
	-D__STDC_LIMIT_MACROS \
	-D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES := \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	bionic \
	$(AQROOT)/compiler/libVSC/include \
	$(LOCAL_PATH)/$(GC_CLC_DIR)/inc

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) "<" 23),1)
LOCAL_C_INCLUDES += \
	external/stlport/stlport
endif

LOCAL_MODULE         := libclPreprocessor
LOCAL_MODULE_TAGS    := optional
include $(BUILD_STATIC_LIBRARY)

#
# libLLVM_viv
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    $(GC_CLC_DIR)/llvm/lib/Support/Debug.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/IsNAN.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/DAGDeltaAlgorithm.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/IsInf.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/TargetRegistry.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/ConstantRange.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/Twine.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/Dwarf.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/Statistic.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/Triple.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/FoldingSet.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/StringMap.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/MemoryObject.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/StringRef.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/FormattedStream.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/MemoryBuffer.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/circular_raw_ostream.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/SystemUtils.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/ErrorHandling.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/APFloat.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/FileUtilities.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/GraphWriter.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/StringPool.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/DeltaAlgorithm.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/SmallPtrSet.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/ManagedStatic.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/Allocator.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/CommandLine.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/APInt.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/PluginLoader.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/raw_ostream.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/Timer.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/SmallVector.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/raw_os_ostream.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/SourceMgr.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/APSInt.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/Regex.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/CrashRecoveryContext.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/StringExtras.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/PrettyStackTrace.cpp \
    $(GC_CLC_DIR)/llvm/lib/Support/regexec.c \
    $(GC_CLC_DIR)/llvm/lib/Support/regstrlcpy.c \
    $(GC_CLC_DIR)/llvm/lib/Support/regfree.c \
    $(GC_CLC_DIR)/llvm/lib/Support/regcomp.c \
    $(GC_CLC_DIR)/llvm/lib/Support/regerror.c \
    $(GC_CLC_DIR)/llvm/lib/VMCore/LLVMContext.cpp \
    $(GC_CLC_DIR)/llvm/lib/VMCore/LLVMContextImpl.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Memory.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/TimeValue.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/DynamicLibrary.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Threading.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/ThreadLocal.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Program.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Host.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/RWMutex.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Errno.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Process.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Path.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Mutex.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Alarm.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/IncludeFile.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Signals.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Disassembler.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/SearchForAddressOfSpecialSymbol.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Valgrind.cpp \
    $(GC_CLC_DIR)/llvm/lib/System/Atomic.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/ToolChain.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/ArgList.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Option.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/OptTable.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Tool.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Arg.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Action.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/CC1Options.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/CC1AsOptions.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Compilation.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Driver.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/DriverOptions.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Job.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Phases.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/ToolChains.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Tools.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/HostInfo.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Driver/Types.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/Version.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/Builtins.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/IdentifierTable.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/SourceLocation.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/SourceManager.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/TokenKinds.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/Targets.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/FileManager.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/TargetInfo.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Basic/Diagnostic.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/FrontendActions.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/PrintPreprocessedOutput.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/TextDiagnosticBuffer.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/CompilerInstance.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/FrontendOptions.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/CacheTokens.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/FrontendAction.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/CompilerInvocation.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/Warnings.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/TextDiagnosticPrinter.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/VerifyDiagnosticsClient.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/InitHeaderSearch.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/LangStandards.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Frontend/InitPreprocessor.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/Lexer.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/PreprocessorLexer.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/PPDirectives.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/LiteralSupport.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/Preprocessor.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/PPMacroExpansion.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/TokenConcatenation.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/HeaderSearch.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/Pragma.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/HeaderMap.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/MacroInfo.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/PTHLexer.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/ScratchBuffer.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/PPExpressions.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/PreprocessingRecord.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/TokenLexer.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/PPCaching.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/MacroArgs.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/Lex/PPLexerChange.cpp \
    $(GC_CLC_DIR)/llvm/tools/clang/lib/FrontendTool/ExecuteCompilerInvocation.cpp

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-D_GNU_SOURCE \
	-D__STDC_LIMIT_MACROS \
	-D__STDC_CONSTANT_MACROS \
	-Wno-writable-strings

LOCAL_CFLAGS += \
	-fPIC -w

LOCAL_C_INCLUDES := \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	bionic \
	$(AQROOT)/compiler/libVSC/include \
	$(AQROOT)/compiler/libCLC/compiler \
	$(LOCAL_PATH)/$(GC_CLC_DIR)/inc \
    $(LOCAL_PATH)/$(GC_CLC_DIR)/llvm/include \
    $(LOCAL_PATH)/$(GC_CLC_DIR)/llvm/include/llvm \
    $(LOCAL_PATH)/$(GC_CLC_DIR)/llvm/lib/System/Unix \
    $(LOCAL_PATH)/$(GC_CLC_DIR)/llvm/tools/clang/include

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) "<" 23),1)
LOCAL_C_INCLUDES += \
	external/stlport/stlport
endif

LOCAL_LDFLAGS := \
	-Wl,-z,defs \
	-Wl,--version-script=$(LOCAL_PATH)/$(GC_CLC_DIR)/llvm/libLLVM.map

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libdl \
	libVSC \
	libGAL

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) "<" 23),1)
LOCAL_SHARED_LIBRARIES += \
	libstlport
endif

LOCAL_STATIC_LIBRARIES := \
	libclCompiler \
	libclPreprocessor \
	libclCommon

LOCAL_MODULE         := libLLVM_viv
LOCAL_MODULE_TAGS    := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

include $(AQROOT)/copy_installed_module.mk

#
# libCLC
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    $(GC_CLC_DIR)/entry/gc_cl_entry.c

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-D_GNU_SOURCE \
	-D__STDC_LIMIT_MACROS \
	-D__STDC_CONSTANT_MACROS

LOCAL_CFLAGS += \
	-fPIC -w

LOCAL_C_INCLUDES := \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	$(AQROOT)/compiler/libVSC/include \
	$(LOCAL_PATH)/$(GC_CLC_DIR)/inc

LOCAL_LDFLAGS := \
	-Wl,-z,defs \
	-Wl,--version-script=$(LOCAL_PATH)/$(GC_CLC_DIR)/entry/libCLC.map

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libdl \
	libLLVM_viv \
	libVSC \
	libGAL

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) "<" 23),1)
LOCAL_SHARED_LIBRARIES += \
	libstlport
endif

LOCAL_STATIC_LIBRARIES := \
	libclCompiler \
	libclPreprocessor \
	libclCommon

LOCAL_MODULE         := libCLC
LOCAL_MODULE_TAGS    := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

include $(AQROOT)/copy_installed_module.mk

endif
