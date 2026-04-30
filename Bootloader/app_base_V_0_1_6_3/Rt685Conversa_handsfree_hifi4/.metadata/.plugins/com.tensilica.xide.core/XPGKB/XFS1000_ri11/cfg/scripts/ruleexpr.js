/*
# Copyright (c) 2017-2021 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.
# These coded instructions, statements, and computer programs are the
# copyrighted works and confidential proprietary information of Cadence.
# They may not be modified, copied, reproduced, distributed, or disclosed to
# third parties in any manner, medium, or form, in whole or in part, without
# the prior written consent of Cadence.
 */

/**
 * This java script basically provides flexibility to define complex rule and is invoked 
 * from RuleExpr via the Script Engine.
 * @author tkapil
 */
var ruleexpr = new Object();

/**
 * _dcls_without_lockonly_lockstep
 * return 1 if dcls is not lock only && lockstep is not sleected
 * 
 */
ruleexpr._dcls_without_lockonly_lockstep = function(ctl, args) {
	// args[0] == flexlock
	// args[1] == flexlock_mem_lockstep

	if (args[0] == 1 && args[1] == 1) {
		return 0;
	}

	return 1;
}

/**
 * _dcls_with_lockonly_lockstep
 * return 1 if dcls is  lock only && lockstep
 * 
 */
ruleexpr._dcls_with_lockonly_lockstep = function(ctl, args) {
	// args[0] == flexlock
	// args[1] == flexlock_mem_lockstep

	if (args[0] == 1 && args[1] == 1) {
		return 1;
	}

	return 0;
}

/**
 * _l2_idbits_log2 = log2ceil((numcore+numl3l2)/numslaves)
 * 
 */
ruleexpr._l2_idbits_log2 = function(ctl, args) {
	// args[0] == l2_numcores
	// args[1] == mp_numOfL2ExternalInboundPorts
	// args[2] == l2_numslaves
	return ruleexpr.log2ceillog((args[0] + args[1]) / args[2]);
}

/**
 * max(idWidth-iDMA,idWidth-ExternalL2Slave)+CeilingLog2((numOfCores + numInstances-ExternalL2Slave) / numInstances(L2Slave))
 * 
 */
ruleexpr._l2slave_idwidth = function(ctl, args) {
	// args[0] == idma
	// args[1] == l2_size
	// args[2] == slavetol2idwidth
	// args[3] == l2_numcores
	// args[4] == mp_numOfL2ExternalInboundPorts
	// args[5] == l2_numslaves

	/*
	 * first get the max of idmaidwidth OR slavetol2idwidth
	 */
	var max_idmaIdWidth_slavetol2idwidth = 0;

	if (args[0] > 0 && args[1] > 0) {
		//means idmademux=true
		max_idmaIdWidth_slavetol2idwidth = 4;
	}

	if (max_idmaIdWidth_slavetol2idwidth < args[2]) {
		max_idmaIdWidth_slavetol2idwidth = args[2]
	}

	return max_idmaIdWidth_slavetol2idwidth
			+ ruleexpr.log2ceillog((args[3] + args[4]) / args[5]);
}

ruleexpr._wwdt_entropy = function(ctl, args) {
	// args[0] == ikey
	// args[1] == bkey
	// args[2] == rkey
	// args[3] == kkey
	// args[4] == ekey
	// args[5] == c1key
	// args[6] == c2key
	// args[7] == t1key
	// args[8] == t2key

	var x = 0;
	var i = 0;
	var bits = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
	var bm = 0;
	var bc = 0;
	var wnb = 0;

	for (i = 0; i < args.length; i++) {
		var loga = 0;
		if (args[i] >= 1) {
			loga = Math.log(args[i]);
		}
		var log2 = Math.log(2.0);
		var l2 = Math.ceil(loga / log2);

		//print("log2 args: " + i + " = " + l2);
		if (bits[l2] == 0) {
			bc++;
		}
		bits[l2] = bits[l2] + 1;
		bm = Math.max(bm, bits[l2]);

		var nbits = ruleexpr.countBits(args[i]);
		wnb += 6 - Math.abs(6 - nbits);
	}

	var ent = parseInt(wnb + ((args.length - bm) + bc) * 3);
	return ent;
}

