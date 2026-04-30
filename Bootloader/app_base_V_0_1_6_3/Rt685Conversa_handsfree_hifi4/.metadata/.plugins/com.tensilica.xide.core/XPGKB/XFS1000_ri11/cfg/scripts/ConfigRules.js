/*
# Copyright (c) 2017-2021 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
# These coded instructions, statements, and computer programs are the
# copyrighted works and confidential proprietary information of Cadence.
# They may not be modified, copied, reproduced, distributed, or disclosed to
# third parties in any manner, medium, or form, in whole or in part, without
# the prior written consent of Cadence.
 */

/**
 * JS that contains all the config rules.
 * 
 * @author tkapil  
 */
var configRules = new Object();

/**
 * LX config rules
 */
configRules.doJsRuleChecksLX = function(op, dep, ctl, msgs) {

	if (op == "secure_mode") {

		configRules.secureModeRules(op, dep, ctl, msgs);

	}

	if (op == "mmuType") {

		if (dep == "lmem~smem~udma") {

			var isMmu = "LinuxMMU".equals(ctl.getModel().getItem("mmuType")
					.toString());

			if (isMmu == true) {
				configRules.linuxMmuMemPlacementRules(op, dep, ctl, msgs);
			}
		}
	}

	if (op == "wwdt") {

		configRules.doWWDTRules(op, dep, ctl, msgs);

	}
	if (op == "etie_enable") {
	
		if (dep == "int") {
			
			// evaluate rule only if etie option or interrupts are touched
			configRules.doEtieRules(op, dep, ctl, msgs);		
		}
	}
		

	if (op == "nna_engine" || op == "nne_engine") {
		
		if (dep == "int~pifWriteResp") {
			configRules.doNNA_EInterruptRules(op, dep, ctl, msgs);
		}

	}
	
	if (op == "udma_channels") {
		
		if (dep == "int~udma") {
			
			// idma interrupts counting
			configRules.doIdmaInterrupts(op, dep, ctl, msgs);		
		}
	}	
	
	if (op == "intTimerCount") {
		
		if (dep == "int") {
			
			// evaluate the timer interrupts
			configRules.doTimerInterrupts(op, dep, ctl, msgs);		
		}
	}		
}

/**
 * rules for timer interrupts, the count of timers should match the timer
 * interrupts.
 */
configRules.doTimerInterrupts = function(op, dep, ctl, msgs) {

	var timerCount = ctl
			.getInt(com.tensilica.xide.core.cfg.mccfg.CoreIsaDefs.INT_TIMER_COUNT);

	if (timerCount == 0) {
		return;
	}

	var nints = ctl
			.getInt(com.tensilica.xide.core.cfg.mccfg.CoreIsaDefs.INT_COUNT);
	var numTimerInts = 0;

	for (var i = 0; i < nints; i++) {

		var iname = com.tensilica.xide.core.cfg.mc.MCModelUtils
				.makeIndexedName("int", i);
		var model = ctl.getModel();
		var cfgValueInterrupt = model.getItem(iname);

		if (cfgValueInterrupt.getTypeId().startsWith("timer")) {
			numTimerInts++;

			if (numTimerInts == timerCount) {
				break;
			}
		}
	}

	if (numTimerInts != timerCount) {

		var formatMsg = msgs
				.format(
						"Exactly %1$s 'Timer' interrupt(s) need to be configured. Currently configured: %2$s",
						timerCount, numTimerInts);

		msgs.addMsg(formatMsg);
	}
}

/**
 * rules for idma done/err interrupts (LX). the number of idma interrupts should
 * match the idma channels
 */
