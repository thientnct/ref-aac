# ---------------------------------------------------------------------------
# mp4mcEnc - MPEG-4 AAC Main profile encoder (encoder.c, raw bitstream out)
# ---------------------------------------------------------------------------
set(MC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(MC_SRC_DIR "${MC_ROOT}/src")

# Object basenames required for encoder (from mp4mcEnc/makefile:
# OBJS_AAC_QC + OBJS_PSYCH + OBJS_AAC_ENC + enc_tf + OBJS_LTP +
# OBJS_SON_AACPP_ENC + OBJS_CT_SBR + OBJS_COM + encoder).
# OBJS_ENC_PAR / OBJS_ENC_LPC / OBJS_VQ_ENC / OBJS_MOT_QC are empty for this
# AAC-only module.
set(MC_ENC_OBJS
  encoder
  # plotmtv_interf.c excluded: Unix-only (unistd.h/sys/wait.h), plotting is
  # optional debug aid; nothing else references its symbols.
  common cmdline bitstream audio austream
  enc_tf
  aac_qc aac_se_enc aac_back_pred tns mc_enc ms is drc
  psych
  util transfo imdct
  nok_ltp_enc nok_pitch
  son_gc_common son_gc_detecter son_gc_etc_enc son_pqf son_gc_modifier
  son_gc_pack son_pqf_common son_gc_detectreset
  ct_sbr ct_polyphase ct_psenc
  mp4mux
)

set(MC_ENC_SOURCES "")
foreach(base ${MC_ENC_OBJS})
  set(candidate "${MC_SRC_DIR}/${base}.c")
  if(EXISTS "${candidate}")
    list(APPEND MC_ENC_SOURCES "${candidate}")
  else()
    message(WARNING "mp4mcEnc source not found: ${base}.c")
  endif()
endforeach()

add_executable(mp4mcenc ${MC_ENC_SOURCES})
set_target_properties(mp4mcenc PROPERTIES OUTPUT_NAME mp4mcenc)

target_include_directories(mp4mcenc PRIVATE "${MC_SRC_DIR}")

target_compile_definitions(mp4mcenc PRIVATE
  WIN32
  USE_AFSP
  SRS
  DOUBLE_WIN
  DOLBY_MDCT
  OLD_PRED_PARAMS
  PLOTMTV
  SON_AAC_PP
  MPEG4V1
  CT_SBR
  PARAMETRICSTEREO
)
target_link_libraries(mp4mcenc PRIVATE tsp isomediafile winmm m)
