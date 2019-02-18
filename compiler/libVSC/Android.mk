##############################################################################
#
#    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
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

#
# libVSC
#
include $(CLEAR_VARS)

ifndef FIXED_ARCH_TYPE
LOCAL_SRC_FILES := \
	old_impl/optimizer/gc_vsc_old_optimizer_dump.c \
	old_impl/optimizer/gc_vsc_old_optimizer.c \
	old_impl/optimizer/gc_vsc_old_optimizer_loadtime.c \
	old_impl/optimizer/gc_vsc_old_optimizer_util.c \
	old_impl/gc_vsc_old_compiler.c \
	old_impl/gc_vsc_old_recompile.c \
	old_impl/gc_vsc_old_preprocess.c \
	old_impl/gc_vsc_old_linker.c \
	old_impl/gc_vsc_old_hw_code_gen.c \
	old_impl/gc_vsc_old_hw_linker.c \
	old_impl/gc_vsc_vir_gcsl_converter.c \
	old_impl/gc_vsc_gcsl_vir_converter.c \
	drvi/gc_vsc_drvi_compile.c \
	drvi/gc_vsc_drvi_ep_dump.c \
	drvi/gc_vsc_drvi_ep_io.c \
	drvi/gc_vsc_drvi_link.c \
	chip/gpu/gc_vsc_chip_state_programming.c \
	chip/gpu/gc_vsc_chip_uarch_caps.c \
	chip/gpu/gc_vsc_chip_mc_codec.c \
	chip/gpu/gc_vsc_chip_mc_dump.c \
	asm/gc_vsc_asm_al_codec.c \
	lib/gc_vsc_lib_gl_builtin.c \
	lib/gc_vsc_lib_common.c \
	lib/gc_vsc_lib_gl_patch.c \
	utils/array/gc_vsc_utils_array.c \
	utils/base/gc_vsc_utils_base_node.c \
	utils/base/gc_vsc_utils_bit_op.c \
	utils/base/gc_vsc_utils_data_digest.c \
	utils/base/gc_vsc_utils_dump.c \
	utils/base/gc_vsc_utils_err.c \
	utils/base/gc_vsc_utils_math.c \
	utils/bitvector/gc_vsc_utils_bm.c \
	utils/bitvector/gc_vsc_utils_bv.c \
	utils/bitvector/gc_vsc_utils_sv.c \
	utils/graph/gc_vsc_utils_dg.c \
	utils/graph/gc_vsc_utils_udg.c \
	utils/hash/gc_vsc_utils_hash.c \
	utils/io/gc_vsc_utils_io.c \
	utils/list/gc_vsc_utils_bi_list.c \
	utils/list/gc_vsc_utils_uni_list.c \
	utils/mm/gc_vsc_utils_mm.c \
	utils/mm/gc_vsc_utils_mm_arena.c \
	utils/mm/gc_vsc_utils_mm_buddy.c \
	utils/mm/gc_vsc_utils_mm_primary_pool.c \
	utils/string/gc_vsc_utils_string.c \
	utils/table/gc_vsc_utils_block_table.c \
	utils/tree/gc_vsc_utils_tree.c \
	vir/analysis/gc_vsc_vir_cfa.c \
	vir/analysis/gc_vsc_vir_ts_dfa_iterator.c \
	vir/analysis/gc_vsc_vir_ms_dfa_iterator.c \
	vir/analysis/gc_vsc_vir_du.c \
	vir/analysis/gc_vsc_vir_liveness.c \
	vir/analysis/gc_vsc_vir_ssa.c \
	vir/codegen/gc_vsc_vir_inst_scheduler.c \
	vir/codegen/gc_vsc_vir_reg_alloc.c \
	vir/codegen/gc_vsc_vir_uniform_alloc.c \
	vir/codegen/gc_vsc_vir_mc_gen.c \
	vir/codegen/gc_vsc_vir_ep_gen.c \
	vir/codegen/gc_vsc_vir_ep_back_patch.c \
	vir/ir/gc_vsc_vir_dump.c \
	vir/ir/gc_vsc_vir_ir.c \
	vir/ir/gc_vsc_vir_io.c \
	vir/ir/gc_vsc_vir_symbol_table.c \
	vir/lower/gc_vsc_vir_hl_2_hl_expand.c \
	vir/lower/gc_vsc_vir_hl_2_ml.c \
	vir/lower/gc_vsc_vir_hl_2_ml_expand.c \
	vir/lower/gc_vsc_vir_ll_2_ll_expand.c \
	vir/lower/gc_vsc_vir_ll_2_ll_scalar.c \
	vir/lower/gc_vsc_vir_ll_2_ll_machine.c \
	vir/lower/gc_vsc_vir_lower_common_func.c \
	vir/lower/gc_vsc_vir_pattern.c \
	vir/lower/gc_vsc_vir_ml_2_ll.c \
	vir/lower/gc_vsc_vir_ll_2_mc.c \
	vir/linker/gc_vsc_vir_linker.c\
	vir/passmanager/gc_vsc_options.c \
	vir/passmanager/gc_vsc_vir_pass_mnger.c \
	vir/transform/gc_vsc_vir_misc_opts.c \
	vir/transform/gc_vsc_vir_peephole.c \
	vir/transform/gc_vsc_vir_scalarization.c \
	vir/transform/gc_vsc_vir_simplification.c \
	vir/transform/gc_vsc_vir_cfo.c \
	vir/transform/gc_vsc_vir_cpp.c \
	vir/transform/gc_vsc_vir_cse.c \
	vir/transform/gc_vsc_vir_dce.c \
	vir/transform/gc_vsc_vir_inline.c \
	vir/transform/gc_vsc_vir_fcp.c \
	vir/transform/gc_vsc_vir_uniform.c \
	vir/transform/gc_vsc_vir_cpf.c \
	vir/transform/gc_vsc_vir_static_patch.c \
	vir/transform/gc_vsc_vir_vectorization.c \
	vir/transform/gc_vsc_vir_loop.c \
	vir/transform/gc_vsc_vir_param_opts.c \
	debug/gc_vsc_debug.c \

LOCAL_GENERATED_SOURCES := \
	$(AQREG)

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-Wno-enum-conversion \
	-Wno-sign-compare \
	-Wno-tautological-constant-out-of-range-compare \
	-Wno-tautological-compare

LOCAL_C_INCLUDES := \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/user/arch \
	$(AQROOT)/hal/os/linux/user \
	$(AQARCH)/cmodel/inc \
	$(LOCAL_PATH)/include

LOCAL_LDFLAGS := \
	-Wl,-z,defs \
	-Wl,--version-script=$(LOCAL_PATH)/libVSC.map

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libdl \
	libGAL

LOCAL_MODULE         := libVSC
LOCAL_MODULE_TAGS    := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)


else

LOCAL_SRC_FILES := \
    $(FIXED_ARCH_TYPE)/libVSC.so

LOCAL_MODULE         := libVSC
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
include $(BUILD_PREBUILT)

endif

include $(AQROOT)/copy_installed_module.mk

