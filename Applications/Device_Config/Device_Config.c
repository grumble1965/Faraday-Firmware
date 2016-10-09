/*
 * Device_Config.c
 *
 *  Created on: Apr 3, 2016
 *      Author: Brent
 */

#include "Device_Config.h"
#include "../../Faraday_HAL/flash.h"
#include "../../RF_Network_Stack/rf.h"
#include "../Telemetry/Telemetry.h"
#include "../../Faraday_HAL/Misc_Functions.h"
#include "msp430.h"
#include "../../Faraday_HAL/Faraday_HAL.h"

#define CONFIG_CALLSIGN_OFFSET 0

//External variables
//volatile extern unsigned char app_telem_uart_boot_bits;
//volatile extern unsigned char app_telem_rf_boot_bits;

volatile unsigned char config_bitmask;
volatile unsigned char local_callsign_len;
volatile char local_callsign[MAX_CALLSIGN_LENGTH];
volatile unsigned char local_device_id;
volatile unsigned char default_gpio_p3_bitmask;
volatile unsigned char default_gpio_p4_bitmask;

//RF
volatile unsigned char boot_freq[RF_FREQ_LEN];
volatile unsigned char boot_rf_PATABLE;


//GPS
volatile char default_lattitde[GPS_LATTITUDE_LEN];
volatile char default_lattitude_dir;
volatile char default_longitude[GPS_LONGITUDE_LEN];
volatile char default_longitude_dir;
volatile char default_altitude[GPS_ALTITUDE_LEN];
volatile char default_altitude_units;
volatile unsigned char gps_boot_bitmask;

//Telemetry
volatile unsigned char telem_boot_bitmask;
volatile unsigned int telem_default_uart_interval;
volatile unsigned int telem_default_rf_interval;

void app_device_config_write(void){
	unsigned char mem_buf[FLASH_MEM_ADR_DEVICE_DESCRIPTOR_SEGMENT_SIZE];

	//Read configuration memory in for editing
	memcpy(&mem_buf, FLASH_MEM_ADR_INFO_D, FLASH_MEM_ADR_DEVICE_DESCRIPTOR_SEGMENT_SIZE);

	__no_operation();
}

void app_device_config_write_callsign(unsigned char *callsign){
	//Callsign field is 6 characters all the time, pad as neccessary
	unsigned char mem_buf[FLASH_MEM_ADR_DEVICE_DESCRIPTOR_SEGMENT_SIZE];
	const unsigned char callsign_len = 6;


	//Read configuration memory in for editing
	memcpy(&mem_buf, FLASH_MEM_ADR_INFO_D, FLASH_MEM_ADR_DEVICE_DESCRIPTOR_SEGMENT_SIZE);
	memcpy(&mem_buf, callsign, callsign_len);

	flash_erase_segment(FLASH_MEM_ADR_INFO_D);


	//////

	flash_write_buffer(FLASH_MEM_ADR_INFO_D, &mem_buf, 128);

	//////

	__no_operation();
}

void app_device_config_write_buffer(unsigned char *data, unsigned char len){
	//Callsign field is 6 characters all the time, pad as neccessary
	unsigned char mem_buf[FLASH_MEM_ADR_DEVICE_DESCRIPTOR_SEGMENT_SIZE];

	//Read configuration memory in for editing
	memcpy(&mem_buf, FLASH_MEM_ADR_INFO_D, FLASH_MEM_ADR_DEVICE_DESCRIPTOR_SEGMENT_SIZE);
	memcpy(&mem_buf, data, len);

	flash_erase_segment(FLASH_MEM_ADR_INFO_D);


	//////

	flash_write_buffer(FLASH_MEM_ADR_INFO_D, &mem_buf, len);

	//////

	__no_operation();
}


