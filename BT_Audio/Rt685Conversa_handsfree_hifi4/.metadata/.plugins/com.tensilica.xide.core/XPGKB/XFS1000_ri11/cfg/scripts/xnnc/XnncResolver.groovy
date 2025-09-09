//
// Copyright (c) 2021 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.
//

import org.ini4j.Profile.Section;
import org.ini4j.Wini;
import com.tensilica.xide.xnnc.core.constants.IXNNCConfigurationConstants;
import com.tensilica.xide.xnnc.core.constants.XnncConstants;

class XnncResolver
{
	static void resolveCfgToXcfg(Wini xcfg , Wini cfg , String xnncVersion)
	{
		setOutputTemp(cfg);
		
		for(String sectionKey : cfg.keySet())
		{
			Section section = cfg.get(sectionKey);
			
			if(sectionKey.equals("tf2onnx"))
			{
				handleTF2OnnxSectionXcfg(section, xcfg);
				continue;
			}
			
			for(String key : section.keySet())
			{
				String val = section.get(key);
				
				if(sectionKey.equals("out_cntrl") && key.equals("description"))
				{
					val = val.replaceAll("=" , "::");
				}
				
				if((key.equals("input") || key.equals("output") || key.equals("tensor")) && sectionKey.equals("graph"))
				{
					handleGraphSplitsXcfg(key, val, xcfg, ",");
					continue;
				}
				
				if(sectionKey.equals("quantization") && key.equals("range"))
				{
					String value = section.get(key);
					String[] splitVals = value.split(":");
					
					if(splitVals.length == 3)
					{
						xcfg.put(sectionKey , sectionKey + "-" + key , splitVals[0] + ":" + splitVals[1]);
						xcfg.put(sectionKey , sectionKey + "-stride" , splitVals[2]);
					}
					else if(splitVals.length == 2)
					{
						xcfg.put(sectionKey , sectionKey + "-" + key , splitVals[0] + ":" + splitVals[1]);
						xcfg.put(sectionKey , sectionKey + "-stride" , "1");
					}
					continue;	
				}
				
				xcfg.put(sectionKey, sectionKey + "-" + key , val);
			}
		}
		
	}
	
	static void resolveXcfgToCfg(Wini xcfg , Wini cfg , String xnncVersion)
	{

		for(String sectionKey : xcfg.keySet())
		{
			if(!(sectionKey.equals("xcfg")))
			{
				Section section = xcfg.get(sectionKey);
				
				if(sectionKey.equals("tf2onnx"))
				{
					handleTF2OnnxSectionCfg(section, cfg);
					continue;
				}
				
				if(sectionKey.equals("graph"))
				{
					handleGraphSectionCfg(section, cfg);
					continue;
				}
				
				if(sectionKey.equals("out_cntrl"))
				{
					handleOutputControlCfg(section , cfg);
					continue;
				}
				
				if(sectionKey.equals("testInferenceXtraArgs"))
				{
					handletestInferenceXtraArgsCfg(section , cfg);
					continue;
				}
				
				for(String key : section.keySet())
				{
					if(sectionKey.equals("quantization") && key.equals("quantization-range"))
					{
						String value = section.get(key);
						String stride = section.get("quantization-stride");
						
						cfg.put(sectionKey, key.split("-")[1] , value + ":" + stride);
						continue;
					}
					if(sectionKey.equals("quantization") && key.equals("quantization-stride"))
						continue;

					String cfgKey = key.split("-")[1];
					cfg.put(sectionKey, cfgKey , section.get(key));
				}
			}
		}
	}
	
	static void splitKeys(Section section , String key , int numParts , String sep)
	{
		String[] values = section.get(key).split(sep);
		for(int i = 0 ; i < numParts && i < values.length ; i++)
		{
			String newKey = key + Integer.toString(i+1);
			
			section.put(newKey, values[i]);
		}
	}
	
	static void handleTF2OnnxSectionXcfg(Section tf2onnx_cfg , Wini xcfg)
	{
		String args = tf2onnx_cfg.get("args").trim();
		if(args.contains("--inputs") || args.contains("--outputs"))
		{
			def input_pattern = ~"--inputs\\s(.*?)(\\s|\$)";
			def output_pattern = ~"--outputs\\s(.*?)(\\s|\$)";
			def input_matcher = args =~ input_pattern ;
			def output_matcher = args =~ output_pattern;
			
			if(input_matcher.find())
			{
				xcfg.put("tf2onnx","tf2onnx-inputs" , input_matcher.group(1));
			}
			
			if(output_matcher.find())
			{
				xcfg.put("tf2onnx","tf2onnx-outputs" , output_matcher.group(1));
			}
			
			String extra_args = args.replaceAll(input_pattern, "");
			extra_args = extra_args.replaceAll(output_pattern, "");
			
			xcfg.put("tf2onnx","tf2onnx-extra_args" , extra_args);

		}
		else
		{
			xcfg.put("tf2onnx","tf2onnx-extra_args" , args);
		}

		
		String modelFormat = tf2onnx_cfg.get("TF_model_format");
		
		if(modelFormat != null)
		{
			modelFormat = modelFormat.trim();
			xcfg.put("tf2onnx" , "tf2onnx-TF_model_format" , modelFormat);
		}
	}
	
