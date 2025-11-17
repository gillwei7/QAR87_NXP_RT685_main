//
// Copyright (c) 2021 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.
//

import java.util.List;

import org.ini4j.Profile.Section;
import org.ini4j.Wini;

import java.util.HashMap;
import java.io.File;

import com.tensilica.xide.core.cfg.util.AddressRange;
import other.xide.external.libutils.NumUtils;
import com.tensilica.xide.xnnc.core.util.XNNCUtil;
import com.tensilica.xide.xnnc.core.validator.XnncGroovyValidator;


class XnncValidator
{
	static void validate(Wini iniFileContents , XnncGroovyValidator messages)
	{
		validateMultiCoreAttributes(iniFileContents , messages);
	}
	
	static void validateMultiCoreAttributes(Wini iniFileContents , XnncGroovyValidator messages)
	{
		String coreName = iniFileContents.get("system", "system-xtensa_core");
		Section multiCoreSection = iniFileContents.get("multicore");
		
		if(coreName == null || multiCoreSection == null)
			return;
		
		long sysRamAddress;
		
		long sysRamSize;
		
		long sysRomAddress;
		
		long sysRomSize;
		
		long globalStartAddress;
		
		long globalAddressSpacing;
		
		String defaultGlobalStartAddress = "0xf0000000";
		
		String defaultGlobalAddressSpacing = "1M";
		
		def addressRangesToCheck = [] ;
		
		String romStartAddress = multiCoreSection.get("multicore-sysrom_start_addr");
		if(romStartAddress == null || romStartAddress.isEmpty())
			sysRomAddress = NumUtils.safeToLong(XNNCUtil.getCoreSystemRom(coreName));
		else
			sysRomAddress = NumUtils.safeToLong(multiCoreSection.get("multicore-sysrom_start_addr"));

		//ROM Size
		String romSize = multiCoreSection.get("multicore-sysrom_size");
		if(romSize == null || romSize.isEmpty())
			sysRomSize = NumUtils.safeToLong(XNNCUtil.getCoreSystemRomSize(coreName));
		else
			sysRomSize = NumUtils.safeToLong(multiCoreSection.get("multicore-sysrom_size"));
		
		//RAM Start Address
		String ramStartAddress = multiCoreSection.get("multicore-sysram_start_addr");
		if(ramStartAddress == null || ramStartAddress.isEmpty())
			sysRamAddress = NumUtils.safeToLong(XNNCUtil.getCoreSystemRam(coreName));
		else
			sysRamAddress = NumUtils.safeToLong(multiCoreSection.get("multicore-sysram_start_addr"));
		
		//RAM Size
		String ramSize = multiCoreSection.get("multicore-sysram_size");
		if(ramSize == null || ramSize.isEmpty())
			sysRamSize = NumUtils.safeToLong(XNNCUtil.getCoreSystemRamSize(coreName));
		else
			sysRamSize = NumUtils.safeToLong(multiCoreSection.get("multicore-sysram_size"));

		//Global start address
		String globalStartAddr = multiCoreSection.get("multicore-global_start_addr");
		if(globalStartAddr == null || globalStartAddr.isEmpty())
			globalStartAddress = NumUtils.safeToLong(defaultGlobalStartAddress);
		else
			globalStartAddress = NumUtils.safeToLong(multiCoreSection.get("multicore-global_start_addr"));

		//Global address spacing
		String addrSpacing = multiCoreSection.get("multicore-global_addr_spacing");
		if(addrSpacing == null || addrSpacing.isEmpty())
			globalAddressSpacing = NumUtils.safeToLong(defaultGlobalAddressSpacing);
		else
			globalAddressSpacing = NumUtils.safeToLong(multiCoreSection.get("multicore-global_addr_spacing"));
		
		AddressRange romAddressRange = new AddressRange(sysRomAddress, sysRomSize);
		romAddressRange.setDescription("ROM Address Range");
		addressRangesToCheck.add(romAddressRange);
		
		AddressRange ramAddressRange = new AddressRange(sysRamAddress, sysRamSize);
		ramAddressRange.setDescription("RAM Address Range");
		addressRangesToCheck.add(ramAddressRange);
		
		//Number of cores as mentioned in the cfg file
		String noCores = multiCoreSection.get("multicore-sym_multicore");
		int coreCount = 1;
		
		if(noCores != null && !noCores.isEmpty())
		{
			coreCount = Integer.parseInt(noCores);
		}
		
		AddressRange globalAddressRange = XNNCUtil.getGlobalAddressRange(globalStartAddress, globalAddressSpacing, coreName, coreCount);
		if(globalAddressRange != null)
		{
			globalAddressRange.setDescription("Global Address Range");
			addressRangesToCheck.add(globalAddressRange);
		}
		
		//Number of cores in the config. This will tell us if this is NX.
		int numCores = XNNCUtil.getNumberOfCores(coreName);
		if(numCores > 1)
			generateNXAddressRanges(coreName , addressRangesToCheck);
		
		checkRamConflicts(ramAddressRange, addressRangesToCheck, messages);
		checkRomConflicts(romAddressRange, addressRangesToCheck, messages);
		
		if(globalAddressRange != null)
		{
			checkGlobalAddressConflicts(globalAddressRange , addressRangesToCheck , messages);
		}
	}
	
