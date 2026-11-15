# ---------------------------------------------------------------------------
# mp4AudVm - mp4auenc (MPEG-4 Natural Audio Encoder, full VM: PAR/LPC/TF/SSC)
# ---------------------------------------------------------------------------
set(VM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# Source directories searched (in priority order) for each object basename.
set(VM_SRC_DIRS
  src_frame_enc
  src_par_enc
  src_lpc_enc
  src_tf_enc
  src_ssc_enc
  src_ssc_enc/analysis_support
  src_ssc_enc/quantisation_support
  src_ssc_enc/audio_read
  src_ssc_enc/sbunch
  src_ssc_enc/sinusoid_analysis
  src_ssc_enc/filter
  src_ssc_enc/sinusoid_cont_phase
  src_ssc_enc/stereo_analysis
  src_ssc_enc/formatter
  src_ssc_enc/sinusoid_psycho
  src_ssc_enc/stereo_quantisation
  src_ssc_enc/include
  src_ssc_enc/sinusoid_quantisation
  src_ssc_enc/transient_analysis
  src_ssc_enc/noise_analysis
  src_ssc_enc/sinusoid_remove
  src_ssc_enc/transient_detection
  src_ssc_enc/noise_quantisation
  src_ssc_enc/sinusoid_synthesis
  src_ssc_enc/transient_quantisation
  src_ssc_enc/noise_temp_env
  src_ssc_enc/sinusoid_tracking
  src_ssc_enc/transient_synthesis
  src_frame
  src_par
  src_lpc
  src_tf
  src_tf_fhg
  src_latm
  src_mp4
  include
  include_enc
  tables
  tables_enc
)

# Object basenames required for mp4auenc (encoder, VERSION=2, PAR=1, LPC=1,
# TF=1, SSC=1, ISOMP4=1, USE_AFSP=1, PICOLA=1(dec only), STATISTICS=0).
set(VM_ENC_OBJS
  # --- frame work (OBJS_COM + main) ---
  mp4auenc mp4mux
  common_m4a flex_mux bitstream plotmtv_interf_dummy statistics_dummy resilience
  cmdline audio austream fir_filt adif
  # --- parametric core (PAR) ---
  enc_par indilinecom indilineqnt indilineenc uhd_fft indilinextr_basic
  uhd_psy_basic
  hvxcCommon hvxcEnc hvxcFExtEnc hvxcFPExtEnc hvxcFltEnc hvxcLpcAnaEnc
  hvxcPWEnc hvxcPchEnc hvxcQAmEnc hvxcQLspEnc hvxcVUVEnc hvxcVxcEnc
  hvxcProtect
  # --- LPC core (LPC) ---
  enc_lpc celp_encoder phi_prep phi_lpc phi_apre celp_bitstream_mux
  phi_axit phi_xits phi_fbit phi_lsfr nec_bws_lpf pan_lspdec phi_priv
  att_lsf2pc att_pc2lsf att_testbound nb_celp_enc pan_lspqtz2_dd
  pan_lsp_intp pan_mv_data pan_sort pan_stab nec_abs_exc_analysis
  nec_qrms_enc nec_mode_decision nec_exc_acb_enc nec_exc_mp_enc
  nec_exc_gain_enc nec_exc_mp_config nec_exc_module nec_enh_mp_enc
  nec_enh_gain_enc nec_bws_acb_enc nec_bws_exc_analysis nec_bws_gain_enc
  nec_bws_mp_enc nec_bws_qlsp_enc nec_bws_module nec_bws_qrms_dec
  phi_nec_lpc
  nec_cng nec_cng_module nec_cng_lspvq nec_dtx nec_vad
  # --- t/f core (TF) ---
  util transfo imdct nok_ltp_enc nok_pitch decdata tf_tables enc_tf
  aac_qc tns3 bitmux psych scal_enc_frame
  # decoder-core LPC objects linked into encoder (OBJS_DEC_LPC_CORE)
  celp_decoder celp_bitstream_demux phi_post phi_gxit
  att_abs_postp att_bwx att_firfilt att_iirfilt nb_celp_dec
  nec_abs_exc_generation nec_qrms_dec nec_exc_acb_dec nec_exc_mp_dec
  nec_exc_gain_dec nec_enh_mp_dec nec_enh_gain_dec nec_bws_acb_dec
  nec_bws_exc_generation nec_bws_gain_dec nec_bws_mp_dec nec_bws_qlsp_dec
  nec_cng_dec
  # --- NTT VQ coder ---
  ntt_BitPack ntt_SclBitPack ntt_TfInit ntt_check ntt_cnst_chk
  ntt_dec_bark_env ntt_dec_gain ntt_dec_lpc_spectrum_inv ntt_dec_pit_seq
  ntt_dec_pgain ntt_denormalizer_spectrum ntt_extend_pitch ntt_fwex
  ntt_get_code ntt_get_cdbk ntt_lsp_decw ntt_lsptowt_int ntt_movdd
  ntt_mulawinv ntt_post_process ntt_redec ntt_scale_coder ntt_scale_init
  ntt_scale_message ntt_scale_tf_proc_spectrum_d ntt_scale_vq_coder
  ntt_tf_proc_spectrum_d
  ntt_scale_vq_decoder ntt_set_isp ntt_tf_perceptual_model
  ntt_tf_pre_process ntt_tf_proc_spectrum ntt_tf_quantize_spectrum
  ntt_tf_requantize_spectrum ntt_tools ntt_vec_lenp ntt_vex_pn
  ntt_vq_coder ntt_zerod ntt_scale_dec_bark_env ntt_dec_sq_gain
  mat_scale_set_shift_para ntt_vq_decoder ntt_dec_pitch
  mat_scale_tf_requantize_spectrum ntt_flex_mux
  # --- SON AAC++ pre/post processing ---
  son_gc_common son_pqf_common son_gc_detecter son_gc_detectreset
  son_gc_etc_enc son_gc_modifier son_gc_pack son_pqf
  # --- BSAC coder ---
  sam_encode_frame sam_encode_bsac sam_faencode sam_bit_buffer
  # --- SSC (parametric, EXT2PAR) ---
  SSC_fft SSC_fft_ff SSC_hires_fft Imp_Audio SSC_Audio Filter
  Imp_Filter LagFilter SSC_Filter bsc_buffer bsc_wres AdpcmTable
  E_hufAdpcmGrid E_hufAmpBirthAbs E_hufAmpBirthRel E_hufAmpContAbs
  E_hufAmpContRel_0 E_hufAmpContRel_1 E_hufAmpContRel_2 E_hufAmpContRel_3
  E_hufFreqBirthAbs E_hufFreqBirthRel E_hufFreqContAbs
  E_hufFreqContRel_0 E_hufFreqContRel_1 E_hufFreqContRel_2
  E_hufFreqContRel_3 E_hufNoiRelLag E_hufNoiRelLsfE E_hufNoiRelNdgE
  E_huf_CoarseIid E_huf_CoarseItd E_huf_CoarseRho E_huf_CoarseRhoGlobal
  E_huf_FineIid E_huf_FineItd E_huf_FineRho E_huf_FineRhoGlobal
  E_huf_stereo E_huf_s_cont E_huf_s_nrof_births
  SSC_Formatter SSC_Streaming Imp_noise_analysis laguerre_analysis
  SSC_noise_analysis SSC_Vector win Imp_noise_quantisation
  SSC_noise_quantisation freqlpc Imp_noise_temp_env SSC_noise_temp_env
  Imp_quantise SSC_Bunch SSC_Log SSC_Pipe Imp_sinusoid_analysis
  SSC_sinusoid_analysis Imp_sinusoid_cont_phase SSC_sinusoid_cont_phase
  Imp_sinusoid_psycho SSC_sinusoid_psycho Imp_sinusoid_quantisation
  SSC_sinusoid_quantisation Imp_sinusoid_remove SSC_sinusoid_remove
  Imp_sinusoid_synthesis SSC_sinusoid_synthesis Imp_sinusoid_tracking
  SSC_sinusoid_tracking Imp_stereo_analysis SSC_stereo_analysis
  ct_polyphase_ssc Imp_stereo_quantisation SSC_stereo_quantisation
  Env_Pipe Imp_transient_analysis SSC_transient_analysis
  Imp_Transient_Detection SSC_Transient_Detection
  Imp_transient_quantisation SSC_transient_quantisation
  Imp_Transient_Synthesis SSC_Transient_Synthesis SscEnc
)

function(find_vm_source out_var base)
  foreach(dir ${VM_SRC_DIRS})
    set(candidate "${VM_ROOT}/${dir}/${base}.c")
    if(EXISTS "${candidate}")
      set(${out_var} "${candidate}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
  message(WARNING "mp4AudVm source not found: ${base}.c")
endfunction()

set(VM_ENC_SOURCES "")
foreach(base ${VM_ENC_OBJS})
  find_vm_source(src "${base}")
  if(src)
    list(APPEND VM_ENC_SOURCES "${src}")
    set(src "")
  endif()
endforeach()
list(REMOVE_DUPLICATES VM_ENC_SOURCES)

add_executable(mp4auenc_vm ${VM_ENC_SOURCES})
set_target_properties(mp4auenc_vm PROPERTIES OUTPUT_NAME mp4auenc)

foreach(dir ${VM_SRC_DIRS})
  target_include_directories(mp4auenc_vm PRIVATE "${VM_ROOT}/${dir}")
endforeach()

target_compile_definitions(mp4auenc_vm PRIVATE
  WIN32
  VERSION2
  CORRIGENDUM1
  ESC_SUPPORT
  AL_CONFIG
  EXT2PAR
  SRS
  DOUBLE_WIN
  DOLBY_MDCT
  PNS
  USE_AFSP
  USELIB_AFsp
  PLATFORM_WIN32_MSVC
  SSC_DEC_LEVEL_MEDIUM
  _SSC_LIB_BUILD
  SSC_MP4REF_BUILD
)

target_link_libraries(mp4auenc_vm PRIVATE tsp isomediafile winmm m)