void app_device_config_load_default(void){
	unsigned char flash_programmed_bit, device_callsign_len;
	memcpy(&flash_programmed_bit, CONFIG_BASIC_FLASH_CONFIG_BITMASK_ADDR, 1);
	memcpy(&device_callsign_len, CONFIG_BASIC_LOCAL_CALLSIGN_LEN_ADDR, 1);

	//Check if flash programmed correctly, if not then program defaults to have something in memory. This should ONLY occur if the device has never be used before or was completely ereased!
	if((!check_bitmask(flash_programmed_bit, BIT0)) | (device_callsign_len>MAX_CALLSIGN_LENGTH)){
		__no_operation();
	//if((flash_programmed_bit != 1) || (device_callsign_len>10)){
		//Device improperly programmed

		//Basic Configuration
		default_flash_config_struct program_config_boot_struct;
		program_config_boot_struct.flash_config_bitmask = BIT0;
		program_config_boot_struct.local_callsign_len = sizeof("NOCALL")-1; //-1 Because string contains \n as last byte
		memcpy(program_config_boot_struct.local_callsign, "NOCALL", program_config_boot_struct.local_callsign_len);
		program_config_boot_struct.local_device_id = 0;
		program_config_boot_struct.default_gpio_p3_bitmask = 0x00;
		program_config_boot_struct.default_gpio_p4_bitmask = 0x00;

		//RF
		//Default frequency is 914.5MHz
		program_config_boot_struct.boot_freq[2] = 0x23; //FREQ MSB
		program_config_boot_struct.boot_freq[1] = 0x2c;
		program_config_boot_struct.boot_freq[0] = 0x4e; //FREQ LSB
		program_config_boot_struct.PATable = 152;

		//GPS
		//Leave default location if not set yet to 0's. Or we could put it somewhere... funny...
		program_config_boot_struct.default_lattitde;
		program_config_boot_struct.default_lattitude_dir;
		program_config_boot_struct.default_longitude;
		program_config_boot_struct.default_longitude_dir;
		program_config_boot_struct.default_altitude;
		program_config_boot_struct.default_altitude_units;
		program_config_boot_struct.gps_boot_bitmask = 1;

		//Telemetry
		program_config_boot_struct.telem_boot_bitmask = BIT0 + BIT1;
		//int_to_byte_array(&program_config_boot_struct.telem_default_uart_interval, 10);
		program_config_boot_struct.telem_default_uart_interval = 1;
		//int_to_byte_array(&program_config_boot_struct.telem_default_rf_interval, 5);
		program_config_boot_struct.telem_default_rf_interval = 5;

		app_device_config_write_buffer(&program_config_boot_struct.flash_config_bitmask, CONFIG_PACKET_LEN);
	}

	//Read default configuration
	app_device_config_read_defaults();

	//Set initial radio frequency on boot
	radio_load_defaults(boot_freq[2], boot_freq[1], boot_freq[0]); //Update RFSettings struct for boot initialization. NOTE: This function follows standard CC430 library from MSB first, i.e [FREQ2, FREQ1, FREQ0]
	CC430_Program_Freq(boot_freq[2], boot_freq[1], boot_freq[0]); // Program radio frequency (may not need if writing at boot prior to radio init)
	//CC430_Program_Freq(78, 44, 35); // Program radio frequency (may not need if writing at boot prior to radio init)
	//Update RF Power on configuration
	Faraday_RF_PWR_Change(boot_rf_PATABLE);
}

void app_device_config_read_defaults(void){
	//Read default device configurations from memory into global variables for use during operations
	//Basic
	memcpy(&config_bitmask,CONFIG_BASIC_FLASH_CONFIG_BITMASK_ADDR,1);
	memcpy(&local_callsign_len,CONFIG_BASIC_LOCAL_CALLSIGN_LEN_ADDR,1);
	memcpy(&local_callsign,CONFIG_BASIC_LOCAL_CALLSIGN_ADDR,MAX_CALLSIGN_LENGTH);
	memcpy(&local_device_id,CONFIG_BASIC_LOCAL_CALLSIGN_ID_ADDR,1);
	memcpy(&default_gpio_p3_bitmask,CONFIG_BASIC_P3_GPIO_ADDR,1);
	memcpy(&default_gpio_p4_bitmask,CONFIG_BASIC_P4_GPIO_ADDR,1);

	//RF
	memcpy(&boot_freq,CONFIG_RF_DEFAULT_FREQ_ADDR,RF_FREQ_LEN); //Note: This will read into an array Freq0:2 and the array will follow, i.e. Array[0]= freq0 which is frequency LSB
	memcpy(&boot_rf_PATABLE,CONFIG_RF_DEFAULT_PATABLE_ADDR,1);



	//GPS
	memcpy(&default_lattitde,CONFIG_GPS_DEFAULT_LATTITUDE_ADDR,GPS_LATTITUDE_LEN);
	memcpy(&default_lattitude_dir,CONFIG_GPS_DEFAULT_LATTITUDE_DIR_ADDR,1);
	memcpy(&default_longitude,CONFIG_GPS_DEFAULT_LONGITUDE_ADDR,GPS_LONGITUDE_LEN);
	memcpy(&default_longitude_dir,CONFIG_GPS_DEFAULT_LONGITYUDE_DIR_ADDR,1);
	memcpy(&default_altitude,CONFIG_GPS_DEFAULT_ALTITUDE_ADDR,GPS_ALTITUDE_LEN);
	memcpy(&default_altitude_units,CONFIG_GPS_DEFAULT_ALTITUDE_UNITS_ADDR,1);
	memcpy(&gps_boot_bitmask,CONFIG_GPS_DEFAULT_BOOT_BITMASK_ADDR,1);

	//Telemetry
	memcpy(&telem_boot_bitmask,CONFIG_TELEMETRY_BOOT_BITMASK_ADDR,1);
	memcpy(&telem_default_uart_interval,CONFIG_TELEMETRY_UART_INTERVAL_ADDR,2);
	memcpy(&telem_default_rf_interval,CONFIG_TELEMETRY_RF_INTERVAL_ADDR,2);
}