ruleexpr._flexlock_entropy = function(ctl, args) {
	// args[2] == rkey
	// args[4] == ekey
	// args[5] == c1key
	// args[6] == c2key
	// args[7] == t1key
	// args[8] == t2key

	var x = 0;
	var i = 0;
	var bits = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
	var bm = 0;
	var bc = 0;
	var wnb = 0;

	for (i = 0; i < args.length; i++) {
		var loga = 0;
		if (args[i] >= 1) {
			loga = Math.log(args[i]);
		}
		var log2 = Math.log(2.0);
		var l2 = Math.ceil(loga / log2);

		//print("log2 args: " + i + " = " + l2);
		if (bits[l2] == 0) {
			bc++;
		}
		bits[l2] = bits[l2] + 1;
		bm = Math.max(bm, bits[l2]);

		var nbits = ruleexpr.countBits(args[i]);
		wnb += 6 - Math.abs(6 - nbits);
	}

	var ent = parseInt(wnb * 0.5 + ((args.length - bm) + bc) * 7);
	return ent;
}

/**
 * walk the int (32 bits) counting how many are ones
 */
ruleexpr.countBits = function(v) {
	var vv = v;
	var bc = 0;
	var i;
	for (i = 0; i < 32; i++) {
		var thisbit = vv & 1;
		bc += thisbit;
		vv >>= 1;
	}
	//print("Bitcount of: " + v + " = " + bc);
	return bc;

}

/**
 * <b>ACHTUNG: THIS IS really log2ceil - for bit counting</b>
 * <p>
 * Given a long (addr or size) return the lod(2) of that rounded up
 * 
 * e.g. 2=>1, 3=>2, 4=>2, 5=>3 etc
 * 
 * @param num
 *            a long. Behaviour undefined for negative, 0 or even less than 1
 * @return the power-of-2 which is that value or above.
 */
ruleexpr.log2ceillog = function(num) {
	var a = Math.ceil(Math.log(num) / Math.log(2.0));
	var b = a + 0.9999999;
	return parseInt(b);
}

/**
 * <b>ACHTUNG: THIS IS MISNAMED ... it is really log2ceil-po2 because it does
 * the log and then ceilings it and then raises back to po2. it is for sizing up
 * not for bit-counting</b>
 * <p>
 * Given a long (addr or size) return the power-of-2 which is that value or
 * above.
 * 
 * e.g. 3=>4, 4=>4, 5=>8, 7=>8, 8=>8, 9=>16 etc
 * 
 * @param n
 *            a long. Behaviour undefined for negative, 0 or even less than 1
 * @return the power-of-2 which is that value or above.
 */
ruleexpr.log2ceil = function(num) {
	var a = Math.ceil(Math.log(num) / Math.log(2.0));
	var b = Math.pow(2, a) + 0.5;
	return parseInt(b);

}

/**
 * _lx_min_dclinesize = datawidth * nbanks
 *    if dc.size=0
 *       0
 *    else
 *       datawidthBYTES * nbanks
 */
ruleexpr._lx_min_dclinesize = function(ctl, args) {
	// args[0] == dc.size
	// args[1] == datawidthBits
	// args[2] == nbanks
	var mindcls = 0;
	if (args[0] > 0) {
		mindcls = args[1] * args[2] / 8;
	}

	return mindcls;
}

/**
 * _lx_min_dclinesized2 = datawidth * nbanks / 2
 *    if dc.size=0
 *       0
 *    else
 *       datawidthBYTES * nbanks / 2
 *    NOTE: new rule only applies if nbanks > 1
 */
ruleexpr._lx_min_dclinesized2 = function(ctl, args) {
	// args[0] == dc.size
	// args[1] == datawidthBits
	// args[2] == nbanks
	var mindcls = 0;
	if (args[0] > 0) {
		if (args[2] > 1) {
			mindcls = args[1] * args[2] / 8 / 2;
		} else {
			mindcls = args[1] / 8;
		}
	}
	//print("_lx_min_dclinesized2 : " + mindcls);

	return mindcls;
}

