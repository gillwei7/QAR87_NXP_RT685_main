<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system_file>
<xtensa_configuration>
<config_name buildTS="0" isaSavTS="1511339740588" parent_bid="0" parent_tdb_cksm="0" tieSavTS="0" variant="0">PDXNX8</config_name>
<config_isa description="">
<core formatversion="8000" isaversion="NX1.0" locked="0" name="PDXNX8">
<coreisa>
<processors ball="0" ball_addsub8x20p="0" ballap_ioq_expstate="0" ballap_isa="0" no_bbe16_isa="0" pdxnx8="1" visionq6="0" vq6_histogram="0" vq6_hpvfp="0" vq6_spvfp="0"/>
<implementation clockgating="1" scan="1"/>
<mmu immu_bgmap="mpu_nowndw" immu_fgentries="16" mmuchoice="IMMU"/>
<isa arCount="16" brs="1" depbits="1" l32r="l32r" maxCoprocs="7" mul32choice="fullupper"/>
<fetch L0IBuffer="256" btb_entries="128"/>
<systemif sys_mif_data_apb="0" sys_mif_data_apb_addr="0" sys_mif_data_apb_size="0x00010000" sys_sif_data_width="128" sys_sif_inst_width="128"/>
<pqif stdports="1" stdqueues="1"/>
<prefetch dprefetch_entries="8" iprefetch_entries="8"/>
<l2cache l2_ctlregaddr="0" l2_indexlocking="0" l2_numcores="1" l2_resetaddr="0" l2_size="0"/>
<icache ic_size="32768"/>
<sdcache sdc_accesswidth="32" sdc_size="16384"/>
<vdcache vdc_size="0x00040000"/>
<localmems data_latency="3" data_width="256" dram_addr="0x2f780000" dram_banks="4" dram_count="1" dram_size="0x00080000" iram_addr="0x2f800000" iram_count="1" iram_size="0x00010000"/>
<memerrors dside_errors="1" iside_errors="1"/>
<idma idma="0" idma_buf_depth="4" idma_channels="1" idma_max_reqs="16" idma_reorder_buf="0"/>
<debug perfcounters="8" traxMemBytes="8192" traxTimestamps="1"/>
<xea3 alt_reset_addr="0x70000000" ints_ext_count="64" ints_max_level="7" ints_sw_count="2" ints_timer_count="2" prim_reset_addr="0x00100000"/>
<sysmem sram_addr="0x80000000" sram_size="0x10000000" srom_addr="0x00100000" srom_size="0x01000000"/>
</coreisa>
<targets>
<sw sw_abi="call0" sw_altreset="0" sw_clibs="xclib"/>
<uiartifacts/>
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
