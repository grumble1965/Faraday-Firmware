/*
 * Applications.c
 *
 *  Created on: Oct 15, 2015
 *      Author: Brent
 */

#ifndef APPLICATIONS_APPLICATIONS_C_
#define APPLICATIONS_APPLICATIONS_C_

#include "Telemetry.h"
#include <msp430.h>
#include "../../Faraday_Globals.h"
#include "../../Faraday_HAL/Faraday_HAL.h"
//#include "../Faraday_Simple_Transport/Services.h"
//#include "../Scratch/Scratch_Brent.h"
#include "../../HAL/gps.h"
#include "../../RF_Network_Stack/rf.h"
#include "../../REVA_Faraday.h"
#include "../../RF_Network_Stack/rf_transport.h"
//#include "../../Ring_Buffers/FIFO.h"
#include "../../Ring_Buffers/FIFO_SRAM.h"
//#include "../../UART/UART_L4.h"
#include "../../UART/UART_Services.h"
#include "../Device_Config/Device_Config.h"
#include "../../Faraday_HAL/Misc_Functions.h"
#include "../../Faraday_HAL/flash.h"
#include "../../HAL/RF1A.h"
#include "../HAB/App_HAB.h"
#include "../../HAL/adc.h"
#include <time.h>
#include <stdlib.h>

/*
 * External Variables
 */

/*volatile unsigned char device_callsign[6];
volatile unsigned char device_callsign_len;
volatile unsigned char device_id;
volatile unsigned char device_boot_freq[3];
volatile unsigned char app_telem_uart_boot_bits=0;*/

#define TELEM_UART_SERVICE_NUMBER 5



/////////////////////////////////////////
// APPLICATION FIFO UART DEFINITIONS
/////////////////////////////////////////




//Telemetry Application FIFO Packet Buffers
volatile fifo_sram_state_machine app_telem_state_machine;
//volatile unsigned char app_telem_state_machine_fifo_buffer[APP_TELEM_PACKET_LEN*APP_TELEM_PACKET_FIFO_COUNT];

volatile fifo_sram_state_machine app_telem_uart_state_machine;
//volatile unsigned char app_telem_state_machine_uart_fifo_buffer[APP_TELEM_PACKET_LEN];

/////////////////////////////////////////
// END APPLICATION FIFO UART DEFINITIONS
/////////////////////////////////////////

/////////////////////////////////////////
// RF Service FIFO DEFINITIONS
/////////////////////////////////////////
#define TELEM_FIFO_SIZE 63
#define TELEM_FIFO_ELEMENT_CNT 3

//Telemetry Application FIFO Packet Buffers
volatile fifo_sram_state_machine rf_rx_telem_state_machine;
//volatile unsigned char rf_rx_telem_fifo_buffer[TELEM_FIFO_SIZE*TELEM_FIFO_ELEMENT_CNT];


/////////////////////////////////////////
// RF Service FIFO  Functions
/////////////////////////////////////////

//volatile unsigned char telem_tx_uart_buf[APP_TELEM_PACKET_LEN];

//UART Beacon interval timer counter
volatile unsigned int telemetry_interval_counter_int = 0;


/*
 * This is the standard FIFO put() command required by the function pointer architecture of the FIFO inteface to devices
 * such as UART or RF. This funciton is called by those service routines when needing to interact with the application.
 */
void app_telem_rf_rx_command_put(unsigned char *data_pointer, unsigned char length){
	//put_fifo(&rf_rx_telem_state_machine, &rf_rx_telem_fifo_buffer, data_pointer);
	put_fifo_sram(&rf_rx_telem_state_machine, data_pointer);
	__no_operation();
}

void app_telem_rf_rx_housekeep(void){
	if(rf_rx_telem_state_machine.inwaiting>0){
		__no_operation();
	}
}

///////////////////////
///////////////////////
///////////////////////

void app_telem_rx_remote_telem(unsigned char *data){
	__no_operation();
	//This shouldn't be direct to UART this should have a FIFO infront!
	//uart_transport_tx_packet(0x31, APP_TELEM_PACKET_LEN, data);;
	//application_send_uart_packet(data);

}


