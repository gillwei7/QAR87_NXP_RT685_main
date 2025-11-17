<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system_file>
    <xtensa_configuration>
        <config_name buildTS="0" isaSavTS="1658832354639" parent_bid="0" parent_tdb_cksm="0" tieSavTS="0" variant="0">XRC_NX_FLIX3_Cache</config_name>
        <config_isa description="">
            <core formatversion="8000" isaversion="NX1.0" locked="0" name="XRC_NX_FLIX3_Cache">
                <coreisa>
                    <processors ball="0" ball_addsub8x20p="0" ballap_ioq_expstate="0" ballap_isa="0" bbn_32b_isa="0" bbn_b10="0" bbn_b20="0" bbn_hh_ioq_expstate="0" bbn_hh_isa="0" bbn_sfp="0" bbn_vec_regs="32" fuj6="0" fuj6_dfp="0" fuj6_sfp="0" no_bbe16_isa="0" pdxnx8="0" visionq6="0" vq6_histogram="0" vq6_hpvfp="0" vq6_spvfp="0"/>
                    <processors2 bbn_dfp="0" bbn_dp_addsub="0" bbn_hh_fft="0" bbn_hp_vfp="0" bbn_ldpc="0" bbn_polar="0" bbn_sp_addsub="0" bbn_sp_reciprsqrtqli="0" bbn_turbo="0" bbn_viterbi="0" flix3="1" flix3_fc="1" scatgat="0" visionq7="0" visionq8="0" vq7_dpvfp="0" vq7_dualquad_8x8_mac="0" vq7_fast_acc="0" vq7_histogram="0" vq7_hpvfp="0" vq7_quad_16x16_mac="0" vq7_spfp_fast_recip="0" vq7_spvfp="0"/>
                    <processors3 bbn_rmFormatF9="0" fpmnx_220="0" fpmnx_240="0" fpmnx_dpvfp="0" fpmnx_dpvfp_addsub="0" fpmnx_hpvfp="0" fpmnx_latency="1" fpmnx_spvfp="0" vq8_dpvfp="0" vq8_fast_acc="0" vq8_histogram="0" vq8_hpvfp="0" vq8_quad_1024_8x8_mac="0" vq8_quad_256_16x16_mac="0" vq8_quad_256_8x8_mac="1" vq8_quad_512_8x8_mac="1" vq8_spfp_fast_recip="0" vq8_spvfp="0"/>
                    <processors4 vq7_bfloat16_alu_mul="0" vq8_fourbit_mac="0"/>
                    <implementation clockgating="1" implasyncreset="1" ir_drop_port="0" scan="1" teaf="0"/>
                    <mmu immu_bgmap="mpu_ca_4kpg" immu_fgentries="16" immu_lock="0" immu_useMinMPUEntry="0" mmuchoice="IMMU"/>
                    <isa arCount="16" brs="1" depbits="1" div32Int="1" etie_enable="0" fp="0" instWidthBytes="8" l32r="l32r" loops="1" maxCoprocs="7" mul32choice="fullupper" prid="1" threadptr="0" tieware="0"/>
                    <fetch L0IBuffer="0" btb_entries="32" btb_rasentries="8" btb_ways="4" inst_missbuffers="3"/>
                    <systemif sys_mif_combine="0" sys_mif_cwf="0" sys_mif_data_apb="0" sys_mif_data_apb_addr="0" sys_mif_data_apb_size="0x00010000" sys_mif_data_width="64" sys_mif_inst_width="64" sys_sif_combine="0" sys_sif_data_idwidth="4" sys_sif_data_width="64" sys_sif_inst_idwidth="4" sys_sif_inst_width="0"/>
                    <pqif stdports="0" stdqueues="0"/>
                    <prefetch dprefetch_entries="0" dprefetchdowngrade_cacheops="0" iprefetch_entries="0"/>
                    <l2cache l2_cache_linesize="64" l2_cache_width="512" l2_coherence="0" l2_core_portmuxing="0" l2_ctlregaddr="0" l2_indexlocking="0" l2_numcores="1" l2_numslaves="1" l2_ram="0" l2_resetaddr="0" l2_size="0" masterl3aceidwith="10" slavetol2idwidth="4" slavetol2width="64"/>
                    <icache ic_linesize="32" ic_size="32768" ic_ways="2"/>
                    <sdcache sdc_accesswidth="64" sdc_linesize="32" sdc_size="32768" sdc_ways="2"/>
                    <vdcache vdc_accesswidth="256" vdc_size="0"/>
                    <localmems data_latency="2" data_width="64" dram_addr="0x30000000" dram_banks="4" dram_count="1" dram_size="0x00010000" inst_width="64" iram_addr="0" iram_count="0" iram_size="32768" loadStoreUnits="1"/>
                    <memerrors address_err="0" csr_parity="0" dside_errors="0" iside_errors="0"/>
                    <idma idma="0" idma_addr_width="32" idma_axi_channels="0" idma_buf_depth="4" idma_channels="1" idma_data_width="128" idma_max_descriptor="64" idma_max_reqs="32" idma_reorder_buf="0" idma_reorderbuf_burst="8"/>
                    <debug dataTraps="2" instTraps="2" numILAentries="0" perfcounters="0" traxMemBytes="0" traxTimestamps="0"/>
                    <xea3 alt_reset_addr="0x60001000" ints_etie_count="0" ints_ext_count="16" ints_max_level="7" ints_sw_count="8" ints_timer_count="1" prim_reset_addr="0x50000000" vecBase1FromPins="0"/>
                    <sysmem sram_addr="0x60000000" sram_size="0x04000000" srom_addr="0x50000000" srom_size="0x00800000"/>
                    <mp cc_idmaportmux="0" cc_inboundportdemux="0" cc_interconnect_base="0" cc_reorderbuffer="0" ints_broadcast_pins="0" ints_intracore_pins="0" mp_numOfL2ExternalInboundPorts="1" mp_numOfL3iDMAPorts="1" mp_psodomain="0"/>
                    <safety flexlock="0" flexlock_c1key="0" flexlock_c2key="0" flexlock_ekey="0" flexlock_mem_lockstep="0" flexlock_num_dflops="2" flexlock_rkey="0" flexlock_t1key="0" flexlock_t2key="0" wwdt="0" wwdt_bkey="0" wwdt_c1key="0" wwdt_c2key="0" wwdt_ekey="0" wwdt_hb_reset_val="0" wwdt_ikey="0" wwdt_kkey="0" wwdt_reset_val="0" wwdt_rkey="0" wwdt_t1key="0" wwdt_t2key="0"/>
                </coreisa>
                <targets>
                    <sw sw_abi="call0" sw_altreset="0" sw_clibs="xclib"/>
                    <cfg cfg_last_written_ver="919" editor_rel="none" fusa_iso26262="0" template_op="none"/>
                </targets>
            </core>
        </config_isa>
        <tie_file name="" shared="0"/>
    </xtensa_configuration>
</system_file>
