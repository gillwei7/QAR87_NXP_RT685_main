<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system_file>
    <xtensa_configuration>
        <config_name buildTS="0" isaSavTS="1652852720446" parent_bid="0" parent_tdb_cksm="0" tieSavTS="0" variant="0">vp6_ao_new</config_name>
        <config_isa description="">
            <core formatversion="7000" isaversion="LX7.0" locked="0" name="vp6_ao_new">
                <coreisa>
                    <isa abs="1" addx="1" arCount="32" branchPredictedTaken="0" brs="1" byteOrder="le" call4_12="1" cbox="1" clamps="1" condStoreSync="0" density="1" div32Int="1" fp="0" hifi2="0" hifi2hd="0" instWidthBytes="16" l32r="l32r_const16" loops="1" mac16="0" maxCoprocs="3" minMax="1" miscSpecRegs="0" mul16="1" mul32="1" mul32US="1" mulIter="0" mulUSReadStage="1" mulUSWriteStage="2" mulUser="0" nsa="1" prid="1" sext="1" sync="1" threadptr="1" tieArbitraryByteEnable="1" tieWideStore="1" unalignedLoadAction="exception" unalignedStoreAction="exception" vectra2="0" vfp="0"/>
                    <isa2 connx="0" cx_bbe16="0" cx_bbe16.despread="0" cx_bbe16.rsqrtx4="0" cx_bbe16.vectorDivide="0" cx_bsp3="0" cx_bsp3.xpose32x32="0" cx_ssp16="0" cx_ssp16.softdemap="0" cx_ssp16.viterbi="0" dfp="0" dfpassist="0" flixedcore570="0" perfcounters="4" stdports="0" stdqueues="0" vectra2opt545="0"/>
                    <isa4 cx_bben.3gpp_demap="0" cx_bben.despread="0" cx_bben.fft="0" cx_bben.fir="0" cx_bben.lfsr_convenc="0" cx_bben.linblk_dec="0" cx_bben.mulpair.real="0" cx_bben.rsqrtx4="0" cx_bben.symfir="0" cx_bben.uls_1slot="0" cx_bben.vectorDivide="0" cx_mdma.rerwer="0" hifi2_40="0" hifi3="0"/>
                    <isa5 cx_bbe16.mulSelSlotShared="0" cx_bbep32="0" cx_bbep64="0" cx_ssp16.num_align_regs="4" cxep.3gpp_demap="0" cxep.advprec="0" cxep.advrsqrt="0" cxep.despread1d="0" cxep.dualpeaksearch="0" cxep.fastrsqrt="0" cxep.fft="0" cxep.inverse.llr="0" cxep.lfsr_convenc="0" cxep.linblk_dec="0" cxep.pk.cx.matmul="0" cxep.symfir="0" cxep.vectordivide="0" hifi2_nobit="0" magicBox="" mul32USReadStage="1" mul32USWriteStage="2" sdfp.pre_inc_LS="0"/>
                    <isa6 connx_dual_ls_flix="0" cx_bbep16="0" cx_bbep8="0" depbits="0" exp_corestate_usertie="0" flix3_570_fc="0" fsn="0" fsn_abmp="0" fsn_aes="0" fsn_avs="0" fsn_bit_shfl="0" fsn_convenc="0" fsn_fp="0" fsn_lfsr_crc="0" fsn_lowpower="0" fsn_quad16="0" fsn_softdemap="0" fsn_viterbi="0" hifi2_lowpower="0" hifi3_acc72="0" hifi3_vector_fp="0" hifi4="0" hifi4_vector_fp="0" ivpep_histogram="0" ivpep_sac="0" pdx="0" user_provided_dpfpu="0" user_provided_spfpu="0"/>
                    <isa7 arithmetic_exc_int="0" cxep.spvfp="0" hifi3z="0" hifi3z_vector_fp="0" hifi5="0" hifi5_hpvfp="0" hifi5_nn_mac="0" hifi5_spvfp="0" ivp26_32="1" ivp26_histogram="1" ivp26_hpvfp="1" ivp26_quadmac="1" ivp26_spvfp="1" ivp26_vecbypass="0" ivp2_32="0" ivp2_histogram="0" ivp2_spvfp="0" ivp2_vecbypass="0" ivp2c_32="0" ivp2ce_32="0" ivp2ce_hpvfp="0" ivp2ce_spvfp="0" pdx4b="0" pdx4b_dfp="0" pdx4b_sfp="0" pdx8="0" pdx8_dfp="0" pdx8_sfp="0" sg_gather="1" sg_scatter="1"/>
                    <isa7b cxep.rm_format_f5="0" etie_enable="0" fpmlx_128="0" fpmlx_spvfp="0" fpmlx_vecregs="32" hifi_le="0" hifi_le_fp="0" ivp26_fourbit_mac="0" visionp1="0" vp1_dualquad_mac_8x8="0" vp1_histogram="0" vp1_hp_vfp="0" vp1_paired_mac="0" vp1_quad_mac_16x16="0" vp1_quad_mac_8x8="0" vp1_sp_vfp="0" vpu_dsp="0" vpu_poolq="0" xnne2_addr="0x40000000" xnne2_mem_error="0" xnne2_num_m_per_s="1" xnne2_num_sblks="0" xnne2_sblk_axi_width="128" xnne2_ubuf_size="0x00040000" xnne_addr="0x40000000" xnne_num_m_per_s="1" xnne_num_sblks="0" xnne_sblk_axi_width="128"/>
                    <isa7c fpmlx_512="0" fpmlx_dpvfp="0" fpmlx_dpvfp_addsub="0" fpmlx_hpvfp="0" nna_engine="0" nne_addr="0x40000000" nne_engine="0" nne_num_m_per_s="1" nne_pso="0" nne_ubuf_size="0x00040000" pdx4b_fpm128="0" pdx4b_fpm128_spvfp="0" tieware="0"/>
                    <isa7d bbnlx_128="0" bbnlx_256="0" bbnlx_32b_isa="0" bbnlx_dp_addsub="0" bbnlx_dp_vfp="0" bbnlx_dualpeaksearch="0" bbnlx_fastreciprsqrt="0" bbnlx_fftsymfir="0" bbnlx_hp_vfp="0" bbnlx_ldpc="0" bbnlx_lfsr="0" bbnlx_sp_addsub="0" bbnlx_sp_reciprsqrtqli="0" bbnlx_sp_vfp="0" bbnlx_turbo="0" bbnlx_vec_regs="32" bbnlx_vectordivide="0" bbnlx_viterbi="0" hifi4_sigtanh="0" hifi5_sigtanh="0" ivp26_wideloops="0" pdx4b_wideloops="0" visionp1_fourbit_mac="0" vp1_vecreggroup="0"/>
                    <implementation L0IBuffer="0" implBlock="flop" implCgFunctionalUnit="1" implCgGlobal="1" implResetFlops="1" implasyncreset="1" mem.data.gating="0" pso.core.ret="None" pso.domains="None" sem.data.gating="None" teaf="0"/>
                    <dataCache cache_test_ops="1" dc.coherent="0" dc.lineSize="16" dc.locking="0" dc.memErr="0" dc.nbanks="1" dc.size="0" dc.ways="1" dc.writeBack="0" dynamic_way_disable="1"/>
                    <instCache ic.lineSize="256" ic.locking="0" ic.memErr="0" ic.size="0x00010000" ic.ways="4" ic.widthBits="128"/>
                    <memerror address_err="0" dataMemErrorType="0" dataMemErrorWidth="1" dramRetry="0" instMemErrorType="0" prefetch_memerror="0"/>
                    <lmem>
                        <lmem.0 busy="0" dma="1" memErr="0" nbanks="2" paddr="0x3ffe0000" secsize="0x0" size="0x20000" type="dataram.0"/>
                        <lmem.1 busy="0" dma="1" memErr="0" nbanks="2" paddr="0x3ffc0000" secsize="0x0" size="0x20000" type="dataram.1"/>
                    </lmem>
                    <smem>
                        <smem.0 paddr="0x60000000" secaddr="0x0" secsize="0x0" size="0x20000000" type="sysram.0"/>
                        <smem.1 paddr="0x50000000" secaddr="0x0" secsize="0x0" size="0x1000000" type="sysrom.0"/>
                    </smem>
                    <datainterface dataWidthBits="512" loadStoreUnits="2" sg_subbanks="8"/>
                    <instinterface iFetchWidthBits="128"/>
                    <pif busBridge="AXI4" dmaDepth="2" pifAsync="0" pifEnabled="1" pifWidthBits="128" pifWriteResp="1" pifver_32="1" prefetchEntries="0" wbBypassAddrBits="0" wbEntries="2"/>
                    <pif5 bs.req.ctl.dep="4" bs.req.data.dep="8" bs.rsp.dep="8" early.restart="1" pif.arb.byte.enable="1" pif.crit.word.first="1" prefetchCastoutLines="1" prefetchToL1="0"/>
                    <pif6 prefetchBlockEntries="0"/>
                    <pif7 activityPort="0" axi_acelite="0" axi_ecc="none" axi_sec_interface="0" ir_drop_port="0" master_exclaccess="1" num_extn_exclmaster="1" slave_exclaccess="1" udma="1" udma_bufferwidth="4" udma_data_width="0" udma_outstanding_rows="16" udma_reorder_buffer="0"/>
                    <debug breakInBreakOut="1" dataTraps="2" debugAPB="1" debugEnabled="1" dtrace="0" instTraps="2" numRomPatchRegs="0" ocd="1" scan="1" trace="1" transparentLatches="0" traxMemBytes="0" traxTimestamps="0"/>
                    <trax traxApbCtl="0" traxMemShared="0"/>
                    <interrupt debugIntLevel="3" execmIntLevel="2" intCount="25" intLevelCount="3" intTimerCount="2">
                        <int.0 level="1" type="level"/>
                        <int.1 level="1" type="level"/>
                        <int.2 level="1" type="level"/>
                        <int.3 level="1" type="level"/>
                        <int.4 level="1" type="level"/>
                        <int.5 level="1" type="level"/>
                        <int.6 level="1" type="level"/>
                        <int.7 level="1" type="level"/>
                        <int.8 level="1" type="timer.0"/>
                        <int.9 level="1" type="sw"/>
                        <int.10 level="2" type="level"/>
                        <int.11 level="2" type="level"/>
                        <int.12 level="2" type="level"/>
                        <int.13 level="2" type="level"/>
                        <int.14 level="2" type="timer.1"/>
                        <int.15 level="2" type="sw"/>
                        <int.16 level="1" type="edge"/>
                        <int.17 level="2" type="edge"/>
                        <int.18 level="2" type="profiling"/>
                        <int.19 level="2" type="UDmaDone"/>
                        <int.20 level="2" type="UDmaErr"/>
                        <int.21 level="2" type="SGErr"/>
                        <int.22 level="2" type="edge"/>
                        <int.23 level="2" type="writeerr"/>
                        <int.24 level="4" type="nmi"/>
                    </interrupt>
                    <mmu altPageSizes="0" dataEntries="4" dataWays4="4" dataWays7="1" extMemAttributes="0" immu_lock="0" immu_noofmpuentries="16" immu_useMinMPUEntry="0" instEntries="4" instWays4="4" mmuType="IMMU"/>
                    <vector vecBase1FromPins="0" vectorsReloc="1" vectorsStaticAltBase="0x60000340">
                        <vecReset prefix="0x0" size="0x300" vaddr="0x50000000"/>
                        <vecWindow prefix="0x0" size="0x178" vaddr="0x60000000"/>
                        <vecLevel.2 prefix="0x8" size="0x38" vaddr="0x60000180"/>
                        <vecLevel.3 prefix="0x8" size="0x38" vaddr="0x600001c0"/>
                        <vecNmi prefix="0x8" size="0x38" vaddr="0x60000200"/>
                        <vecKernel prefix="0x8" size="0x38" vaddr="0x60000240"/>
                        <vecUser prefix="0x8" size="0x38" vaddr="0x60000280"/>
                        <vecDouble prefix="0x48" size="0x40" vaddr="0x60000300"/>
                    </vector>
                    <safety csr_parity="0" flexlock="0" flexlock_c1key="0" flexlock_c2key="0" flexlock_ekey="0" flexlock_mem_lockstep="0" flexlock_num_dflops="2" flexlock_rkey="0" flexlock_t1key="0" flexlock_t2key="0" secure_mode="0" wwdt="0" wwdt_bkey="0" wwdt_c1key="0" wwdt_c2key="0" wwdt_ekey="0" wwdt_hb_reset_val="0" wwdt_ikey="0" wwdt_kkey="0" wwdt_reset_val="0" wwdt_rkey="0" wwdt_t1key="0" wwdt_t2key="0"/>
                    <pipeline bStage="1" eStage="1" iFetchLatency="2" mStage="3" wStage="4"/>
                </coreisa>
            </core>
        </config_isa>
        <tie_file name="" shared="0"/>
    </xtensa_configuration>
</system_file>
