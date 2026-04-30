<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system_file>
<xtensa_configuration>
<config_name buildTS="0" isaSavTS="1512109965141" parent_bid="0" parent_tdb_cksm="0" tieSavTS="0" variant="0">XRC_Vision_Q6_AO</config_name>
<config_isa description="">
<core formatversion="8000" isaversion="NX1.0" locked="0" name="XRC_Vision_Q6_AO">
<coreisa>
<processors ball="0" ball_addsub8x20p="0" ballap_ioq_expstate="0" ballap_isa="0" no_bbe16_isa="0" pdxnx8="0" visionq6="1" vq6_histogram="1" vq6_hpvfp="1" vq6_spvfp="1"/>
<implementation clockgating="1" scan="1"/>
<mmu immu_bgmap="mpu_ca_4kpg" immu_fgentries="16" mmuchoice="IMMU"/>
<isa arCount="32" brs="0" depbits="1" l32r="l32r_const16" maxCoprocs="7" mul32choice="fullupper"/>
<fetch L0IBuffer="64" btb_entries="128"/>
<systemif sys_mif_data_apb="0" sys_mif_data_apb_addr="0" sys_mif_data_apb_size="0x00010000" sys_sif_data_width="64" sys_sif_inst_width="0"/>
<pqif stdports="0" stdqueues="0"/>
<prefetch dprefetch_entries="0" iprefetch_entries="0"/>
<l2cache l2_ctlregaddr="0" l2_indexlocking="0" l2_numcores="1" l2_resetaddr="0" l2_size="0"/>
<icache ic_size="0x00010000"/>
<sdcache sdc_accesswidth="32" sdc_size="16384"/>
<vdcache vdc_size="0"/>
<localmems data_latency="5" data_width="512" dram_addr="0x3ffc0000" dram_banks="2" dram_count="2" dram_size="0x00020000" iram_addr="0" iram_count="0" iram_size="0x00020000"/>
<memerrors dside_errors="0" iside_errors="0"/>
<idma idma="1" idma_buf_depth="4" idma_channels="2" idma_max_reqs="32" idma_reorder_buf="0"/>
<debug perfcounters="8" traxMemBytes="8192" traxTimestamps="1"/>
<xea3 alt_reset_addr="0x7ff40000" ints_ext_count="25" ints_max_level="7" ints_sw_count="3" ints_timer_count="3" prim_reset_addr="0x50000000"/>
<sysmem sram_addr="0x60000000" sram_size="0x20000000" srom_addr="0x50000000" srom_size="0x01000000"/>
</coreisa>
<targets>
<sw sw_abi="windowed" sw_altreset="0" sw_clibs="xclib"/>
<uiartifacts/>
</targets>
</core>
</config_isa>
<tie_file name="" shared="0"/>
</xtensa_configuration>
</system_file>