configRules.doIdmaInterrupts = function(op, dep, ctl, msgs) {

	var idma = ctl.getInt(com.tensilica.xide.core.cfg.mccfg.CoreIsaDefs.UDMA);

	if (idma == 0 || !ctl.isLX8()) {
		return;
	}

	var nints = ctl
			.getInt(com.tensilica.xide.core.cfg.mccfg.CoreIsaDefs.INT_COUNT);
	var numIdmaChannels = ctl
			.getInt(com.tensilica.xide.core.cfg.mccfg.CoreIsaDefs.UDMA_CHANNELS);
	var numIdmaErr = 0;
	var numIdmaDone = 0;

	var firstIdmaDone = -1;
	var lastIdmaDone = -1;

	var firstIdmaErr = -1;
	var lastIdmaErr = -1;

	for (var i = 0; i < nints; i++) {

		var iname = com.tensilica.xide.core.cfg.mc.MCModelUtils
				.makeIndexedName("int", i);
		var model = ctl.getModel();
		var cfgValueInterrupt = model.getItem(iname);

		if ("UDmaDone".equals(cfgValueInterrupt.getTypeId())) {
			numIdmaDone++;

			if (firstIdmaDone == -1) {
				firstIdmaDone = i;
			}
			lastIdmaDone = i;
		}

		if ("UDmaErr".equals(cfgValueInterrupt.getTypeId())) {
			numIdmaErr++;

			if (firstIdmaErr == -1) {
				firstIdmaErr = i;
			}
			lastIdmaErr = i;
		}
	}

	var checkIntSequence = true;

	if (numIdmaDone != numIdmaChannels) {

		var formatMsg = msgs
				.format(
						"Exactly %1$s 'iDmaDone' interrupt(s) need to be configured(1 per idma channel). Currently configured: %2$s",
						numIdmaChannels, numIdmaDone);

		msgs.addMsg(formatMsg);

		checkIntSequence = false;
	}

	if (numIdmaErr != numIdmaChannels) {

		var formatMsg = msgs
				.format(
						"Exactly %1$s 'iDmaErr' interrupt(s) need to be configured(1 per idma channel). Currently configured: %2$s",
						numIdmaChannels, numIdmaErr);

		msgs.addMsg(formatMsg);

		checkIntSequence = false;
	}

	if (checkIntSequence && numIdmaChannels > 1) {
		// check sequence only if correct number of interrupts are configured
		if (lastIdmaDone != (firstIdmaDone + numIdmaChannels - 1)) {

			var formatMsg = msgs
					.format(
							"The %1$s 'iDmaDone' interrupts must be defined sequentially one after the other.",
							numIdmaChannels);
			msgs.addMsg(formatMsg);

		}

		if (lastIdmaErr != (firstIdmaErr + numIdmaChannels - 1)) {

			var formatMsg = msgs
					.format(
							"The %1$s 'iDmaErr' interrupts must be defined sequentially one after the other.",
							numIdmaChannels);
			msgs.addMsg(formatMsg);
		}

		if (firstIdmaErr != (lastIdmaDone + 1)) {

			var formatMsg = msgs
					.format(
							"The %1$s 'iDmaErr' interrupts must immediately follow the last 'iDmaDone' interrupt (i.e. interrupt: %2$s).",
							numIdmaChannels, lastIdmaDone);
			msgs.addMsg(formatMsg);
		}
	}
}

/**
 * rules for NNA NNE write error interrupt
 */
configRules.doNNA_EInterruptRules = function(op, dep, ctl, msgs) {
	
	var nna_e_val = ctl.getLong(op);
	var pifWriteResp = ctl.getLong("pifWriteResp");
	
	if(nna_e_val > 0 && pifWriteResp > 0){
		
		var idx = ctl.findInterrupt("writeerr", -1);
		
		if(idx < 0){
			
			msgs.addMsg("PIF Write Responses are configured - a Write Error interrupt is required in order to handle events(1)");
		}	
	}	
}


/**
 * rules for etie interrupts (LX) -- NX is done in yaml
 */
configRules.doEtieRules = function(op, dep, ctl, msgs) {
	
	var nints = ctl.getInt(com.tensilica.xide.core.cfg.mccfg.CoreIsaDefs.INT_COUNT);
	var etieIntCount = 0;
	
	for (var i = 0 ; i < nints ; i++){
			
		var iname = com.tensilica.xide.core.cfg.mc.MCModelUtils.makeIndexedName("int", i);
		var model = ctl.getModel();
		var cfgValueInterrupt = model.getItem(iname);
		
		if("etie".equals(cfgValueInterrupt.getTypeId())){
			etieIntCount++;
		}
		
		if(etieIntCount>8){
			break;
		}
	}
	
	if(etieIntCount>8){

			msgs.addMsg("Maximum 8 eTIE interrupts can be configured.");		
	}
}

/**
 * rules for watchdog timer (both LX and NX)
 */