void app_telem_housekeeping(void){

	/*
	 * Move APPLICATION packets into the APPLICATION UART FIFO
	 */
	if((app_telem_state_machine.inwaiting>0) && (app_telem_uart_state_machine.inwaiting == 0)){
		//Create temporary array to store packet from APPLICATION
		volatile unsigned char app_packet_buf[APP_TELEM_PACKET_LEN];
		//GET() APPLICATION packet from FIFO
		//get_fifo(&app_telem_state_machine, app_telem_state_machine_fifo_buffer, (unsigned char *)app_packet_buf);
		get_fifo_sram(&app_telem_state_machine, app_packet_buf);
		//PUT() APPLICATION packet into APPLICATION UART FIFO
		//put_fifo(&app_telem_uart_state_machine, &app_telem_state_machine_uart_fifo_buffer, (unsigned char *)app_packet_buf);
		put_fifo_sram(&app_telem_uart_state_machine, app_packet_buf);
		__no_operation();
	}
	else{
		//Nothing in FIFO
		__no_operation();
	}

	/*
	 * Move APPLICATION UART FIFO packet(s) into the UART stack and transmit
	 */
	if(app_telem_uart_state_machine.inwaiting>0){
		//Create temporary array to store packet from APPLICATION
		unsigned char telem_tx_uart_buf[APP_TELEM_PACKET_LEN];
		//GET() APPLICATION packet from APPLICATION UART FIFO
		//get_fifo(&app_telem_uart_state_machine, app_telem_state_machine_uart_fifo_buffer, (unsigned char *)app_packet_buf);
		get_fifo_sram(&app_telem_uart_state_machine, &telem_tx_uart_buf);
		//Transmit APPLICATOIN UART FIFO packet into UART stack
		//unsigned char service_number = TELEM_UART_SERVICE_NUMBER;
		//uart_transport_tx_packet(service_number, APP_TELEM_PACKET_LEN, telem_tx_uart_buf);
		uart_send(TELEM_UART_SERVICE_NUMBER, APP_TELEM_PACKET_LEN, telem_tx_uart_buf);
	}
	else{
		//Nothing in FIFO
		__no_operation();
	}


	__no_operation();
}

void application_telem_create_pkt_1(unsigned char *packet){
	//Create temporary structs for packets
	TELEMETRY_PACKET_DATAGRAM_STRUCT telem_datagram;
	TELEMETRY_PACKET_1_STRUCT telem_pkt_1;

	//Fill telemetry packet payload with data
	telem_pkt_1.rf_freq2 = ReadSingleReg(FREQ2);
	telem_pkt_1.rf_freq1 = ReadSingleReg(FREQ1);
	telem_pkt_1.rf_freq0 = ReadSingleReg(FREQ0);
	telem_pkt_1.rf_pwr = ReadSingleReg(PATABLE);

	//Copy telemetry packet payload into telemetry datagram
	memcpy(&telem_datagram.data, &telem_pkt_1, 4);

	//Set datagram settings correctly for packet type #1
	telem_datagram.packet_type = 1;
	telem_datagram.rf_source = 0;
	telem_datagram.data_length = 4;

	//Error detection 16 bit calculation
	int_to_byte_array(&telem_datagram.error_detection_16, compute_checksum_16(9-2, &telem_datagram));

	//Copy full packet into pointer buffer
	unsigned char sizetelem = sizeof(telem_datagram);
	memcpy(&packet[0], &telem_datagram, sizetelem);
}


void application_telem_create_pkt_2(unsigned char *packet){
	//Create temporary structs for packets
	TELEMETRYDATA telem;
	TELEMETRY_PACKET_DATAGRAM_STRUCT telem_datagram;
	//volatile TELEMETRY_PACKET_3_STRUCT telem_packet_3_struct;

	//Copy device config memory into buffer
	memcpy(&telem_datagram.data, FLASH_MEM_ADR_INFO_C, APP_TELEM_PACKET_TYPE_2_LEN);


	unsigned char telem_packet_length = APP_TELEM_PACKET_TYPE_2_LEN;//sizeof(telem_packet_3_struct)-1; //-1 for end of struct character (off by one due to data alignmet in memory!)
	telem_datagram.packet_type = 2;
	telem_datagram.rf_source = 0;
	telem_datagram.data_length = APP_TELEM_PACKET_TYPE_2_LEN;

	//Error detection 16 bit calculation
	int_to_byte_array(&telem_datagram.error_detection_16, compute_checksum_16(APP_TELEM_PACKET_LEN-2, &telem_datagram));

	//Copy full packet into pointer buffer
	unsigned char sizetelem = sizeof(telem_datagram);
	memcpy(&packet[0], &telem_datagram, sizetelem);
}


