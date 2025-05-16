//
//    **                                                **
//   ******************************************************
//  ***  THIS PROGRAM IS THE PROPERTY OF ALCYONE LIMITED ***
//   ******************************************************
//    **                                                **
//
//  ssm.h by Phil Skuse
//  Copyright (C) Alcyone Limited 2007.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//


/*=====================================================================*/
/* ALL OF THESE FUNCTIONS RETURN ZERO ON SUCCESS AND NON-ZERO ON ERROR */
/*=====================================================================*/


/*---------------------------------------------------------------------*/
//  ssm_open() opens the COM port and configures it to talk to the car */
/*---------------------------------------------------------------------*/
int ssm_open(char *device);

/*------------------------------------------------*/
/* ssm_reset() tells the car to stop sending data */
/*------------------------------------------------*/
 
int ssm_reset();

/*--------------------------------------------------------------------------*/
/* ssm_query_ecu() tells the ECU to start sending the contents of a given   */
/* address in it's RAM. It will read the address n times and store the data */
/* in the buffer so that you can analyse how the data changes over time.    */
/*--------------------------------------------------------------------------*/

int ssm_query_ecu(int address, __u_char *buffer, int n);

/*--------------------------------------------------------------------------*/
/* ssm_query_tcu() tells the TCU to start sending the contents of a given   */
/* address in it's RAM. It will read the address n times and store the data */
/* in the buffer so that you can analyse how the data changes over time.    */
/*--------------------------------------------------------------------------*/

int ssm_query_tcu(int address, __u_char *buffer, int n);

/*--------------------------------------------------------------------------*/
/* ssm_query_4ws() tells the 4WS to start sending the contents of a given   */
/* address in it's RAM. It will read the address n times and store the data */
/* in the buffer so that you can analyse how the data changes over time.    */
/*--------------------------------------------------------------------------*/

int ssm_query_4ws(int address, __u_char *buffer, int n);

/*-----------------------------------------------------*/
/* ssm_current() returns the data currently being sent */
/*-----------------------------------------------------*/

int ssm_current(int *address,__u_char *data);
	
/*---------------------------------------*/ 
// ssm_close() shuts down the connection */
/*---------------------------------------*/ 
 
int ssm_close();

/*--------------------------------------------------*/
/* ssm_write_ecu() writes a byte of data to the ECU */
/* ssm_write_tcu() writes a byte of data to the TCU */
/*--------------------------------------------------*/

int ssm_write_ecu(int address,unsigned char data);
int ssm_write_tcu(int address,unsigned char data);

/*-----------------------------------------------*/
/* ssm_romid_ecu() returns the ROM ID of the ECU */
/* ssm_romid_tcu() returns the ROM ID of the TCU */
/*-----------------------------------------------*/

int ssm_romid_ecu(int *romid);
int ssm_romid_tcu(int *romid);
int ssm_romid_4ws(int *romid);