configRules.doWWDTRules = function(op, dep, ctl, msgs) {

	var model = ctl.getModel();

	if (model.getItem("wwdt").getInt() > 0) {

		if (dep == "wwdt_reset_val" && ctl.getLong("wwdt_reset_val") > 0) {

			var val = ctl.getValueStr(model.getItem("wwdt_reset_val"), model
					.getItem("wwdt_reset_val").toString());

			var formatMsg = msgs
					.format(
							"N:%2$s %1$s gets you actual cycle count of '%1$sfff' cycles",
							val, ctl.getOptName("wwdt_reset_val"));

			msgs.addMsg(formatMsg);
		}

		if (dep == "wwdt_hb_reset_val") {

			var val = ctl.getValueStr(model.getItem("wwdt_hb_reset_val"), model
					.getItem("wwdt_hb_reset_val").toString());

			var formatMsg = msgs
					.format(
							"N:%2$s %1$s gets you actual cycle count of '%1$sfff' cycles",
							val, ctl.getOptName("wwdt_hb_reset_val"));

			msgs.addMsg(formatMsg);

		}
	}
}

/**
 * Validate the local and system memory placements when linux mmu
 */
configRules.linuxMmuMemPlacementRules = function(op, dep, ctl, msgs) {

	// 0x0-0x7FF_FFFF (l/s mems)
	var x0__x7FF_FFFF = new com.tensilica.xide.core.cfg.util.AddressRange(0,
			134217728);

	// 0x2000_0000-0x3FFF_FFFF (only for local memories)
	var x2000_0000__x3FFF_FFFF = new com.tensilica.xide.core.cfg.util.AddressRange(
			536870912, 536870912);

	// 0xF000_0000-0xFFFF_FFFF (l/s mems)
	var xF000_0000__xFFFF_FFFF = new com.tensilica.xide.core.cfg.util.AddressRange(
			4026531840, 268435456);

	var lmems = ctl.getMemoryList([ "lmem" ], false, false, 0);

	for (var i = 0; i < lmems.size(); i++) {
		var cvm = lmems.get(i);

		var lmemAddressRange = cvm.getAddressRange();

		if (!(x0__x7FF_FFFF.fullyContains(lmemAddressRange)
				|| x2000_0000__x3FFF_FFFF.fullyContains(lmemAddressRange) || xF000_0000__xFFFF_FFFF
				.fullyContains(lmemAddressRange))) {

			var formatMsg = msgs
					.format(
							"Full MMU with TLB requires that local memory '%1$s' must be placed in one of these regions: (%2$s), (%3$s), (%4$s)",
							ctl.getOptName(cvm.getName()), x0__x7FF_FFFF
									.toAddressString(), x2000_0000__x3FFF_FFFF
									.toAddressString(), xF000_0000__xFFFF_FFFF
									.toAddressString());
			msgs.addMsg(formatMsg, [ cvm.getName() ]);

		}

		if (ctl.getInt("udma") > 0 && cvm.getMemName().startsWith("dataram")) {
			// with idma data ram can be placed only in these 2 regions
			if (!(x0__x7FF_FFFF.fullyContains(lmemAddressRange) || xF000_0000__xFFFF_FFFF
					.fullyContains(lmemAddressRange))) {

				var formatMsg = msgs
						.format(
								"Full MMU with TLB with iDMA requires that local memory '%1$s' must be placed in one of these regions: (%2$s), (%3$s)",
								ctl.getOptName(cvm.getName()), x0__x7FF_FFFF
										.toAddressString(),
								xF000_0000__xFFFF_FFFF.toAddressString());
				msgs.addMsg(formatMsg, [ cvm.getName() ]);

			}
		}
	}

	var cvm = ctl.findMemory("sysrom.0");

	if (cvm != null) {

		var systemRomAddRange = cvm.getAddressRange();

		if (systemRomAddRange != null) {
			if (!(x0__x7FF_FFFF.fullyContains(systemRomAddRange) || xF000_0000__xFFFF_FFFF
					.fullyContains(systemRomAddRange))) {

				var formatMsg = msgs
						.format(
								"Full MMU with TLB requires that sys memory '%1$s' must be placed in one of these regions: (%2$s), (%3$s)",
								ctl.getOptName(cvm.getName()), x0__x7FF_FFFF
										.toAddressString(),
								xF000_0000__xFFFF_FFFF.toAddressString());
				msgs.addMsg(formatMsg, [ cvm.getName() ]);
			}
		}
	}
}

