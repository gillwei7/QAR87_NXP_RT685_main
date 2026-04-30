<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system_file>
<xtensa_configuration>
<config_name buildTS="1584591563817" isaSavTS="1584591540358" parent_bid="0" parent_tdb_cksm="0" tieSavTS="0" variant="0">XRC_7C0_v2_ECC</config_name>
<config_isa description="">
<core formatversion="7000" isaversion="LX7.0" locked="0" name="XRC_7C0_v2_ECC">
<coreisa>
<isa abs="1" addx="1" arCount="16" branchPredictedTaken="0" brs="0" byteOrder="le" call4_12="0" cbox="0" clamps="0" condStoreSync="1" density="1" div32Int="0" fp="0" hifi2="0" hifi2hd="0" instWidthBytes="4" l32r="l32r" loops="1" mac16="0" maxCoprocs="1" minMax="1" miscSpecRegs="0" mul16="0" mul32="0" mul32US="0" mulIter="0" mulUSReadStage="1" mulUSWriteStage="2" mulUser="0" nsa="0" prid="0" sext="0" sync="1" threadptr="0" tieArbitraryByteEnable="0" tieWideStore="1" unalignedLoadAction="exception" unalignedStoreAction="exception" vectra2="0" vfp="0"/>
<isa2 connx="0" cx_bbe16="0" cx_bbe16.despread="0" cx_bbe16.rsqrtx4="0" cx_bbe16.vectorDivide="0" cx_bsp3="0" cx_bsp3.xpose32x32="0" cx_ssp16="0" cx_ssp16.softdemap="0" cx_ssp16.viterbi="0" dfp="0" dfpassist="0" flixedcore570="0" perfcounters="0" stdports="0" stdqueues="0" vectra2opt545="0"/>
<isa4 cx_bben.3gpp_demap="0" cx_bben.despread="0" cx_bben.fft="0" cx_bben.fir="0" cx_bben.lfsr_convenc="0" cx_bben.linblk_dec="0" cx_bben.mulpair.real="0" cx_bben.rsqrtx4="0" cx_bben.symfir="0" cx_bben.uls_1slot="0" cx_bben.vectorDivide="0" cx_mdma.rerwer="0" hifi2_40="0" hifi3="0"/>
<isa5 cx_bbe16.mulSelSlotShared="0" cx_bbep32="0" cx_bbep64="0" cx_ssp16.num_align_regs="4" cxep.3gpp_demap="0" cxep.advprec="0" cxep.advrsqrt="0" cxep.despread1d="0" cxep.dualpeaksearch="0" cxep.fastrsqrt="0" cxep.fft="0" cxep.inverse.llr="0" cxep.lfsr_convenc="0" cxep.linblk_dec="0" cxep.pk.cx.matmul="0" cxep.symfir="0" cxep.vectordivide="0" hifi2_nobit="0" magicBox="" mul32USReadStage="1" mul32USWriteStage="2" sdfp.pre_inc_LS="0"/>
<isa6 connx_dual_ls_flix="0" cx_bbep16="0" cx_bbep8="0" depbits="0" exp_corestate_usertie="0" flix3_570_fc="0" fsn="0" fsn_abmp="0" fsn_aes="0" fsn_avs="0" fsn_bit_shfl="0" fsn_convenc="0" fsn_fp="0" fsn_lfsr_crc="0" fsn_lowpower="0" fsn_quad16="0" fsn_softdemap="0" fsn_viterbi="0" hifi2_lowpower="0" hifi3_acc72="0" hifi3_vector_fp="0" hifi4="0" hifi4_vector_fp="0" ivpep_histogram="0" ivpep_sac="0" pdx="0" user_provided_dpfpu="0" user_provided_spfpu="0"/>
<isa7 arithmetic_exc_int="0" cxep.spvfp="0" hifi3z="0" hifi3z_vector_fp="0" hifi5="0" hifi5_hpvfp="0" hifi5_nn_mac="0" hifi5_spvfp="0" ivp26_32="0" ivp26_histogram="0" ivp26_hpvfp="0" ivp26_quadmac="1" ivp26_spvfp="0" ivp26_vecbypass="0" ivp2_32="0" ivp2_histogram="0" ivp2_spvfp="0" ivp2_vecbypass="0" ivp2c_32="0" ivp2ce_32="0" ivp2ce_hpvfp="0" ivp2ce_spvfp="0" pdx4b="0" pdx4b_dfp="0" pdx4b_sfp="0" pdx8="0" pdx8_dfp="0" pdx8_sfp="0" sg_gather="0" sg_scatter="0"/>
<isa7b vpu_dsp="1" vpu_poolq="0" xnne2_addr="0x40000000" xnne2_mem_error="0" xnne2_num_m_per_s="1" xnne2_num_sblks="0" xnne2_sblk_axi_width="128" xnne2_ubuf_size="0x00040000" xnne_addr="0x30000000" xnne_num_m_per_s="1" xnne_num_sblks="0" xnne_sblk_axi_width="128"/>
<implementation L0IBuffer="0" implBlock="flop" implCgFunctionalUnit="1" implCgGlobal="1" implResetFlops="1" implasyncreset="1" mem.data.gating="0" pso.core.ret="None" pso.domains="None" sem.data.gating="None"/>
<dataCache cache_test_ops="1" dc.coherent="0" dc.lineSize="16" dc.locking="0" dc.memErr="0" dc.nbanks="1" dc.size="0" dc.ways="1" dc.writeBack="0" dynamic_way_disable="0"/>
<instCache ic.lineSize="16" ic.locking="0" ic.memErr="0" ic.size="0" ic.ways="1" ic.widthBits="0"/>
<memerror dataMemErrorType="2" dataMemErrorWidth="4" dramRetry="0" instMemErrorType="2" prefetch_memerror="0"/>
<lmem>
<lmem.0 busy="0" dma="0" memErr="1" paddr="0xa0000000" size="0x10000" type="instram.0"/>
<lmem.1 busy="0" dma="0" memErr="1" nbanks="1" paddr="0x9fffe000" size="0x2000" type="dataram.0"/>
</lmem>
<smem/>
<datainterface dataWidthBits="32" loadStoreUnits="1" sg_subbanks="4"/>
<instinterface iFetchWidthBits="32"/>
<pif busBridge="none" dmaDepth="4" pifAsync="0" pifEnabled="0" pifWidthBits="0" pifWriteResp="0" pifver_32="0" prefetchEntries="0" wbBypassAddrBits="0" wbEntries="4"/>
<pif5 bs.req.ctl.dep="4" bs.req.data.dep="8" bs.rsp.dep="8" early.restart="0" pif.arb.byte.enable="0" pif.crit.word.first="0" prefetchCastoutLines="1" prefetchToL1="0"/>
<pif6 prefetchBlockEntries="0"/>
<pif7 axi_acelite="0" axi_ecc="none" axi_sec_interface="0" ir_drop_port="0" master_exclaccess="0" num_extn_exclmaster="0" slave_exclaccess="0" udma="0" udma_bufferwidth="4" udma_data_width="0" udma_outstanding_rows="4" udma_reorder_buffer="0"/>
<debug breakInBreakOut="0" dataTraps="0" debugAPB="0" debugEnabled="0" dtrace="0" instTraps="0" numRomPatchRegs="0" ocd="0" scan="1" trace="0" transparentLatches="0" traxMemBytes="0" traxTimestamps="0"/>
<trax traxApbCtl="0" traxMemShared="0"/>
<interrupt debugIntLevel="0" execmIntLevel="1" intCount="0" intLevelCount="1" intTimerCount="0"/>
<mmu dataEntries="4" immu_noofmpuentries="32" instEntries="4" mmuType="CaMMU"/>
<vector vecBase1FromPins="0" vectorsReloc="0" vectorsStaticAltBase="0x00000000">
<vecReset prefix="0x0" size="0x300" vaddr="0xa0000000"/>
<vecMemErr prefix="0x30" size="0x180" vaddr="0xa0000600"/>
<vecKernel prefix="0x8" size="0x38" vaddr="0xa0000308"/>
<vecUser prefix="0x8" size="0x38" vaddr="0xa0000348"/>
<vecDouble prefix="0x48" size="0x40" vaddr="0xa00003c8"/>
</vector>
<pipeline bStage="1" eStage="1" iFetchLatency="2" mStage="3" wStage="4"/>
</coreisa>
<targets>
<sw sw_abi="call0" sw_altreset="0" sw_clibs="xclib" sw_extl32r="0" sw_fixes="" sw_floating_abi="0" sw_mvlinux="0" sw_nucleus="0"/>
<cfg cfg_auto_mmap="0" cfg_auto_vectors="0" cfg_mmu_lmems_space="0xFE000000" cfg_vector_style="lx5_iram" fusa_iso26262="0" template_op="none"/>
</targets>
</core>
</config_isa>
<tie_file name="" shared="0"/>
</xtensa_configuration>
</system_file>
