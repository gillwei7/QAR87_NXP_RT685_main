/*
 * Copyright 2021 Retune DSP
 * Copyright 2022-2023, 2025 NXP
 * 
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP
 * and may only be used strictly in accordance with the applicable license terms.
 * By expressly accepting such terms or by downloading, installing,
 *  activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms.
 * If you do not agree to be bound by the applicable license terms,
 * then you may not retain, install, activate or otherwise use the software.
 */


#ifndef NXP_CONVERSA_PLUGIN_H_
#define NXP_CONVERSA_PLUGIN_H_

#include "NxpConversaStatusCodes.h"
#include "NxpDeviceConfig.h"
#include "NxpTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @struct nxp_conversa_plugin_constants_t
 *   Conversa constants that can be queried with NxpConversa_Plugin_GetConstants().
 */
typedef struct nxp_conversa_plugin_constants_s {
	uint32_t max_num_mics;						///< Maximum number of microphones supported.
	uint32_t max_num_spks;						///< Maximum number of speakers supported.
	uint32_t num_bytes_per_sample;				///< Number of bytes per audio sample.
} nxp_conversa_plugin_constants_t;


 /** @struct nxp_conversa_plugin_config_t
  *   Configurable options of the Conversa library that must be defined
  *   by the user before calling NxpConversa_Plugin_Create().
  */
typedef struct nxp_conversa_plugin_config_s {
	uint32_t num_mics;							///< Number of microphones (Tx inputs).
	uint32_t num_mics_bf;						///< Number of microphones at BF input.
	uint32_t num_rx_in;							///< Number of Rx inputs.
	uint32_t num_spks;							///< Number of speakers (Rx outputs).
	uint32_t sample_rate;						///< Sample rate in Hertz.
	uint32_t framesize;							///< Number of samples per frame.
	uint32_t num_bands;							///< Number of frequency bands in the filterbank.
	uint32_t fb_lp;								///< Length of filterbank prototype.
	uint32_t create_aec;						///< Set create_aec = 1 to allocate memory for the Acoustic Echo Canceller (AEC).
	nxp_float aec_filter_length_ms;				///< Set the length of the AEC filter in milliseconds. May be rounded up to support SIMD.
	uint32_t aec_uses_current_sensing;			///< Set aec_uses_current_sensing = 1 to use an alternative reference signal than the Rx input for the AEC.
	uint32_t create_bf;							///< Set create_bf = 1 to allocate memory for the Beamformer (BF).
	uint32_t create_doa;						///< Set create_doa = 1 to allocate memory for the Direction of Arrival (DOA) estimator.
	uint32_t create_se_model;					///< Set create_se_model = 1 to allocate memory for the Speech Enhancement (SE) model.
	uint8_t* se_model_ptr;						///< Set the pointer to the SE model data.
	uint32_t se_model_size;						///< Set the size of the SE model data.
	uint32_t se_model_is_permuted;				///< Select whether the SE model data is permuted. Unpermuted models will be copied to the Conversa heap and permuted there.
	uint32_t enable_tuning_tool_signal_buffer;	///< Set enable_tuning_tool_signal_buffer = 1 to enable additional tuning tool signals. Can be disabled after tuning to save memory.
	uint32_t internal_output_selector;			///< Bitfield to select the internal(s) buffer(s) to be extracted. see conversa_internal_output enum for details.
	uint32_t device_id;							///< Set the NXP device ID for hardware identification.
} nxp_conversa_plugin_config_t;


/** @typedef conversa_Handle_t
 *  Instance handle pointer
 */
typedef void *conversa_Handle_t;

/** @struct nxp_conversa_plugin_t
 *   Conversa plugin struct containing instance handle, current configuration and memory addresses.

 */