	static void handleGraphSplitsXcfg(String key , String value , Wini xcfg , String sep)
	{
		String[] values = value.split(sep);
		for(int i = 0 ; i < values.length ; i++)
		{
			switch(i)
			{
				case 0:
					xcfg.put("graph","graph-" + key + "_name" , values[i]);
					break;
				case 1:
					xcfg.put("graph","graph-" + key + "_layout" , values[i]);
					break;
				case 2:
					xcfg.put("graph","graph-" + key + "_stride1" , values[i]);
					break;
				case 3:
					xcfg.put("graph","graph-" + key + "_stride2" , values[i]);
					break;
				case 4:
					xcfg.put("graph","graph-" + key + "_stride3" , values[i]);
					break;
			}
		}
	}
	
	static void handleTF2OnnxSectionCfg(Section tf2onnx , Wini cfg)
	{
		String inputs = tf2onnx.get("tf2onnx-inputs");
		String outputs = tf2onnx.get("tf2onnx-outputs");
		String extraArgs = tf2onnx.get("tf2onnx-extra_args");
		String val = "";
		if(inputs != null && outputs != null)
		{
			val = "--inputs ${inputs} --outputs ${outputs} ";
		}
		
		String finalVal = val + "${extraArgs}";
		
		cfg.put("tf2onnx" , "args" , finalVal);
		
		String model_format = tf2onnx.get("tf2onnx-TF_model_format");
		
		if(model_format != null)
			cfg.put("tf2onnx" , "TF_model_format" , model_format);
	}
	
	static void handleGraphSectionCfg(Section graph , Wini cfg)
	{
		for(String key : graph.keySet())
		{
			if(key.contains("graph-input"))
			{
				handleGraphSplitsCfg(graph, "input", cfg, ",");;
			}
			else if(key.contains("graph-output"))
			{
				handleGraphSplitsCfg(graph, "output", cfg, ",");;
			}
			else if(key.contains("graph-tensor"))
			{
				handleGraphSplitsCfg(graph, "tensor", cfg, ",");;
			}
			else
				cfg.put("graph" , key.split("-")[1] , graph.get(key))

		}
		
	}
	
	static void handleOutputControlCfg(Section section , Wini cfg)
	{
		cfg.put("out_cntrl" , "description" , section.get("out_cntrl-cfg_description"));
	}
	
	static void handleGraphSplitsCfg(Section section , String contentionKey , Wini cfg , String sep)
	{
		if(cfg.get("graph" , contentionKey) != null)
			return;
		
		String name = ""; 
		String layout = "";
		String stride1 = "";
		String stride2 = "";
		String stride3 = "";
		
		for(String key : section.keySet())
		{
			if(key.contains(contentionKey))
			{
				switch(key)
				{
					case ~/.*_name/:
						name = section.get(key);
						break;
					case ~/.*_layout/:
						layout = section.get(key);
						break;
					case ~/.*_stride1/:
						stride1 = section.get(key);
						break;
					case ~/.*_stride2/:
						stride2 = section.get(key);
						break;
					case ~/.*_stride3/:
						stride3 = section.get(key);
						break;						
				}
			}
		}
		
		String finalVal = "";
		if(stride1 == "" || stride2 == "" || stride3 == "")
			finalVal = name + sep + layout;
		else
			finalVal = name + sep + layout + sep + stride1 + sep + stride2 + sep + stride3;
		
		cfg.put("graph" , contentionKey , finalVal);

	}
	
	static void handletestInferenceXtraArgsCfg(Section section , Wini cfg)
	{
		String inputVal = section.get("testInferenceXtraArgs---input-layout");
		
		cfg.put("testInferenceXtraArgs" , "--input-layout" , inputVal);
	}
	
	static void setOutputTemp(Wini ini)
	{
		ini.put(IXNNCConfigurationConstants.PATH_SECTION,
				IXNNCConfigurationConstants.OUTPUT_DIR, XnncConstants.OUTPUT_FOLDER);
		ini.put(IXNNCConfigurationConstants.PATH_SECTION,
				IXNNCConfigurationConstants.TEMP_DIR, XnncConstants.TEMP_FOLDER);
	}
	
}
