<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system_file>
<xtensa_configuration>
<config_name buildTS="0" isaSavTS="1622106827590" parent_bid="0" parent_tdb_cksm="0" tieSavTS="0" variant="0">sample_full_mmu_dual_ls</config_name>
<config_isa description="">
<core formatversion="7000" isaversion="LX7.0" locked="0" name="sample_full_mmu_dual_ls">
<coreisa>
<isa abs="1" addx="1" arCount="64" branchPredictedTaken="0" brs="1" byteOrder="le" call4_12="1" cbox="0" clamps="1" condStoreSync="0" density="1" div32Int="1" fp="0" hifi2="0" hifi2hd="0" instWidthBytes="8" l32r="l32r" loops="1" mac16="0" maxCoprocs="2" minMax="1" miscSpecRegs="0" mul16="0" mul32="0" mul32US="0" mulIter="0" mulUSReadStage="1" mulUSWriteStage="2" mulUser="0" nsa="1" prid="1" sext="1" sync="1" threadptr="1" tieArbitraryByteEnable="1" tieWideStore="1" unalignedLoadAction="exception" unalignedStoreAction="exception" vectra2="0" vfp="0"/>
<isa2 connx="0" cx_bbe16="0" cx_bbe16.despread="0" cx_bbe16.rsqrtx4="0" cx_bbe16.vectorDivide="0" cx_bsp3="0" cx_bsp3.xpose32x32="0" cx_ssp16="0" cx_ssp16.softdemap="0" cx_ssp16.viterbi="0" dfp="0" dfpassist="0" flixedcore570="0" perfcounters="4" stdports="0" stdqueues="0" vectra2opt545="0"/>
<isa4 cx_bben.3gpp_demap="0" cx_bben.despread="0" cx_bben.fft="0" cx_bben.fir="0" cx_bben.lfsr_convenc="0" cx_bben.linblk_dec="0" cx_bben.mulpair.real="0" cx_bben.rsqrtx4="0" cx_bben.symfir="0" cx_bben.uls_1slot="0" cx_bben.vectorDivide="0" cx_mdma.rerwer="0" hifi2_40="0" hifi3="0"/>
<isa5 cx_bbe16.mulSelSlotShared="0" cx_bbep32="0" cx_bbep64="0" cx_ssp16.num_align_regs="4" cxep.3gpp_demap="0" cxep.advprec="0" cxep.advrsqrt="0" cxep.despread1d="0" cxep.dualpeaksearch="0" cxep.fastrsqrt="0" cxep.fft="0" cxep.inverse.llr="0" cxep.lfsr_convenc="0" cxep.linblk_dec="0" cxep.pk.cx.matmul="0" cxep.symfir="0" cxep.vectordivide="0" hifi2_nobit="0" magicBox="" mul32USReadStage="1" mul32USWriteStage="2" sdfp.pre_inc_LS="0"/>
<isa6 connx_dual_ls_flix="0" cx_bbep16="0" cx_bbep8="0" depbits="0" exp_corestate_usertie="0" flix3_570_fc="0" fsn="0" fsn_abmp="0" fsn_aes="0" fsn_avs="0" fsn_bit_shfl="0" fsn_convenc="0" fsn_fp="0" fsn_lfsr_crc="0" fsn_lowpower="0" fsn_quad16="0" fsn_softdemap="0" fsn_viterbi="0" hifi2_lowpower="0" hifi3_acc72="0" hifi3_vector_fp="0" hifi4="0" hifi4_vector_fp="0" ivpep_histogram="0" ivpep_sac="0" pdx="0" user_provided_dpfpu="0" user_provided_spfpu="0"/>
<isa7 arithmetic_exc_int="0" cxep.spvfp="0" hifi3z="0" hifi3z_vector_fp="0" hifi5="0" hifi5_hpvfp="0" hifi5_nn_mac="0" hifi5_spvfp="0" ivp26_32="0" ivp26_histogram="0" ivp26_hpvfp="0" ivp26_quadmac="0" ivp26_spvfp="0" ivp26_vecbypass="0" ivp2_32="0" ivp2_histogram="0" ivp2_spvfp="0" ivp2_vecbypass="0" ivp2c_32="0" ivp2ce_32="0" ivp2ce_hpvfp="0" ivp2ce_spvfp="0" pdx4b="0" pdx4b_dfp="0" pdx4b_sfp="0" pdx8="0" pdx8_dfp="0" pdx8_sfp="0" sg_gather="0" sg_scatter="0"/>
<isa7b cxep.rm_format_f5="0" etie_enable="0" fpmlx_128="0" fpmlx_spvfp="0" fpmlx_vecregs="32" hifi_le="0" hifi_le_fp="0" ivp26_fourbit_mac="0" visionp1="0" vp1_dualquad_mac_8x8="0" vp1_histogram="0" vp1_hp_vfp="0" vp1_paired_mac="0" vp1_quad_mac_16x16="0" vp1_quad_mac_8x8="0" vp1_sp_vfp="0" vpu_dsp="0" vpu_poolq="0" xnne2_addr="0x40000000" xnne2_mem_error="0" xnne2_num_m_per_s="1" xnne2_num_sblks="0" xnne2_sblk_axi_width="128" xnne2_ubuf_size="0x00040000" xnne_addr="0x40000000" xnne_num_m_per_s="1" xnne_num_sblks="0" xnne_sblk_axi_width="128"/>
<isa7c fpmlx_512="0" fpmlx_dpvfp="0" fpmlx_dpvfp_addsub="0" fpmlx_hpvfp="0" tieware="0"/>
<implementation L0IBuffer="256" implBlock="flop" implCgFunctionalUnit="1" implCgGlobal="1" implResetFlops="1" implasyncreset="1" mem.data.gating="1" pso.core.ret="None" pso.domains="None" sem.data.gating="All" teaf="0"/>
<dataCache cache_test_ops="1" dc.coherent="0" dc.lineSize="64" dc.locking="1" dc.memErr="0" dc.nbanks="4" dc.size="0x00020000" dc.ways="4" dc.writeBack="1" dynamic_way_disable="1"/>
<instCache ic.lineSize="64" ic.locking="1" ic.memErr="0" ic.size="32768" ic.ways="4" ic.widthBits="128"/>
<memerror address_err="0" dataMemErrorType="0" dataMemErrorWidth="1" dramRetry="0" instMemErrorType="0" prefetch_memerror="0"/>
<lmem/>
<smem>
<smem.0 paddr="0x00020000" secaddr="0x0" secsize="0x0" size="0xfe0000" type="sysram.0"/>
<smem.1 paddr="0x00000000" secaddr="0x0" secsize="0x0" size="0x20000" type="sysrom.0"/>
</smem>
<datainterface dataWidthBits="128" loadStoreUnits="2" sg_subbanks="4"/>
<instinterface iFetchWidthBits="64"/>
<pif busBridge="AXI4" dmaDepth="2" pifAsync="0" pifEnabled="1" pifWidthBits="128" pifWriteResp="1" pifver_32="1" prefetchEntries="16" wbBypassAddrBits="1" wbEntries="32"/>
<pif5 bs.req.ctl.dep="4" bs.req.data.dep="8" bs.rsp.dep="8" early.restart="1" pif.arb.byte.enable="1" pif.crit.word.first="1" prefetchCastoutLines="1" prefetchToL1="0"/>
<pif6 prefetchBlockEntries="0"/>
<pif7 axi_acelite="0" axi_ecc="none" axi_sec_interface="0" ir_drop_port="0" master_exclaccess="0" num_extn_exclmaster="0" slave_exclaccess="0" udma="0" udma_bufferwidth="4" udma_data_width="0" udma_outstanding_rows="4" udma_reorder_buffer="0"/>
<debug breakInBreakOut="1" dataTraps="2" debugAPB="1" debugEnabled="1" dtrace="1" instTraps="2" numRomPatchRegs="0" ocd="1" scan="1" trace="1" transparentLatches="0" traxMemBytes="1024" traxTimestamps="1"/>
<trax traxApbCtl="1" traxMemShared="0"/>
<interrupt debugIntLevel="3" execmIntLevel="2" intCount="5" intLevelCount="3" intTimerCount="1">
<int.0 level="2" type="level"/>
<int.1 level="1" type="writeerr"/>
<int.2 level="1" type="profiling"/>
<int.3 level="1" type="timer.0"/>
<int.4 level="1" type="sw"/>
</interrupt>
<mmu altPageSizes="1" dataEntries="8" dataWays4="8" dataWays7="4" extMemAttributes="1" immu_lock="0" immu_noofmpuentries="32" immu_useMinMPUEntry="0" instEntries="8" instWays4="8" mmuType="LinuxMMU"/>
<vector vecBase1FromPins="0" vectorsReloc="1" vectorsStaticAltBase="0x00000c00">
<vecReset prefix="0x0" size="0x300" vaddr="0x00000800"/>
<vecWindow prefix="0x0" size="0x178" vaddr="0x00001800"/>
<vecLevel.2 prefix="0x8" size="0x38" vaddr="0x00001980"/>
<vecLevel.3 prefix="0x8" size="0x38" vaddr="0x000019c0"/>
<vecKernel prefix="0x8" size="0x38" vaddr="0x00001a00"/>
<vecUser prefix="0x8" size="0x38" vaddr="0x00001a40"/>
<vecDouble prefix="0x48" size="0x140" vaddr="0x00001ac0"/>
</vector>
<safety csr_parity="0" flexlock="0" flexlock_c1key="0" flexlock_c2key="0" flexlock_ekey="0" flexlock_mem_lockstep="0" flexlock_num_dflops="2" flexlock_rkey="0" flexlock_t1key="0" flexlock_t2key="0" secure_mode="0" wwdt="0" wwdt_bkey="0" wwdt_c1key="0" wwdt_c2key="0" wwdt_ekey="0" wwdt_hb_reset_val="0" wwdt_ikey="0" wwdt_kkey="0" wwdt_reset_val="244" wwdt_rkey="0" wwdt_t1key="0" wwdt_t2key="0"/>
<pipeline bStage="1" eStage="1" iFetchLatency="2" mStage="3" wStage="4"/>
</coreisa>
<targets>
<cad cad_conditions="Worst" cad_gates="0x0007f3ff" cad_geometry="28hm" cad_leakage_conditions="Typical" cad_mhz="729" cad_mhz_mode="cad_mhz_value" cad_user_lib="Default,1.0,1.0,1.0,1.0,1.0"/>
<sw sw_abi="windowed" sw_altreset="0" sw_clibs="xclib" sw_extl32r="0" sw_fixes="" sw_floating_abi="0" sw_mvlinux="0" sw_nucleus="0"/>
<cfg cfg_auto_mmap="1" cfg_auto_vectors="0" cfg_last_written_ver="817" cfg_mmu_lmems_space="0x00000001" cfg_vector_style="lx7_linuxmmu_sysmem" editor_rel="none" fusa_iso26262="0" template_op="none"/>
</targets>
<build ConfigID0="0" ConfigID1="0" ConfigKey0="0" ConfigKey1="0" build_req_id="" release="">
<software sw_c_lib="" sw_earliesthw="" sw_fusa="0" sw_platforms="" sw_redist="" sw_targets="" sw_version=""/>
<syssim syssim_targets=""/>
<hardware hw_emulation="" hw_fusa="0" hw_platforms="" hw_rtl="" hw_version=""/>
<messages>
<msg.1 msg.1.origin="Xplorer" msg.1.sev="2" msg.1.txt="The selected value of &amp;lt;Memory management selection&amp;gt; (Full MMU with TLBs) recommends that &amp;lt;Misc Special registers&amp;gt; is at least 2"/>
<msg.2 msg.2.origin="Xplorer" msg.2.sev="2" msg.2.txt="The selected value of &amp;lt;Memory management selection&amp;gt; (Full MMU with TLBs) recommends that &amp;lt;Conditional Store Sync&amp;gt; is selected"/>
<msg.3 msg.3.origin="Xplorer" msg.3.sev="2" msg.3.txt="Data Cache beyond 64k is supported, but may cause significant speed (MHz) degradation"/>
<msg.4 msg.4.origin="Xplorer" msg.4.sev="2" msg.4.txt="if the instruction cache way size (cache size / number of ways) &amp;gt; 4 kB, ensure any OS making use of the MMU page translation features supports page coloring"/>
<msg.5 msg.5.origin="Xplorer" msg.5.sev="2" msg.5.txt="The selected value of &amp;lt;Memory management selection&amp;gt; (Full MMU with TLBs) recommends that &amp;lt;Count of timers&amp;gt; is at least 2"/>
<msg.6 msg.6.origin="Xplorer" msg.6.sev="2" msg.6.txt="if the data cache way size (cache size / number of ways) &amp;gt; 4 kB, ensure any OS making use of the MMU page translation features supports page coloring"/>
</messages>
</build>
</core>
</config_isa>
<tie_file name="" shared="0"/>
</xtensa_configuration>
</system_file>
