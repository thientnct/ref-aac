# ---------------------------------------------------------------------------
# mp4MultChEnc980508 - MPEG-4 multichannel AAC encoder (encoder.c, raw bitstream)
# ---------------------------------------------------------------------------
set(MC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(MC_SRC_DIR "${MC_ROOT}/src")

# Object basenames required for encoder (from mp4MultChEnc980508/makefile:
# OBJS_AAC_QC + OBJS_PSYCH + OBJS_AAC_ENC + enc_tf + OBJS_COM + encoder).
# OBJS_T2F_ENC / OBJS_VQ_ENC / OBJS_MOT_QC / OBJS_ENC_PAR / OBJS_ENC_LPC are
# empty for this AAC-only module (makefile.tf is commented out).
set(MC_ENC_OBJS
  encoder
  # plotmtv_interf.c excluded: Unix-only (unistd.h/sys/wait.h), plotting is
  # optional debug aid; PLOTMTV is not defined so nothing references it.
  common cmdline bitstream audio austream
  enc_tf
  aac_qc aac_se_enc aac_back_pred tns mc_enc ms is
  nok_ltp_enc nok_pitch
  psych
  util transfo imdct
  # SON AAC++ pre-processing (enc_tf.c references son_gc_* in AAC_PP paths)
  son_gc_common son_gc_detecter son_gc_etc_enc son_pqf son_gc_modifier
  son_gc_pack son_pqf_common son_gc_detectreset
  mp4mux
)

set(MC_ENC_SOURCES "")
foreach(base ${MC_ENC_OBJS})
  set(candidate "${MC_SRC_DIR}/${base}.c")
  if(EXISTS "${candidate}")
    list(APPEND MC_ENC_SOURCES "${candidate}")
  else()
    message(WARNING "mp4MultChEnc980508 source not found: ${base}.c")
  endif()
endforeach()

add_executable(mp4multchenc ${MC_ENC_SOURCES})
set_target_properties(mp4multchenc PROPERTIES OUTPUT_NAME mp4multchenc)

target_include_directories(mp4multchenc PRIVATE "${MC_SRC_DIR}")

target_compile_definitions(mp4multchenc PRIVATE
  WIN32
  USE_AFSP
  SRS
  DOUBLE_WIN
  DOLBY_MDCT
  OLD_PRED_PARAMS
  MPEG4V1
)
target_link_libraries(mp4multchenc PRIVATE tsp isomediafile winmm m)
