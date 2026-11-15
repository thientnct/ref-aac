# ---------------------------------------------------------------------------
# mp4AudVm_Rewrite - mp4auenc (AAC-LC encoder)
# ---------------------------------------------------------------------------
set(AAC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# Source directories searched (in priority order) for each object basename.
set(AAC_LC_SRC_DIRS
  src_frame_enc
  src_tf_enc
  src_frame
  src_mp4
  src_tf
  src_par
  src_lpc
  src_ssc
  src_ssc/import
  src_mpeg12
  include
)

# Object basenames required for mp4auenc (AAC-LC, TF/AAC path).
set(AAC_LC_OBJS
  mp4auenc
  common_m4a flex_mux bitstream plotmtv_interf_dummy statistics_dummy resilience
  cmdline audio austream fir_filt adif
  streamfile streamfile_helper streamfile_diagnose streamfile_mp4
  streamfile_fl4 streamfile_latm streamfile_raw
  util transfo imdct nok_bwp_enc nok_ltp_enc nok_pitch decdata
  enc_tf ms
  aac_qc aac_tools tns3 bitmux
  psych
  ntt_BitPack ntt_TfInit ntt_check ntt_cnst_chk ntt_dec_bark_env
  ntt_dec_gain ntt_dec_lpc_spectrum_inv ntt_dec_pit_seq ntt_dec_pgain
  ntt_denormalizer_spectrum ntt_extend_pitch ntt_fwex ntt_get_code
  ntt_get_cdbk ntt_lsp_decw ntt_lsptowt_int ntt_movdd ntt_mulawinv
  ntt_post_process ntt_redec ntt_scale_coder ntt_scale_init
  ntt_scale_message ntt_scale_tf_proc_spectrum_d ntt_scale_vq_coder
  ntt_scale_vq_decoder ntt_set_isp ntt_tf_perceptual_model
  ntt_tf_proc_spectrum ntt_tf_quantize_spectrum ntt_tf_requantize_spectrum
  ntt_tools ntt_vec_lenp ntt_vex_pn ntt_vq_coder ntt_win_sw ntt_zerod
  ntt_scale_dec_bark_env ntt_dec_sq_gain mat_scale_set_shift_para
  ntt_vq_decoder ntt_tf_proc_spectrum_d ntt_dec_pitch
  mat_scale_tf_requantize_spectrum
  son_gc_common son_pqf_common son_gc_detecter son_gc_detectreset
  son_gc_etc_enc son_gc_modifier son_gc_pack son_pqf
  sam_encode_frame sam_encode_bsac sam_faencode sam_bit_buffer
  scal_enc_frame ct_polyphase_enc ct_sbr
)

function(find_aac_source out_var base)
  foreach(dir ${AAC_LC_SRC_DIRS})
    set(candidate "${AAC_ROOT}/${dir}/${base}.c")
    if(EXISTS "${candidate}")
      set(${out_var} "${candidate}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
  message(WARNING "AAC-LC source not found: ${base}.c")
endfunction()

set(MP4AUENC_SOURCES "")
foreach(base ${AAC_LC_OBJS})
  find_aac_source(src "${base}")
  if(src)
    list(APPEND MP4AUENC_SOURCES "${src}")
    set(src "")
  endif()
endforeach()

add_executable(mp4auenc ${MP4AUENC_SOURCES})

foreach(dir ${AAC_LC_SRC_DIRS})
  target_include_directories(mp4auenc PRIVATE "${AAC_ROOT}/${dir}")
endforeach()

target_compile_definitions(mp4auenc PRIVATE
  VERSION2
  CORRIGENDUM1
  ESC_SUPPORT
  AL_CONFIG
  CT_SBR
  PARAMETRICSTEREO
  MPEG12
  AAC_ELD
  MP4FF
  USELIB_ISOMP4
  SRS
  DOUBLE_WIN
  DOLBY_MDCT
  PNS
  DRC
  WIN32
  USE_AFSP
  USELIB_AFsp
)

target_link_libraries(mp4auenc PRIVATE isomediafile tsp winmm m)