configRules.secureModeRules = function(op, dep, ctl, msgs) {

	var model = ctl.getModel();

	if (model.getItem("secure_mode").getInt() > 0) {

		if (dep == "smem") {
			configRules.secureMemSizeCheck(ctl, msgs, ctl
					.getMemoryByName("sysrom.0"), false);
			configRules.secureMemSizeCheck(ctl, msgs, ctl
					.getMemoryByName("sysram.0"), false);
		}

		if (dep == "lmem") {
			configRules.secLocalMemCheck(ctl, msgs);
		}
	}
}

configRules.secLocalMemCheck = function(ctl, msgs) {

	// check data ram
	var dataRams = ctl.getLocalMemList("dataram");

	if (dataRams == null || dataRams.size() == 0) {

		msgs.addMsg("Secure mode requires that a secure data ram is defined");

	} else {

		var dram0 = dataRams.get(0);
		var dram1 = null;

		if (dataRams.size() > 1) {
			dram1 = dataRams.get(1);
		}

		if (dram1 != null) {

			if ((dram0.getSecSize() == 0 && dram1.getSecSize() == 0)
					|| (dram0.getSecSize() > 0 && dram1.getSecSize() > 0)
					|| (dram0.getSecSize() > 0 && msgs.compare(
							dram0.getPAddr(), dram1.getPAddr()) < 0)
					|| (dram1.getSecSize() > 0 && msgs.compare(
							dram0.getPAddr(), dram1.getPAddr()) > 0)) {

				msgs
						.addMsg(
								"When 2 Data RAMs are present, secure area must be defined on the RAM at the higher address",
								[ dram0.getName(), dram1.getName() ]);

			} else {
				if (dram0.getSecSize() > 0) {
					configRules.secureMemSizeCheck(ctl, msgs, dram0, true);				
				}

				if (dram1.getSecSize() > 0) {
					configRules.secureMemSizeCheck(ctl, msgs, dram1, true);
				}
			}
		} else {
			configRules.secureMemSizeCheck(ctl, msgs, dram0, true);
			configRules.singleLmemSecureSize(ctl, msgs, dram0);
		}
	}

	// check data rom

	var dataRom = ctl.getLocalMemList("datarom");

	if (dataRom != null && dataRom.size() > 0
			&& dataRom.get(0).getSecSize() > 0) {
		configRules.secureMemSizeCheck(ctl, msgs, dataRom.get(0), true);
	}

	// check inst ram/rom
	var irams = ctl.getLocalMemList("instram");

	var iroms = ctl.getLocalMemList("instrom");

	if ((irams == null || irams.size() == 0)
			&& (iroms == null || iroms.size() == 0)) {

		msgs
				.addMsg("Secure mode requires that atleast secure iram or irom is defined");

	} else {

		var hasSecureIrom = false;
		var hasSecureIram = false;

		if (iroms != null && iroms.size() > 0 && iroms.get(0).getSecSize() > 0) {
			hasSecureIrom = true;
			configRules.secureMemSizeCheck(ctl, msgs, iroms.get(0), true);
		}

		if (irams != null && irams.size() > 0) {

			var iram0 = irams.get(0);
			var iram1 = null;

			if (irams.size() > 1) {
				iram1 = irams.get(1);
			}

			if (iram1 != null) {

				if ((iram0.getSecSize() == 0 && iram1.getSecSize() == 0 && hasSecureIrom == false)
						|| (iram0.getSecSize() > 0 && iram1.getSecSize() > 0)
						|| (iram0.getSecSize() > 0 && msgs.compare(iram0
								.getPAddr(), iram1.getPAddr()) < 0)
						|| (iram1.getSecSize() > 0 && msgs.compare(iram0
								.getPAddr(), iram1.getPAddr()) > 0)) {

					msgs
							.addMsg(
									"When 2 Instruction RAMs are present, secure area must be defined on the RAM at the higher address",
									[ iram0.getName(), iram1.getName() ]);

				} else {

					if (iram1.getSecSize() > 0) {
						hasSecureIram = true;
						configRules.secureMemSizeCheck(ctl, msgs, iram1, true);
					}
				}
			}

			if (iram0.getSecSize() > 0) {
				hasSecureIram = true;
				configRules.secureMemSizeCheck(ctl, msgs, iram0, true);
				configRules.singleLmemSecureSize(ctl, msgs, iram0);
			}
		}

		if (hasSecureIrom == false && hasSecureIram == false) {
			msgs
					.addMsg("Secure mode requires that atleast secure iram or irom is defined");
		}
	}
}