void application_telem_create_pkt(unsigned char *packet, char *src_callsign, unsigned char src_callsign_len, unsigned char src_callsign_id, char *dest_callsign, unsigned char dest_callsign_len, unsigned char dest_callsign_id){
	//Create temporary structs for packets
		TELEMETRYDATA telem;
		TELEMETRY_PACKET_DATAGRAM_STRUCT telem_datagram;
		TELEMETRY_PACKET_3_STRUCT telem_packet_3_struct;

		if(src_callsign_len < 10){
			memcpy(telem_packet_3_struct.source_callsign,(char *)src_callsign, src_callsign_len);
			telem_packet_3_struct.source_callsign_length = src_callsign_len;
			telem_packet_3_struct.source_callsign_id = src_callsign_id;
		}else{
			_no_operation();
		}

		if(dest_callsign_len < 10){
			memcpy(telem_packet_3_struct.destination_callsign,(char *)dest_callsign, dest_callsign_len);
			telem_packet_3_struct.destination_callsign_length = dest_callsign_len;
			telem_packet_3_struct.destination_callsign_id = dest_callsign_id;
		}else{
			_no_operation();
		}

		// Obtains safely read time from RTCRDYIFG interrupt structure
		telem_packet_3_struct.rtc_second = RTC.second;
		telem_packet_3_struct.rtc_minute = RTC.minute;
		telem_packet_3_struct.rtc_hour = RTC.hour;
		telem_packet_3_struct.rtc_day = RTC.day;
		telem_packet_3_struct.rtc_dow = RTC.dow;
		telem_packet_3_struct.rtc_month = RTC.month;
		telem_packet_3_struct.rtc_year = RTC.year;

		//Obtain GPS information
		memcpy(&telem_packet_3_struct.gps_lattitude,(char *)GGA.Lat,9);
		memcpy(&telem_packet_3_struct.gps_lattitude_dir,(char *)GGA.Lat_Dir,1);
		memcpy(&telem_packet_3_struct.gps_longitude,(char *)GGA.Long,10);
		memcpy(&telem_packet_3_struct.gps_longitude_dir,(char *)GGA.Long_Dir,1);
		memcpy(&telem_packet_3_struct.gps_altitude,(char *)GGA.Altitude,8);
		memcpy(&telem_packet_3_struct.gps_altitude_units,(char *)GGA.Alt_Units,1);
		memcpy(&telem_packet_3_struct.gps_speed,(char *)RMC.Speed,5);
		memcpy(&telem_packet_3_struct.gps_fix,(char *)GGA.Fix_Quality,1);
		memcpy(&telem_packet_3_struct.gps_hdop,(char *)GGA.HDOP,4);

		//GPIO and System State
		telem.IO_State	= 0x00;
		telem.IO_State 	|= (P5IN & ARDUINO_IO_8) << 5;		// Grab IO 8 and shift left 5 to put into BIT 8 of IO_State
		telem.IO_State 	|= (P5IN & ARDUINO_IO_9) << 3;		// Grab IO 9 and shift left 3 to put into BIT 7 of IO_State
		telem.IO_State	|= (P5IN & MOSFET_CNTL) << 1;		// Grab MOSFET CNTL value and shift left 1 bit to put into BIT 6 of IO_State
		telem.IO_State	|= (P3IN & LED_1) >> 2;				// Grab LED_1 value and shift right 2 bits to put into BIT 5 of IO_State
		telem.IO_State	|= (P3IN & LED_2) >> 4;				// Grab LED_2 value and shift right 4 bits to put into BIT 4 of IO_State
		telem.IO_State	|= (P3IN & GPS_RESET) >> 1;			// Grab GPS_RESET value and shift right 1 bits to put into BIT 3 of IO_State
		telem.IO_State	|= (P3IN & GPS_STANDBY) >> 3;		// Grab GPS_STANDBY value and shift right 3 bits to put into BIT 2 of IO_State
		telem.IO_State	|= (P1IN & BUTTON_1) >> 7;			// Grab BUTTON_1 value and shift right 7 bits to put into BIT 1 of IO_State

		telem.RF_State	= 0x00;
		telem.RF_State	|= (P3IN & PA_ENABLE) << 7;			// Grab PA_ENABLE value and shift left 7 bits to put into BIT 8 of RF_State
		telem.RF_State	|= (P3IN & LNA_ENABLE) << 5;		// Grab LNA_ENABLE value and shift left 5 bits to put into BIT 7 of RF_State
		telem.RF_State	|= (P3IN & HGM_SELECT) << 3;		// Grab HGM_SELECT value and shift left 3 bits to put into BIT 6 of RF_State

		telem_packet_3_struct.gpio_state = P4IN;
		telem_packet_3_struct.io_state = telem.IO_State;
		telem_packet_3_struct.rf_state = telem.RF_State;

		//ADC Data
		int_to_byte_array(&telem_packet_3_struct.adc_0, ADC_Data[0]);
		int_to_byte_array(&telem_packet_3_struct.adc_1, ADC_Data[1]);
		int_to_byte_array(&telem_packet_3_struct.adc_2, ADC_Data[2]);
		int_to_byte_array(&telem_packet_3_struct.adc_3, ADC_Data[3]);
		int_to_byte_array(&telem_packet_3_struct.adc_4, ADC_Data[4]);
		int_to_byte_array(&telem_packet_3_struct.adc_5, ADC_Data[5]);
		int_to_byte_array(&telem_packet_3_struct.adc_6, ADC_Data[6]);
		int_to_byte_array(&telem_packet_3_struct.adc_7, adc_corrected_calibration_temp_C_int(ADC_Data[7]));
		int_to_byte_array(&telem_packet_3_struct.adc_8, ADC_Data[8]);



		unsigned char telem_packet_length = APP_TELEM_PACKET_TYPE_3_LEN;//sizeof(telem_packet_3_struct)-1; //-1 for end of struct character (off by one due to data alignmet in memory!)
		telem_datagram.packet_type = 3;
		telem_datagram.rf_source = 0;
		telem_datagram.data_length = telem_packet_length;

		//TEMPORARY HAB APPLICATION DATA
		//unsigned char uChar_HAB_Data[7];
		unsigned char uChar_HAB_Packet_Status;
		uChar_HAB_Packet_Status = application_hab_create_telem_3_pkt(&telem_packet_3_struct.HAB_DATA);

		//Copy packet Type 1 into packet datagram
		memcpy(&telem_datagram.data, &telem_packet_3_struct,telem_packet_length);

		//Error detection 16 bit calculation
		int_to_byte_array(&telem_datagram.error_detection_16, compute_checksum_16(APP_TELEM_PACKET_LEN-2, &telem_datagram));

		//Copy full packet into pointer buffer
		volatile unsigned char sizetelem = sizeof(telem_datagram);
		memcpy(&packet[0], &telem_datagram, sizetelem);
}