/**
 * decides if inbound if configured on local memory
 */
ruleexpr._inbound_memory = function(ctl, args) {
	// args[0] == pifenabled
	if (args[0] > 0) {
		
		var lmems = ctl.getMemoryList([ "lmem" ], false, false, 0);

		for (var i = 0; i < lmems.size(); i++) {
			var cvm = lmems.get(i);
			
			if (cvm.getDma() > 0){
				return 1;
			}
		}
	}
	return 0;
}

/**
 * Inject the values in the controller model just before the build is uploaded
 */
ruleexpr.injectValuesBeforeBuild = function(ctl, cbo) {

//	if (ctl.getIsaFamily() == "LX") {
//		
//		var nne_val = ctl.getLong("nne_engine");
//		var bwisanver = ctl.getLong("bwisanver");
//	
//		if(nne_val>0 && bwisanver>=27100080){
//		
//			var model = ctl.getModel();
//
//			model.modValue("nne_version", 2, false);
//			model.modValue("nne_ipversion", 1, false);
//		}
//	}
}


/**
 *
 *This one is for LX, get the sequence of operations that should be performed 
 *on the config when the passed button is pressed
 */
ruleexpr.getModelOperationsForBtnLX = function(btnName) {

	if (btnName == "create_secure") {

		var dict = {};

		dict["secure_mode"] = "1";
		dict["mmuType"] = "IMMU";
		dict["debugEnabled"] = "1";
		dict["debugAPB"] = "1";
		dict["lmem~instram"] = "sizeBytes~32768,secSize~16384";
		dict["lmem~instrom"] = "sizeBytes~32768,secSize~16384";
		dict["lmem~dataram"] = "sizeBytes~32768,secSize~16384";
		dict["cfg_vector_style"] = "lx7_sec";
		dict["vectorsReloc"] = "0";
		dict["cfg_auto_mmap"] = "1";
		dict["cfg_auto_vectors"] = "1";

		return dict;

	}
	
	if (btnName == "gen_wwdt_keys") {

		return ruleexpr.getRandomWWDTKeys();

	}
	
	if (btnName == "gen_flexlock_keys") {

		return ruleexpr.getRandomFlexlockKeys();

	}

	return null;
}

/**
 *
 *This one is for NX, get the sequence of operations that should be performed 
 *on the config when the passed button is pressed
 */
ruleexpr.getModelOperationsForBtnNX = function(btnName) {
	
	if (btnName == "gen_wwdt_keys") {

		return ruleexpr.getRandomWWDTKeys();

	}
	
	if (btnName == "gen_flexlock_keys") {

		return ruleexpr.getRandomFlexlockKeys();

	}

	return null;
}

/**
 * 
 * Random wwdt keys for the dynamic action buttons (LX/NX).
 */
ruleexpr.getRandomWWDTKeys = function() {
	
	var keys = ruleexpr.getRandomKeys(9);
	var dict = {};
	var index=0;
	
	dict["wwdt_ikey"] =  keys[index++];
	dict["wwdt_bkey"] =  keys[index++];
	dict["wwdt_rkey"] =  keys[index++];
	dict["wwdt_kkey"] =  keys[index++];
	dict["wwdt_ekey"] =  keys[index++];
	dict["wwdt_c1key"] = keys[index++];
	dict["wwdt_c2key"] = keys[index++];
	dict["wwdt_t1key"] = keys[index++];
	dict["wwdt_t2key"] = keys[index++];
		
	return dict;
}

/**
 * 
 * Random wwdt keys for the dynamic action buttons (LX/NX).
 */
ruleexpr.getRandomFlexlockKeys = function() {
	
	var keys = ruleexpr.getRandomKeys(6);	
	var dict = {};
	var index=0;
	
	dict["flexlock_rkey"] =  keys[index++];
	dict["flexlock_ekey"] =  keys[index++];
	dict["flexlock_c1key"] =  keys[index++];
	dict["flexlock_c2key"] =  keys[index++];
	dict["flexlock_t1key"] =  keys[index++];
	dict["flexlock_t2key"] = keys[index++];
		
	return dict;
}