void app_device_config_update_ram_parameter(unsigned char parameter, unsigned char *payload){
	switch(parameter){
	case 0: //Callsign
		memcpy(&local_callsign_len, &payload[0], 1);
		memcpy(&local_callsign, &payload[1], MAX_CALLSIGN_LENGTH);
		memcpy(&local_device_id, &payload[10], 1);
		break;
	case 1: //
		break;
	case 2: //GPIO
		break;
	case 3: //GPS Location Data
		break;
	case 4: //GPS Boot Bitmask
		break;
	case 5: //Telemetry Bitmask
		break;
	case 6: //Telemetry Interval
		break;
	default:
		break;
	}
}

void app_device_config_device_debug_reset(void){
	unsigned char buf_info_c[128]; //Reset to all zeros
	unsigned char i;

	for(i=0;i<128;i++){
		buf_info_c[i] = 0;
	}
	//Save incremented bootup counter
	flash_erase_segment(FLASH_MEM_ADR_INFO_C);
	flash_write_buffer(FLASH_MEM_ADR_INFO_C, buf_info_c, 128);
	//flash_write_info_c_segment_char(CONFIG_DEBUG_BOOT_COUNT_INDEX, bootcnt);
	//flash_write_info_c_segment_int(CONFIG_DEBUG_BOOT_COUNT_INDEX, bootcnt);
	//flash_write_char(FLASH_MEM_ADR_INFO_C + CONFIG_DEBUG_BOOT_COUNT_INDEX, bootcnt)
}


void app_device_config_device_debug_update(unsigned char param, unsigned char command){
	static unsigned int bootcnt = 0;
	switch(param){
		case 0: //Boot up
			app_device_config_device_debug_increment_int(CONFIG_DEBUG_BOOT_COUNT_INDEX);
			/*
			memcpy(&bootcnt, FLASH_MEM_ADR_INFO_C + CONFIG_DEBUG_BOOT_COUNT_INDEX, 2);
			bootcnt += 1; //Increment counter
			//Save incremented bootup counter
			flash_write_info_c_segment_int(CONFIG_DEBUG_BOOT_COUNT_INDEX, bootcnt);
			*/
			break;
		case 1:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_RESET_COUNT_INDEX);
			break;
		case 2:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_BOR_INDEX);
			break;
		case 3:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_RSTNMI_INDEX); // A HARD reset in debugger will trigger this too!
			break;
		case 4:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_SVSL_INDEX);
			break;
		case 5:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_SVSH_INDEX);
			break;
		case 6:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_SVML_OVP_INDEX);
			break;
		case 7:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_SVMH_OVP_INDEX);
			break;
		case 8:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_WDT_TIMEOUT_INDEX);
			break;
		case 9:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_FLASHKEY_INDEX);
			break;
		case 10:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_FLL_UNLOCK_INDEX);
			break;
		case 11:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_PERIPH_CONFIG_INDEX);
			break;
		case 12:
			app_device_config_device_debug_increment_char(CONFIG_DEBUG_BOOT_SYSRSTIV_SYSUNIV_ACCVIFG_INDEX);

			break;
		default:
			break;
	}
}

void app_device_config_device_debug_increment_int(unsigned char offset){
	unsigned int buf_value;
	memcpy(&buf_value, FLASH_MEM_ADR_INFO_C + offset, 2);
	buf_value += 1; //Increment counter
	//Save incremented bootup counter
	flash_write_info_c_segment_int(offset, buf_value);
}

void app_device_config_device_debug_increment_char(unsigned char offset){
	unsigned char buf_value;
	memcpy(&buf_value, FLASH_MEM_ADR_INFO_C + offset, 1);
	buf_value += 1; //Increment counter
	//Save incremented bootup counter
	flash_write_info_c_segment_char(offset, buf_value);
}