void application_send_telemetry_packet_1(void){
	unsigned char telem_packet_buffer[123]; //It is very important that this is the correct size, if too small the buffer write will overflow and be upredicatable to system performance
	application_telem_create_pkt_1(telem_packet_buffer);
	application_send_uart_packet(&telem_packet_buffer);
	//application_send_uart_packet((unsigned char *)telem.temp_1);
	__no_operation();
}

void application_send_telemetry_device_debug(void){
	unsigned char telem_packet_buffer[123]; //It is very important that this is the correct size, if too small the buffer write will overflow and be upredicatable to system performance
	application_telem_create_pkt_2(telem_packet_buffer);
	application_send_uart_packet(&telem_packet_buffer);
	//application_send_uart_packet((unsigned char *)telem.temp_1);
	__no_operation();
}

void application_send_telemetry_3(char *string, unsigned char callsignlength, unsigned char ID){
	unsigned char telem_packet_buffer[123]; //It is very important that this is the correct size, if too small the buffer write will overflow and be upredicatable to system performance
	application_telem_create_pkt(telem_packet_buffer, local_callsign, local_callsign_len, local_device_id, local_callsign, local_callsign_len, local_device_id);
	application_send_uart_packet(&telem_packet_buffer);
	//application_send_uart_packet((unsigned char *)telem.temp_1);
	__no_operation();
}


void application_update_RTC_calender_gps(void){

	char buff[2];
	int year,month,day,hour,min,sec;

	memcpy(buff,(const char*)RMC.Date + 4 * CHARSIZE, 2 * CHARSIZE);
	year = 2000 + atoi(buff);
	RTCYEAR = year;
	memcpy(buff,(const char*)RMC.Date + 2 * CHARSIZE, 2 * CHARSIZE);
	month = atoi(buff);
	RTCMON = month;
	memcpy(buff,(const char*)RMC.Date, 2 * CHARSIZE);
	day = atoi(buff);
	RTCDAY = day;
	memcpy(buff,(const char*)GGA.Time, 2 * CHARSIZE);
	hour = atoi(buff);
	RTCHOUR = hour;
	memcpy(buff,(const char*)GGA.Time + 2 * CHARSIZE, 2 * CHARSIZE);
	min = atoi(buff);
	RTCMIN = min;
	memcpy(buff,(const char*)GGA.Time + 4 * CHARSIZE, 2 * CHARSIZE);
	sec = atoi(buff);
	RTCSEC = sec;

	struct tm timeinfo;

	timeinfo.tm_year = year - 1900;
	timeinfo.tm_mon = month - 1;
	timeinfo.tm_mday = day;
	timeinfo.tm_hour = hour;
	timeinfo.tm_min = min;
	timeinfo.tm_sec = sec;
	timeinfo.tm_isdst = -1;
	mktime(&timeinfo);

	RTCDOW = timeinfo.tm_wday;
}

