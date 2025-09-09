<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system_file>
<xtensa_configuration>
<config_name buildTS="1553812850630" isaSavTS="1553812894937" parent_bid="0" variant="0">nx_5c6_flix3_tcm_timers</config_name>
<config_isa description="-">
<core formatversion="8000" isaversion="NX1.0" locked="0" name="nx_5c6_flix3_tcm_timers">
<coreisa>
<processors ball="0" ball_addsub8x20p="0" ballap_ioq_expstate="0" ballap_isa="0" bbn_32b_isa="0" bbn_b10="0" bbn_b20="0" bbn_hh_ioq_expstate="0" bbn_hh_isa="0" bbn_sfp="0" bbn_vec_regs="32" fuj6="0" fuj6_dfp="0" fuj6_sfp="0" no_bbe16_isa="0" pdxnx8="0" visionq6="0" vq6_histogram="0" vq6_hpvfp="0" vq6_spvfp="0"/>
<processors2 bbn_hh_fft="0" bbn_hp_vfp="0" bbn_viterbi_turbo="0" flix3="1" flix3_fc="1" scatgat="0" visionq7="0" vq7_dualquad_8x8_mac="0" vq7_fast_acc="0" vq7_histogram="0" vq7_hpvfp="0" vq7_quad_16x16_mac="0" vq7_spfp_fast_recip="0" vq7_spvfp="0"/>
<implementation clockgating="1" implasyncreset="1" ir_drop_port="0" scan="1"/>
<mmu immu_bgmap="mpu_ca_4kpg" immu_fgentries="16" mmuchoice="IMMU"/>
<isa arCount="64" brs="0" depbits="1" div32Int="1" fp="0" instWidthBytes="8" l32r="l32r" loops="1" maxCoprocs="7" mul32choice="fullupper" threadptr="0"/>
<fetch L0IBuffer="0" btb_entries="128"/>
<systemif sys_mif_combine="0" sys_mif_cwf="0" sys_mif_data_apb="0" sys_mif_data_apb_addr="0" sys_mif_data_apb_size="0x00010000" sys_mif_data_width="128" sys_mif_inst_width="64" sys_sif_combine="0" sys_sif_data_idwidth="4" sys_sif_data_width="0" sys_sif_inst_idwidth="4" sys_sif_inst_width="0"/>
<pqif stdports="0" stdqueues="0"/>
<prefetch dprefetch_entries="0" dprefetchdowngrade_cacheops="0" iprefetch_entries="0"/>
<l2cache l2_coherence="0" l2_ctlregaddr="0" l2_indexlocking="0" l2_numcores="1" l2_resetaddr="0" l2_size="0" masterl3aceidwith="10" slavetol2idwidth="4" slavetol2width="64"/>
<icache ic_size="0"/>
<sdcache sdc_accesswidth="32" sdc_size="0"/>
<vdcache vdc_accesswidth="256" vdc_size="0"/>
<localmems data_latency="2" data_width="128" dram_addr="0x30000000" dram_banks="2" dram_count="2" dram_size="0x00020000" inst_width="64" iram_addr="0x40000000" iram_count="1" iram_size="0x00020000" loadStoreUnits="2"/>
<memerrors dside_errors="0" iside_errors="0"/>
<idma idma="0" idma_addr_width="32" idma_buf_depth="4" idma_channels="1" idma_data_width="128" idma_max_reqs="32" idma_reorder_buf="0"/>
<debug dataTraps="2" instTraps="2" perfcounters="8" traxMemBytes="512" traxTimestamps="0"/>
<xea3 alt_reset_addr="0x60001000" ints_ext_count="16" ints_max_level="7" ints_sw_count="1" ints_timer_count="3" prim_reset_addr="0x50000000" vecBase1FromPins="0"/>
<sysmem sram_addr="0x60000000" sram_size="0x04000000" srom_addr="0x50000000" srom_size="0x00800000"/>
<mp cc_idmaportmux="0" cc_inboundportdemux="0" cc_interconnect_base="0" cc_reorderbuffer="0" ints_broadcast_pins="0" ints_dedicated="0" ints_intracore_pins="0"/>
</coreisa>
<targets>
<sw sw_abi="windowed" sw_altreset="0" sw_clibs="newlib"/>
<cfg template_op="none"/>
</targets>
</core>
</config_isa>
<tie_file/>
</xtensa_configuration>
</system_file>