typedef struct nxp_conversa_plugin_s {
	nxp_conversa_plugin_config_t config;		///< The current Conversa configuration as set by NxpConversa_Plugin_Create().
	void* memory_base_address;					///< Address to Conversa's heap non-critical memory. See conversa_memory_req_t for more info
	void* critical_memory_base_address;			///< Address to Conversa's heap critical memory. See conversa_memory_req_t for more info
	uint32_t memory_size_bytes;					///< Size of Conversa's heap memory. The required size can be queried using conversa_memory_req_t.NonCritical_mem returned by NxpConversa_Plugin_GetRequiredHeapMemoryBytes().
	uint32_t critical_memory_size_bytes;		///< Size of Conversa's critical heap memory. The required size can be queried using conversa_memory_req_t.Critical_mem returned by NxpConversa_Plugin_GetRequiredHeapMemoryBytes().
	conversa_Handle_t handle;					///< Pointer to Conversa's instance handle
} nxp_conversa_plugin_t;

/** @struct conversa_memory_req_t
 *  @brief Conversa memory requirements structure
 *
 *   For platforms with Cache:
 *   	* IF Conversa AND tool communication are running on different cores:
 *   		- Critical_mem MUST be located in NonCacheable section
 *   	* IF Conversa AND tool communication are running on same core:
 *   		- No restrictions
 *   For platforms without Cache:
 *   	- No restrictions
 */
typedef struct conversa_memory_req_s
{
	size_t  Critical_mem;						///< Conversa requirements for Critical section memory size
	size_t	NonCritical_mem;					///< Conversa requirements for non-critical section memory size
} conversa_memory_req_t;

/**
 * Query the Conversa library version.
 * 
 *  @param [in] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [out] Amajor Major version number
 *  @param [out] Aminor Minor version number
 *  @param [out] Apatch Patch version number
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_GetLibVersion(nxp_conversa_plugin_t* APluginInit_s, uint32_t* Amajor, uint32_t* Aminor, uint32_t* Apatch);

/**
 * Query the constants the Conversa library was built with.
 * 
 *  @param [out] Aconversa_constants Pointer to struct to store results.
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_GetConstants(nxp_conversa_plugin_constants_t* Aconversa_constants);

/**
 * Check if nxp_conversa_plugin_config_t is a valid configuration before creating.
 * 
 *  @param [in] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_CheckConfiguration(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Check how much dynamic memory the Conversa configuration needs.
 * This amount must be allocated by the caller and passed as part of the 
 * nxp_conversa_plugin_t struct when calling NxpConversa_Plugin_Create().
 * 
 *  @param [in] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return conversa_memory_req_t filled with heqp requirements for both critical, non-critical section
 */