/**
 * get the random 12bit keys
 */
ruleexpr.getRandomKeys = function(numKeys) {
	
	var keys = [];
	
	for (var i = 0 ; i < numKeys ; i++)
	{
		var random = Math.random();
		random = random * 4095;		
		keys[i] = parseInt(random)+"";
	}
	
	return keys;
}

/**
 *
 *Do the config checks and add them to the msgs.
 */
ruleexpr.doJsRuleChecks = function(op, dep, ctl, msgs) {

	if (ctl.getIsaFamily() == "LX") {

		configRules.doJsRuleChecksLX(op, dep, ctl, msgs);
		
	} else if (ctl.getIsaFamily() == "NX") {

		configRules.doJsRuleChecksNX(op, dep, ctl, msgs);
		
	} 
}

/**
 * during import/upgrade check and return a map of any explicit fixups if applicable
 */
ruleexpr.importUpgradeConfigOptions = function(ctl) {
	
	if (ctl.getIsaFamily() == "LX") {
			
		if (ctl.getIsaVersion() == "LX8") {			
			// import or upgrade is to LX8.
			var dict = {};
			
			// first things is remove the template_op (guides) and editor
			// release if they exist!
			dict["template_op"] = "none";
			dict["editor_rel"] = "none";
			
			if(ctl.getString("pso.domains") != "None"){
				// if the controller had pso.domain, then post upgrade set
				// cc_psodomain
				dict["cc_psodomain"] = "1";
			}	
			
			if(ctl.getString("tempBus") == "axi4" && ctl.getInt("pifAsync") > 0){
				// if the controller has async pif in LX7, on upgrade enable
				// asyncClock
				dict["asyncClock"] = "1";
			}

			if(ctl.getInt("ivp26_32") > 0 && ctl.hasFeature("lx_visionp_512")){	
				// DO THIS ONLY IF YOU HAVE THE NEW FEATURE.
				
				// the feature for v130 and vp6 quadmac stays the same across
				// lx7 and lx8
				var has_ivp26_quadmac_rg4 = ctl.hasFeature("ivp26_quadmac_rg4");

					
					dict["visionp_512"] = "1";
					
					dict["vp_512_fourbit_mac"] = ctl.getString("ivp26_fourbit_mac");
					dict["vp_512_histogram"] = ctl.getString("ivp26_histogram");
					
					dict["vp_512_hpvfp"] = ctl.getString("ivp26_hpvfp");
					dict["vp_512_spvfp"] = ctl.getString("ivp26_spvfp");
					
					if(has_ivp26_quadmac_rg4){
						// if you have feature then leave value as is
						dict["vp_512_quadmac"] = ctl.getString("ivp26_quadmac");
					}else{
						// no feature, turn it 1
						dict["vp_512_quadmac"] = "1";
					}
					
			}
			
			// now same with vision p1, convert to v110
			if(ctl.getInt("visionp1") > 0 && ctl.hasFeature("lx_visionp_128")){	
				
				dict["visionp_128"] = "1";
				var vp_128_mac_options = (ctl.getInt("vp1_paired_mac") * 3 ) + (ctl.getInt("vp1_dualquad_mac_8x8") * 4 ) + (ctl.getInt("vp1_quad_mac_16x16") * 8 );
				dict["vp_128_mac_options"] = vp_128_mac_options.toString();
				dict["vp_128_fourbit_mac"] = ctl.getString("visionp1_fourbit_mac");
				dict["vp_128_hpvfp"] = ctl.getString("vp1_hp_vfp");
				dict["vp_128_spvfp"] = ctl.getString("vp1_sp_vfp");
				dict["vp_128_vecreggroup"] = ctl.getString("vp1_vecreggroup");
			
			}
			
			return dict;
		}
	}
	return null;
}
