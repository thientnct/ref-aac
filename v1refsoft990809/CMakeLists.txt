# ---------------------------------------------------------------------------
# v1refsoft990809 - MPEG-4 Audio VM reference software (TF/AAC codec family only)
# Builds mp4auenc (encoder) and mp4audec (decoder) for the TF (AAC) core.
# PAR/LPC/G.729/G.723 codec families are excluded (PAR=0 LPC=0 G729=0 G723=0).
# The TF core includes the NTT VQ scalable coder and SON AAC++ preprocessing
# objects because enc_tf.c/dec_tf.c reference them unconditionally.
# ---------------------------------------------------------------------------
set(V1_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(V1_FRAME "${V1_ROOT}/src_frame")
set(V1_FRAME_ENC "${V1_ROOT}/src_frame_enc")
set(V1_TF "${V1_ROOT}/src_tf")
set(V1_TF_ENC "${V1_ROOT}/src_tf_enc")

# Find a source file by basename, searching the given dirs in order.
function(v1_find_src out_var)
  set(dirs ${ARGN})
  set(found "")
  foreach(base ${dirs})
    foreach(dir ${V1_TF_ENC} ${V1_TF} ${V1_FRAME_ENC} ${V1_FRAME})
      set(cand "${dir}/${base}.c")
      if(EXISTS "${cand}")
        set(found "${cand}")
        break()
      endif()
    endforeach()
    if(found)
      break()
    endif()
  endforeach()
  if(found)
    set(${out_var} "${found}" PARENT_SCOPE)
  else()
    message(WARNING "v1refsoft990809 source not found: ${dirs}")
    set(${out_var} "" PARENT_SCOPE)
  endif()
endfunction()

# Collect sources for a list of basenames into a CMake list variable.
function(v1_collect out_var)
  set(result "")
  foreach(base ${ARGN})
    v1_find_src(src_path "${base}")
    if(src_path)
      list(APPEND result "${src_path}")
    endif()
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# Common frame objects (OBJS_COM) - src_frame
# ---------------------------------------------------------------------------
set(V1_OBJS_COM
  common_m4a cmdline bitstream audio austream fir_filt flex_mux adif
  plotmtv_interf_dummy statistics_dummy
)

# ---------------------------------------------------------------------------
# Encoder objects (OBJS_ENC = mp4auenc + OBJS_COM + OBJS_ENC_TF)
# ---------------------------------------------------------------------------
set(V1_ENC_TF_OBJS
  # OBJS_AAC_ENC (+ concealment_dummy, tf_celp_dmy for LPC=0)
  util transfo imdct nok_bwp_enc nok_ltp_enc nok_pitch decdata tf_tables
  concealment_dummy tf_celp_dmy
  # enc_tf
  enc_tf
  # OBJS_AAC_QC
  aac_qc tns3 bitmux
  # OBJS_PSYCH
  psych
  # OBJS_VQ_ENC (NTT VQ coder)
  ntt_BitPack ntt_SclBitPack ntt_TfInit ntt_check ntt_cnst_chk
  ntt_dec_bark_env ntt_dec_gain ntt_dec_lpc_spectrum_inv ntt_dec_pit_seq
  ntt_dec_pgain ntt_denormalizer_spectrum ntt_extend_pitch ntt_fwex
  ntt_get_code ntt_get_cdbk ntt_lsp_decw ntt_lsptowt_int ntt_movdd
  ntt_mulawinv ntt_post_process ntt_redec ntt_scale_coder ntt_scale_init
  ntt_scale_message ntt_scale_tf_proc_spectrum_d ntt_scale_vq_coder
  ntt_scale_vq_decoder ntt_set_isp ntt_tf_perceptual_model ntt_tf_pre_process
  ntt_tf_proc_spectrum ntt_tf_quantize_spectrum ntt_tf_requantize_spectrum
  ntt_tools ntt_vec_lenp ntt_vex_pn ntt_vq_coder ntt_zerod
  ntt_scale_dec_bark_env ntt_dec_sq_gain mat_scale_set_shift_para
  ntt_vq_decoder ntt_tf_proc_spectrum_d ntt_dec_pitch
  mat_scale_tf_requantize_spectrum
  # OBJS_SON_AACPP_ENC
  son_gc_common son_pqf_common son_gc_detecter son_gc_detectreset
  son_gc_etc_enc son_gc_modifier son_gc_pack son_pqf
  # scalable frame
  scal_enc_frame
)

# ---------------------------------------------------------------------------
# Decoder objects (OBJS_DEC = mp4audec + OBJS_COM + OBJS_DEC_TF + picola + mod_buf)
# ---------------------------------------------------------------------------
set(V1_DEC_TF_OBJS
  # OBJS_AAC_DEC (+ statistics_aac_dummy, tf_celp_dmy for LPC=0)
  config coupling decdata decoder_tf pns huffdec1 huffdec2 huffdec3
  huffinit hufftables imdct intensity intrins monopred nok_lt_prediction
  nok_prediction stereo tns tns2 transfo util buffers tf_tables
  statistics_aac_dummy tf_celp_dmy
  # dec_tf
  dec_tf
  # OBJS_PSYCH
  psych
  # OBJS_VQ_DEC (NTT VQ coder)
  ntt_BitUnPack ntt_SclBitUnPack ntt_TfInit ntt_check ntt_cnst_chk
  ntt_dec_bark_env ntt_dec_gain ntt_dec_lpc_spectrum_inv ntt_dec_pgain
  ntt_dec_pit_seq ntt_dec_pitch ntt_denormalizer_spectrum ntt_extend_pitch
  ntt_fwex ntt_get_cdbk ntt_get_code ntt_lsp_decw ntt_lsptowt_int ntt_movdd
  ntt_mulawinv ntt_post_process ntt_redec ntt_scale_init ntt_scale_message
  ntt_scale_tf_proc_spectrum_d ntt_scale_vq_decoder ntt_set_isp
  ntt_tf_proc_spectrum_d ntt_tf_requantize_spectrum ntt_vec_lenp ntt_vex_pn
  ntt_vq_decoder ntt_zerod ntt_scale_dec_bark_env ntt_dec_sq_gain
  mat_scale_set_shift_para mat_scale_tf_requantize_spectrum tvqAUDecode
  ntt_headerdec
  # OBJS_SON_AACPP_DEC
  son_gc_unpack son_gc_common son_gc_compensate son_ipqf son_pqf_common
  son_gc_etc
  # scalable frame
  scal_dec_frame
)

# PICOLA=0 -> picola_dmy; mod_buf for decoder
set(V1_DEC_EXTRA_OBJS picola_dmy mod_buf)

# ---------------------------------------------------------------------------
# Build mp4auenc
# ---------------------------------------------------------------------------
v1_collect(V1_ENC_SOURCES "${V1_ENC_TF_OBJS}")
list(APPEND V1_ENC_SOURCES "${V1_FRAME_ENC}/mp4auenc.c")
list(APPEND V1_ENC_SOURCES "${V1_FRAME_ENC}/mp4mux.c")
v1_collect(V1_ENC_COM_SOURCES "${V1_OBJS_COM}")
list(APPEND V1_ENC_SOURCES ${V1_ENC_COM_SOURCES})
# codec-family dummies (PAR=0 LPC=0 G729=0 G723=0)
list(APPEND V1_ENC_SOURCES
  "${V1_ROOT}/src_par_enc/enc_par_dmy.c"
  "${V1_ROOT}/src_lpc_enc/enc_lpc_dmy.c"
  "${V1_ROOT}/src_g729_enc/enc_g729_dmy.c"
  "${V1_ROOT}/src_g723_enc/enc_g723_dmy.c"
  "${V1_ROOT}/src_g729/dec_g729_dmy.c"
)
list(REMOVE_DUPLICATES V1_ENC_SOURCES)

add_executable(v1_mp4auenc ${V1_ENC_SOURCES})
set_target_properties(v1_mp4auenc PROPERTIES OUTPUT_NAME mp4auenc)

target_include_directories(v1_mp4auenc PRIVATE
  "${V1_FRAME}" "${V1_FRAME_ENC}" "${V1_TF}" "${V1_TF_ENC}"
  "${V1_ROOT}/src_lpc" "${V1_ROOT}/src_lpc_enc"
  "${V1_ROOT}/src_par" "${V1_ROOT}/src_par_enc"
  "${V1_ROOT}/src_g729" "${V1_ROOT}/src_g729_enc"
  "${V1_ROOT}/src_g723" "${V1_ROOT}/src_g723_enc"
)

target_compile_definitions(v1_mp4auenc PRIVATE
  WIN32
  USE_AFSP
  AL_CONFIG
  SRS
  DOUBLE_WIN
  DOLBY_MDCT
  PNS
)
target_link_libraries(v1_mp4auenc PRIVATE tsp isomediafile winmm m)

# Post-build: copy mp4auenc.exe to build/ root directory
add_custom_command(TARGET v1_mp4auenc POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:v1_mp4auenc> "${CMAKE_BINARY_DIR}/mp4auenc.exe"
  COMMENT "Copying mp4auenc.exe to build/ root"
)

# ---------------------------------------------------------------------------
# Build mp4audec
# ---------------------------------------------------------------------------
v1_collect(V1_DEC_SOURCES "${V1_DEC_TF_OBJS}")
list(APPEND V1_DEC_SOURCES "${V1_FRAME}/mp4audec.c")
v1_collect(V1_DEC_COM_SOURCES "${V1_OBJS_COM}")
list(APPEND V1_DEC_SOURCES ${V1_DEC_COM_SOURCES})
v1_collect(V1_DEC_EXTRA_SOURCES "${V1_DEC_EXTRA_OBJS}")
list(APPEND V1_DEC_SOURCES ${V1_DEC_EXTRA_SOURCES})
# codec-family dummies (PAR=0 LPC=0 G729=0 G723=0)
list(APPEND V1_DEC_SOURCES
  "${V1_ROOT}/src_par/dec_par_dmy.c"
  "${V1_ROOT}/src_par/hvxcDec_dmy.c"
  "${V1_ROOT}/src_par/hvxc_lpc_dmy.c"
  "${V1_ROOT}/src_lpc/dec_lpc_dmy.c"
  "${V1_ROOT}/src_g729/dec_g729_dmy.c"
  "${V1_ROOT}/src_g723/dec_g723_dmy.c"
)
list(REMOVE_DUPLICATES V1_DEC_SOURCES)

add_executable(v1_mp4audec ${V1_DEC_SOURCES})
set_target_properties(v1_mp4audec PROPERTIES OUTPUT_NAME mp4audec)

target_include_directories(v1_mp4audec PRIVATE
  "${V1_FRAME}" "${V1_FRAME_ENC}" "${V1_TF}" "${V1_TF_ENC}"
  "${V1_ROOT}/src_lpc" "${V1_ROOT}/src_lpc_enc"
  "${V1_ROOT}/src_par" "${V1_ROOT}/src_par_enc"
  "${V1_ROOT}/src_g729" "${V1_ROOT}/src_g729_enc"
  "${V1_ROOT}/src_g723" "${V1_ROOT}/src_g723_enc"
)

target_compile_definitions(v1_mp4audec PRIVATE
  WIN32
  USE_AFSP
  AL_CONFIG
  SRS
  DOUBLE_WIN
  DOLBY_MDCT
  PNS
)
target_link_libraries(v1_mp4audec PRIVATE tsp winmm m)