	static void checkRomConflicts(AddressRange romAddressRange , List<AddressRange> addressRangesToCheck , XnncGroovyValidator messages)
	{
		String errMessage = "err : attr:(multicore-sysrom_start_addr)";
		if(romAddressRange != null)
		{
			for(addr in addressRangesToCheck)
			{
				if(addr.contains(romAddressRange) && !addr.equals(romAddressRange)){
					errMessage += "ROM address space conflicts with " + addr.getDescription();
					messages.addMessage(errMessage);
				}
					
			}
		}
	}
	
	static void checkRamConflicts(AddressRange ramAddressRange , List<AddressRange> addressRangesToCheck , XnncGroovyValidator messages)
	{
		String errMessage = "err : attr:(multicore-sysram_start_addr)";
		if(ramAddressRange != null){
			for(addr in addressRangesToCheck)
			{
				if(addr.contains(ramAddressRange) && !addr.equals(ramAddressRange))
				{
					errMessage += "RAM address space conflicts with " + addr.getDescription();
					messages.addMessage(errMessage);
				}
					
			}
		}
	}
	
	static void checkGlobalAddressConflicts(AddressRange globalAddressRange , List<AddressRange> addressRangesToCheck , XnncGroovyValidator messages)
	{
		String errMessage = "err : attr:(multicore-global_addr_spacing)";
		if(globalAddressRange!=null)
		{
			for(AddressRange addr : addressRangesToCheck)
			{
				if(addr.contains(globalAddressRange) && addr != globalAddressRange)
					errMessage += "Global address space conflicts with " + addr.getDescription();
			}
		}
		
	}
	
	static void generateNXAddressRanges(String coreName , List<AddressRange> addressRangesToCheck)
	{
		
		//APB address range
		AddressRange apbAddressRange = XNNCUtil.getAPBAddressRange(coreName);
		if(apbAddressRange != null)
		{
			apbAddressRange.setDescription("APB Address Range");
			addressRangesToCheck.add(apbAddressRange);
		}

		//L2 address Range
		AddressRange l2CacheAddressRange = XNNCUtil.getL2AddressRange(coreName);
		if(l2CacheAddressRange != null)
		{
			l2CacheAddressRange.setDescription("L2 Cache Address Range");
			addressRangesToCheck.add(l2CacheAddressRange);
		}
			
		
		//All local memory address ranges.
		int count = 0;
		for(AddressRange ladr : XNNCUtil.getAllLocalMemoryAddressRanges(coreName))
		{
			ladr.setDescription("Local Memory "+ count);
			addressRangesToCheck.add(ladr);
			count ++;
		}
	}
	
	static void imageListWizardValidations(HashMap<String , String> expressionMap , XnncGroovyValidator validator)
	{
		String value = expressionMap.get("image_dir");

		if(value != null && value != "")
		{
			String img_count_str = expressionMap.get("num_images");
			if(img_count_str != null && img_count_str != "")
			{
				int img_count = img_count_str.toInteger();
				String extension = expressionMap.get("image_format");
				File file = new File(value);
				
				int count = 0;
				
				if(file.isDirectory())
				{
					file.eachFile()
						{
	    					if (it.name.endsWith(extension)) 
	    					{
	    				        count = count + 1;
	    				    }					
						}
				}
				
				if(img_count > count)
				{
					validator.addMessage("err: The image directory only has $count $extension images.");
				}
			}
		}
		
		String gt_value = expressionMap.get("gt_image_dir");

		if(gt_value != null && gt_value != "")
		{
			String img_count_str = expressionMap.get("num_images");
			if(img_count_str != null && img_count_str != "")
			{
				int img_count = img_count_str.toInteger();
				String extension = expressionMap.get("gt_image_format");
				File file = new File(gt_value);
				
				int count = 0;
				
				if(file.isDirectory())
				{
					file.eachFile()
						{
	    					if (it.name.endsWith(extension)) 
	    					{
	    				        count = count + 1;
	    				    }					
						}
				}
				
				if(img_count > count)
				{
					validator.addMessage("err: The ground truth image directory only has $count $extension images.");
				}
			}
		}
		
		String image_set_dir = expressionMap.get("image_set_dir");
		
		if(image_set_dir != null && image_set_dir != "")
		{
			String img_label = expressionMap.get("img_label");
			if(img_label != null && img_label != "")
			{
				File dir = new File(image_set_dir);
				File label_dir = null;
				boolean exists = false;
				dir.eachDir()
	    			{
	    				if(it.name == img_label)
	    				{
	    					exists = true;
	    					label_dir = it;
	    				}
	    			}
				
				if(!exists)
				{
					validator.addMessage("err: The label \"${img_label}\" does not exist in the Image set directory.");
				}
				else
				{
					String img_count_str = expressionMap.get("img_count_group");
					if(img_count_str != null && img_count_str != "")
					{
						int img_count = img_count_str.toInteger();
						String extension = expressionMap.get("image_format_group");
						
						int count = 0;
						
						label_dir.eachFile()
						{
	    					if (it.name.endsWith(extension)) 
	    					{
	    				        count = count + 1;
	    				    }					
						}
						
						if(img_count > count)
						{
							validator.addMessage("err: The image directory only has $count $extension images.");
						}
					}
				}
			}
		}
	}
	
	
}