/**
 * Secure memory size for local ram when only 1 ram is defined
 */
configRules.singleLmemSecureSize = function(ctl, msgs, mem) {

	if (mem != null) {

		if (mem.getSecSize() > (mem.getSizeBytes()/2)) {

			msgs.addMsg("When only one RAM is defined, the secure size cannot exceed ram-size/2 ("+ctl.getOptName(mem.getName())+")", [ mem
					.getName() ]);
		}

	}
}

/**
 * Secure memory size checks for local and system secure memory
 */
configRules.secureMemSizeCheck = function(ctl, msgs, mem, islocal) {

	if (mem != null) {

		if (islocal == true) {

			if (mem.getSecSize() < 16384) {

				msgs.addMsg(ctl.getOptName(mem.getName())
						+ " secure size must be >= 16K (i.e. 0x4000)", [ mem
						.getName() ]);
			}

		} else {

			if (mem.getSecSize() > 0 && mem.getSecSize() < 32768) {

				msgs.addMsg(ctl.getOptName(mem.getName())
						+ " secure size must be >= 32K (i.e. 0x8000)", [ mem
						.getName() ]);
			}
			
			if (mem.getSecSize() > (mem.getSizeBytes()/2)) {

				msgs.addMsg(ctl.getOptName(mem.getName())
						+ " secure size must be <= half its size.", [ mem
						.getName() ]);
			}
			
		}

		if (msgs.compare(mem.getSecSize(), mem.getSizeBytes()) > 0) {

			msgs.addMsg(ctl.getOptName(mem.getName())
					+ " secure size cannot exceed its own size", [ mem
					.getName() ]);
		}
	}
}

/**
 * calculate the max interrupts that can be configured
 */
configRules.doMaxExtInterruptRules = function(op, dep, ctl, msgs) {

	var model = ctl.getModel();

	var ints_ext_count = model.getItem("ints_ext_count").getInt();
	var ints_sw_count = model.getItem("ints_sw_count").getInt();
	var ints_timer_count = model.getItem("ints_timer_count").getInt();
	var ints_etie_count = model.getItem("ints_etie_count").getInt();
	// 2 per idma channel
	var ints_idma_channel_count = model.getItem("idma").getInt() > 0 ? (model
			.getItem("idma_channels").getInt() * 2) : 0;
	var ints_wwdt_interrupts_count = model.getItem("wwdt").getInt() > 0 ? 1 : 0;
	var ints_flexlock_interrupts_count = model.getItem("flexlock").getInt() > 0 ? 1
			: 0;
	// 2 if l2 configured
	var ints_l2_interrupts_count = model.getItem("l2_size").getInt() > 0 ? 2
			: 0;

	var totalInterruptCount = ints_ext_count + ints_sw_count + ints_timer_count
			+ ints_etie_count + ints_idma_channel_count
			+ ints_wwdt_interrupts_count + ints_flexlock_interrupts_count
			+ ints_l2_interrupts_count;

	if (totalInterruptCount > 248) {
		var formatMsg = msgs
				.format(
						"Total Number of interrupts cannot exceed 248. (current count=%9$s ie: external=%1$s, software=%2$s, timers=%3$s, etie=%4$s, idma=%5$s, watchdog=%6$s, splitlock=%7$s, l2=%8$s)",
						ints_ext_count, ints_sw_count, ints_timer_count,
						ints_etie_count, ints_idma_channel_count,
						ints_wwdt_interrupts_count,
						ints_flexlock_interrupts_count,
						ints_l2_interrupts_count,
						totalInterruptCount);
		msgs.addMsg(formatMsg);
	}
}

/**
 * 
 * NX config rules
 */
configRules.doJsRuleChecksNX = function(op, dep, ctl, msgs) {

	if (op == "wwdt") {

		configRules.doWWDTRules(op, dep, ctl, msgs);

	}
	
	if (op == "ints_ext_count") {

		configRules.doMaxExtInterruptRules(op, dep, ctl, msgs);

	}	

}