//void telemetry_rf_transport_rx(void){ //Probably should pass by reference! BUG
//	__no_operation();
//	telem_application_rx_rf_parse();
//}
//
//void telem_application_rx_rf_parse(void){
//
//
//
//	test_structrx.payload_length = rf_transport_packet_rx_struct.payload[0];
//	test_structrx.service_number = rf_transport_packet_rx_struct.payload[1];
//	unsigned char i;
//
//	for(i=2;i<30;i++){
//		test_structrx.payload[i] = rf_transport_packet_rx_struct.payload[i];
//	}
//	//Application payload to UART command
//	if(test_structrx.service_number == 5){
//		__no_operation();
//		//Send over uart
//		//faraday_simple_transport_send_uart(&test_structrx.payload[2], test_structrx.payload_length, 5);
//
//	}
//}

void rf_application_tx_rf(unsigned char count){

	test_structtx.payload_length = 28;
	test_structtx.service_number = 5;
	unsigned char i;
	unsigned char testpkt[28] = "This is a test of Message #";
	testpkt[27] = count;
	for(i=0;i<28;i++){
		test_structtx.payload[i]= testpkt[i];
	}
	//unsigned char testpkt[114] = "This is a test of long datagrams. My callsign is KB1LQD and I\'m creating a new RF modem! This should fragment. 123";

	//rf_transport_tx_create_packet(&test_structtx.payload_length, 30, 0xBB);
	//rf_transport_tx_fragment(45);
	//rf_transport_tx_state_machine(); //Watchout, transport forces 128 bytes to 3 packets which at more than 1 (i.e. 2) thats 6 packets total would overflow.
}

void application_telemetry_initialize(void){
	//initialize FIFO's
	init_app_telem_fifo();
	//application_load_defaults();
}
void init_app_telem_fifo(void){
	fifo_sram_init(&app_telem_state_machine, 0, APP_TELEM_PACKET_LEN, APP_TELEM_PACKET_FIFO_COUNT);
	fifo_sram_init(&app_telem_uart_state_machine, 126, APP_TELEM_PACKET_LEN, 1);
	fifo_sram_init(&rf_rx_telem_state_machine, 189, TELEM_FIFO_SIZE, TELEM_FIFO_ELEMENT_CNT);
}

void application_send_uart_packet(unsigned char *packet){
	//put_fifo(&app_telem_state_machine, &app_telem_state_machine_fifo_buffer, packet);
	put_fifo_sram(&app_telem_state_machine, packet);
}

void application_load_defaults(void){
	unsigned char * flash_ptr;
	flash_ptr = (unsigned char *) 0x1821;
	//app_telem_boot_bits = *flash_ptr;
}

void application_telemetry_send_self(void){
	application_send_telemetry_3(local_callsign, local_callsign_len, local_device_id); // Send Faraday callsign and unit ID metadata
}

void application_telemetry_send_device_debug_telem(void){
	application_send_telemetry_device_debug();
}

//application_telemetry_interval_increment(void){
//	telemetry_interval_counter_int++;
//}

void application_telemetry_uart_housekeeping_interval(void){
	//Check if bitmask allows UART telemetry OR if telemetry interval == 0 (if 0 then disable telemetry UART actions)
	if(check_bitmask(telem_boot_bitmask,UART_BEACON_BOOT_ENABLE) && (telem_default_uart_interval != 0)){
		//Check if interval counter reached
		if((telemetry_interval_counter_int>=telem_default_uart_interval) || (telem_default_uart_interval == 1)){
			//Reset counter
			telemetry_interval_counter_int = 0;
			//send uart local telemetry
			application_telemetry_send_self();
		}
		else{
			telemetry_interval_counter_int++;
		}
	}

	else{
		//Nothing
	}

}

#endif /* APPLICATIONS_APPLICATIONS_C_ */