extern conversa_memory_req_t NxpConversa_Plugin_GetRequiredHeapMemoryBytes(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Creates an instance of the Conversa plugin based on nxp_conversa_plugin_config_t.
 * The instance will be created at memory_base_address
 * 
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_Create(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Check how much dynamic memory was used by the Conversa instance.
 * 
 *  @param  [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return the number of heap memory allocated by Conversa in bytes.
 */
extern size_t NxpConversa_Plugin_GetAllocatedMemoryBytes(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Initializes the Conversa plugin to the default state.
 * 
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_Init(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Destroys the instance of the Conversa plugin. Plugin memory can be safely freed.
 * 
 *  @param  [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_Destroy(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Runs the Conversa plugin process with Q31 fixed-point input.
 * 
 * Internal processing is in floating point.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [in] Amic_in Pointer to an array of mic inputs. Amic_in[0][0]..Amic_in[0][framesize-1]
 *		is one frame of data from the first mic. Amic_in[1][0] points to one frame of data
 *		from the second mic.
 *  @param [in] Arx_in Pointer to an array of Rx inputs. Arx_in[0][0]..Arx_in[0][framesize-1]
 *		is one frame of data from the first Rx channel. Arx_in[1][0] points to one frame of data
 *		from the second Rx channel.
 *  @param [in] Aopt_current_ref Optional pointer to an array of current ref. inputs.
 *		`aec_uses_current_sensing` must be enabled in the `rdsp_conversa_plugin_config_t`.
 *		Aopt_current_ref[0][0]..Aopt_current_ref[0][framesize-1] is one frame of data
 *		from the first current ref. channel. Aopt_current_ref[1][0] points to one frame of data
 *		from the second current ref. channel.
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_Process_Q31(nxp_conversa_plugin_t* APluginInit_s, int32_t** Amic_in, int32_t** Arx_in, int32_t** Aopt_current_ref);

/**
 * Returns a pointer to an array of Rx outputs in Q31 fixed-point.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return a pointer to the Rx output in Q31 fixed-point form.
 */
extern int32_t** NxpConversa_Plugin_GetRxOut_Q31(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Returns a pointer to the Tx output in Q31 fixed-point.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return a pointer to the Tx output in Q31 fixed-point form.
 */
extern int32_t* NxpConversa_Plugin_GetTxOut_Q31(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Returns a pointer to the Tx beamformer output in Q31 fixed-point.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return a pointer to the beamformer output in Q31 fixed-point form.
 */
extern int32_t* NxpConversa_Plugin_GetTxBfOut_Q31(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Runs the Conversa plugin process with floating-point input.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [in] Amic_in Pointer to an array of mic inputs. Amic_in[0][0]..Amic_in[0][framesize-1]
 *		is one frame of data from the first mic. Amic_in[1][0] points to one frame of data
 *		from the second mic.
 *  @param [in] Arx_in Pointer to an array of Rx inputs. Arx_in[0][0]..Arx_in[0][framesize-1]
 *		is one frame of data from the first Rx channel. Arx_in[1][0] points to one frame of data
 *		from the second Rx channel.
 *  @param [in] Aopt_current_ref Optional pointer to an array of current ref. inputs.
 *		`aec_uses_current_sensing` must be enabled in the `rdsp_conversa_plugin_config_t`.
 *		Aopt_current_ref[0][0]..Aopt_current_ref[0][framesize-1] is one frame of data
 *		from the first current ref. channel. Aopt_current_ref[1][0] points to one frame of data
 *		from the second current ref. channel.
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_Process(nxp_conversa_plugin_t* APluginInit_s, nxp_float ** Amic_in, nxp_float** Arx_in, nxp_float** Aopt_current_ref);

/**
 * Returns a pointer to an array of Rx outputs.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return a pointer to the Rx output in floating-point form.
 */
extern nxp_float** NxpConversa_Plugin_GetRxOut(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Returns a pointer to the Tx output.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return a pointer to the Tx output in floating-point form.
 */
extern nxp_float* NxpConversa_Plugin_GetTxOut(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Returns a pointer to the NLP output.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return a pointer to the Nlp output in floating-point form.
 */
extern nxp_float* NxpConversa_Plugin_GetTxNlpOut(nxp_conversa_plugin_t *APluginInit_s);

/**
 * Returns a pointer to the AEC (linear part) output.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [in] ch_index select which AEC channel to extract.
 *  @return a pointer to the AEC output in floating-point form.
 */
extern nxp_float* NxpConversa_Plugin_GetTxAecOut(nxp_conversa_plugin_t *APluginInit_s, uint32_t ch_index);

/**
 * Get pointer to the Tx beamformer output.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return  a pointer to the beamformer output in floating-point form.
 */
extern nxp_float* NxpConversa_Plugin_GetTxBfOut(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Returns a pointer to the NLP freq output.
 *
 *  @param [in] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return a pointer to the Nlp frequencies output bins in complex floating-point form.
 */
extern nxp_complex* NxpConversa_Plugin_GetTxNlpFreqOut(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Returns a pointer to the AEC freq (linear part) output.
 *
 *  @param [in] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [in] ch_index select which AEC channel to extract.
 *  @return a pointer to the AEC frequencies output bins in complex floating-point form.
 */
extern nxp_complex* NxpConversa_Plugin_GetTxAecFreqOut(nxp_conversa_plugin_t* APluginInit_s, uint32_t ch_index);


/**
 * Returns a pointer to the AEC Ref freq (linear part) output.
 *
 *  @param [in] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [in] ch_index select which AEC Ref channel to extract.
 *  @return a pointer to the AEC Ref frequencies output bins in complex floating-point form.
 */
extern nxp_complex* NxpConversa_Plugin_GetTxAecRefFreqOut(nxp_conversa_plugin_t* APluginInit_s, uint32_t ch_index);

/**
 * Get pointer to the Tx beamformer output.
 *
 *  @param [in] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return  a pointer to the beamformer frequencies output bins in complex floating-point form.
 */
extern nxp_complex* NxpConversa_Plugin_GetTxBfFreqOut(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Returns a pointer to the Conversa control data struct for the tuning tool connection.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @return a pointer to the Conversa control data struct.
 */
extern void* NxpConversa_Plugin_GetControlDataAddress(nxp_conversa_plugin_t* APluginInit_s);

/**
 * Set Conversa parameters
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [in] Ainfo Parameter info string
 *  @param [in] Ainfo_length Length of parameter info string
 *  @param [in] Aparameters Pointer to the binary parameter data.
 *  @param [in] Anum_32b_words Number of 32-bit words expected in Aparameters. Will not be checked if set to 0.
 *  @return OK if successful or INVALID_PARAMETER if there is an error loading the parameters.
 */
extern NXP_STATUS NxpConversa_Plugin_SetParameters(nxp_conversa_plugin_t* APluginInit_s, const char* Ainfo, int32_t Ainfo_length, const uint32_t* Aparameters, uint32_t Anum_32b_words);

/**
 * Set Conversa parameters from .bin file.
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [in] Aparameters Pointer to the binary parameter data.
 *  @param [in] Anum_32b_words Number of 32-bit words expected in the parameter .bin file. Will not be checked if set to 0.
 *  @return OK if successful or INVALID_PARAMETER if there is an error loading the parameters.
 */
extern NXP_STATUS NxpConversa_Plugin_SetParameterBinData(nxp_conversa_plugin_t* APluginInit_s, const uint32_t* Aparameters, uint32_t Anum_32b_words);

/**
 *  Get Conversa internal Signal
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [in] Aid Signal ID to retrieve
 *  @param [out] Aparameter_data pointer to store the retrieved signal. Ensure Aparameter_data size can contain a signalType signal
 *  @param [in] signalType Signal type to retrieve. See conversa_signal_types enum for more info.
 *  @return OK if successful or INVALID_PARAMETER if there is an error loading the parameters.
 */
extern NXP_STATUS NxpConversa_Plugin_GetSignalID(nxp_conversa_plugin_t *APluginInit_s, uint32_t Aid, void* Aparameter_data, conversa_signal_types signalType);
/**
 * Set Conversa parameter by ID
 *
 *  @param [in,out] APluginInit_s Pointer to the Conversa plugin instance.
 *  @param [in] Aid ID of the parameter.
 *  @param [in] Alength Length of the parameter data.
 *  @param [in] Aparameter_data Pointer to the parameter data.
 *  @return OK if successful or INVALID_PARAMETER if the parameter ID is not found.
 */
extern NXP_STATUS NxpConversa_Plugin_SetParameterID(nxp_conversa_plugin_t* APluginInit_s, uint32_t Aid, uint32_t Alength, uint32_t* Aparameter_data);

/**
 * Print the Conversa memory usage.
 *
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_PrintMemoryUsage(void);

/**
 * Print the current and maximum Conversa MCPS.
 *
 *  @return OK if successful.
 */
extern NXP_STATUS NxpConversa_Plugin_PrintCycleCount(void);

#ifdef __cplusplus
}
#endif

#endif // NXP_CONVERSA_PLUGIN_H_
