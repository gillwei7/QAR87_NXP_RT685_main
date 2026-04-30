<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system_file>
<xtensa_configuration>
<config_name buildTS="0" isaSavTS="1600386852336" parent_bid="0" parent_tdb_cksm="0" tieSavTS="0" variant="0">XRC_Vision_Q7_2P_r2a</config_name>
<config_isa description="Vision Q7 2P MC Reference (r2)">
<core formatversion="8000" isaversion="NX1.0" locked="0" name="XRC_Vision_Q7_2P_r2a">
<coreisa>
<processors ball="0" ball_addsub8x20p="0" ballap_ioq_expstate="0" ballap_isa="0" bbn_32b_isa="0" bbn_b10="0" bbn_b20="0" bbn_hh_ioq_expstate="0" bbn_hh_isa="0" bbn_sfp="0" bbn_vec_regs="32" fuj6="0" fuj6_dfp="0" fuj6_sfp="0" no_bbe16_isa="0" pdxnx8="0" visionq6="0" vq6_histogram="0" vq6_hpvfp="0" vq6_spvfp="0"/>
<processors2 bbn_dfp="0" bbn_hh_fft="0" bbn_hp_vfp="0" bbn_ldpc="0" bbn_polar="0" bbn_turbo="0" bbn_viterbi="0" flix3="0" flix3_fc="0" scatgat="1" visionq7="1" visionq8="0" vq7_dpvfp="0" vq7_dualquad_8x8_mac="0" vq7_fast_acc="0" vq7_histogram="0" vq7_hpvfp="0" vq7_quad_16x16_mac="0" vq7_spfp_fast_recip="0" vq7_spvfp="0"/>
<implementation clockgating="1" implasyncreset="1" ir_drop_port="0" scan="1"/>
<mmu immu_bgmap="mpu_ca_4kpg" immu_fgentries="32" immu_lock="0" immu_useMinMPUEntry="0" mmuchoice="IMMU"/>
<isa arCount="32" brs="0" depbits="1" div32Int="1" etie_enable="0" fp="0" instWidthBytes="16" l32r="l32r_const16" loops="1" maxCoprocs="7" mul32choice="fullupper" prid="1" threadptr="1"/>
<fetch L0IBuffer="64" btb_entries="128" btb_rasentries="8" btb_ways="4" inst_missbuffers="3"/>
<systemif sys_mif_combine="0" sys_mif_cwf="1" sys_mif_data_apb="1" sys_mif_data_apb_addr="0xd0000000" sys_mif_data_apb_size="0x00200000" sys_mif_data_width="128" sys_mif_inst_width="128" sys_sif_combine="0" sys_sif_data_idwidth="4" sys_sif_data_width="128" sys_sif_inst_idwidth="4" sys_sif_inst_width="0"/>
<pqif stdports="0" stdqueues="0"/>
<prefetch dprefetch_entries="0" dprefetchdowngrade_cacheops="0" iprefetch_entries="0"/>
<l2cache l2_cache_linesize="64" l2_cache_width="512" l2_coherence="0" l2_core_portmuxing="2" l2_ctlregaddr="0x55500000" l2_indexlocking="1" l2_numcores="2" l2_numslaves="2" l2_ram="1" l2_resetaddr="0x55600000" l2_size="0x00100000" masterl3aceidwith="10" slavetol2idwidth="4" slavetol2width="128"/>
<icache ic_linesize="64" ic_size="0x00010000" ic_ways="4"/>
<sdcache sdc_accesswidth="32" sdc_linesize="64" sdc_size="16384" sdc_ways="4"/>
<vdcache vdc_accesswidth="256" vdc_size="0"/>
<localmems data_latency="4" data_width="512" dram_addr="0x3ffc0000" dram_banks="2" dram_count="2" dram_size="0x00020000" inst_width="128" iram_addr="0" iram_count="0" iram_size="32768" loadStoreUnits="2"/>
<memerrors address_err="0" csr_parity="0" dside_errors="0" iside_errors="0"/>
<idma idma="1" idma_addr_width="40" idma_axi_channels="2" idma_buf_depth="4" idma_channels="4" idma_data_width="128" idma_max_descriptor="64" idma_max_reqs="64" idma_reorder_buf="64"/>
<debug dataTraps="2" instTraps="2" numILAentries="64" perfcounters="8" traxMemBytes="8192" traxTimestamps="1"/>
<xea3 alt_reset_addr="0x7ff40000" ints_etie_count="0" ints_ext_count="25" ints_max_level="7" ints_sw_count="3" ints_timer_count="3" prim_reset_addr="0x50000000" vecBase1FromPins="0"/>
<sysmem sram_addr="0x60000000" sram_size="0x20000000" srom_addr="0x50000000" srom_size="0x01000000"/>
<mp cc_idmaportmux="0" cc_inboundportdemux="0" cc_interconnect_base="0x10000000" cc_reorderbuffer="0" ints_broadcast_pins="4" ints_intracore_pins="4" mp_numOfL2ExternalInboundPorts="1" mp_numOfL3iDMAPorts="2" mp_psodomain="0"/>
</coreisa>
<targets>
<sw sw_abi="windowed" sw_altreset="0" sw_clibs="xclib"/>
<cfg fusa_iso26262="0" template_op="none"/>
</targets>
<build ConfigID0="0" ConfigID1="0" ConfigKey0="0" ConfigKey1="0" build_req_id="" release="">
<software sw_c_lib="" sw_earliesthw="" sw_platforms="" sw_redist="" sw_targets="" sw_version=""/>
<syssim syssim_targets=""/>
<hardware hw_emulation="" hw_platforms="" hw_rtl="" hw_version=""/>
<messages/>
</build>
</core>
</config_isa>
<tie_file name="" shared="0"/>
</xtensa_configuration>
</system_file>
