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

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <asm/ioctls.h>
#include <linux/fs.h>
#include <linux/serial.h>
#include <linux/tty.h>
//#include <signal.h>
#include <sys/types.h>
#include <time.h>

#define ECU_QUERY_CMD           0x78
#define TCU_QUERY_CMD           0x45
#define FWS_QUERY_CMD           0x92
#define RESET_CMD               0x12
#define POLL_DELAY		50000000
#define MAX_WAIT		10		/* Multiples of POLL_DELAY */
#define MAX_RETRIES             10
#define TTY_BUFSIZE		4096

struct serial_struct SerialInfo, OldSerialInfo;
struct termios Termios, OldTermios;

int fd;
unsigned char response[3];
int indx=0;
volatile int receive_flag=0;
volatile int semW;
volatile char *Query_buffer=NULL;
volatile int Query_address=0, Query_index=0, Query_max=0;

volatile int Returned_address;
volatile unsigned char Returned_value;

int sigskip;

unsigned char last_command;

void psleep(long long nsec)
{
	struct timespec req,rem;

	req.tv_sec = 0;
	rem.tv_sec = 0;
	req.tv_nsec = nsec;

	while (req.tv_nsec > 0)
	{
		rem.tv_nsec = 0;
		nanosleep(&req,&rem);
		req.tv_nsec = rem.tv_nsec;
	}
}

/*=============================================================*/
/* process_response() is called for each complete response     */
/* packet received from the ECU                                */
/*=============================================================*/

void process_response()
{
	unsigned char lsb, msb;

	msb = response[0];
	lsb = response[1];

	Returned_address = (msb << 8) | lsb;
	Returned_value   = response[2];

	if ((Returned_address == Query_address) && (Query_buffer != NULL)
	&&  (Query_index < Query_max))
	{
		Query_buffer[Query_index++] = Returned_value;
	}
}

/*=============================================================*/
/* SIGIOhandler() is called whenever data arrives from the ECU */
/* The input has to be processed asyncronously because there   */
/* is no flow control.                                         */
/*=============================================================*/

void SIGIOhandler (int status)
{
	unsigned char data[TTY_BUFSIZE];
	int count,i;

	/*--------------------------------------------------*/
	/* Don't process if this sig happens during a write */
	/*--------------------------------------------------*/

	if (semW != 0) return;

	/*---------------------------------------------------------*/
	/* Don't process on every sig, otherwise we get some wierd */
	/* situation where nanosleep never returns.                */
	/*---------------------------------------------------------*/

	if (sigskip-- > 0) return;
	sigskip=2;
	
	/*---------------------------------*/
	/* read the data from the COM port */
	/*---------------------------------*/

	count = read(fd, data, TTY_BUFSIZE);
	if (count > 0) receive_flag = 1;

	/*-----------------------------------*/
	/* Put it in the response buffer and */
	/* process each completed response   */
	/*-----------------------------------*/

	for (i=0;i<count;i++)
	{	
		response[indx]=data[i];
		indx = (indx + 1) % 3;
		if (indx == 0)
		{
			process_response();
		}
	}
}

/*========================================================================*/
/* ssm_open opens the serial port and configures it for access to the SSM */
/*========================================================================*/

int ssm_open(char *device)
{
	struct sigaction saio;
	int baud;

	/*----------------------*/
	/* Open the device file */
	/*----------------------*/

	if ((fd = open(device, O_RDWR)) < 0)
	{
                return -1;
        }

	/*-------------------------*/
	/* Get TTY driver settings */
	/*-------------------------*/
	
	if (tcgetattr(fd, &Termios) < 0)
	{
		close(fd);
                return -4;
	}

	OldTermios = Termios;

	/*-------------------------------------------------------------*/
	/* Make the read() call timeout after 0.1 seconds has elapsed. */
	/*-------------------------------------------------------------*/
	
	cfmakeraw(&Termios);	  
	Termios.c_cc[ VTIME ]=1;
	Termios.c_cc[ VMIN  ]=0;

	/*--------------------------------------------------------*/
	/* Must set TTY baud to 38400 when using custom baud rate */
	/*--------------------------------------------------------*/
	
	cfsetispeed(&Termios, B38400);
	cfsetospeed(&Termios, B38400);
	
	/*-----------------------*/
	/* Set tty driver to 8E1 */
	/*-----------------------*/

	Termios.c_cflag |=  CS8;	// 8 Data bits
 	Termios.c_cflag |=  PARENB;	// Parity enable 
	Termios.c_cflag &= ~PARODD;   	// Even (not odd) parity
 	Termios.c_cflag &= ~CSTOPB;	// 1 (not two) stopbits
 	Termios.c_cflag |=  CLOCAL;	// No Flow Control

	/*--------------------------------------*/
	/* Flush out any junk in the TTY buffer */
	/*--------------------------------------*/
	
	if (tcflush(fd, TCIFLUSH) < 0)
	{
		close(fd);
		return -5;
	};

	/*-----------------------------------*/
	/* Apply the new TTY driver settings */
	/*-----------------------------------*/

        if (tcsetattr(fd, TCSANOW, &Termios) < 0)
	{
		close(fd);
                return -6;
	}

	/*----------------------------*/
	/* Get serial driver settings */
	/*----------------------------*/

        if (ioctl(fd, TIOCGSERIAL, &SerialInfo) < 0) {
		close(fd);
                return -2;
        }

	OldSerialInfo = SerialInfo;
	
	/*---------------------------------*/
	/* Calculate divisor for 1953 baud */
	/*---------------------------------*/

	baud = 1953;

	SerialInfo.flags |= ASYNC_SPD_CUST;
	SerialInfo.custom_divisor = (SerialInfo.baud_base + ( baud / 2 )) / baud;
	/*-------------------------------*/
	/* Update serial driver settings */ 
	/*-------------------------------*/

 	if (ioctl(fd, TIOCSSERIAL, &SerialInfo) < 0)
	{
		close(fd);
                return -3;
        }

	/*------------------------*/
	/* Install Signal Handler */
	/*------------------------*/
	
	sigskip=2;
	semW=0;	

	saio.sa_handler  = SIGIOhandler;
        saio.sa_flags    = 0;
        saio.sa_restorer = NULL;
        sigemptyset(&saio.sa_mask);

        sigaction(SIGIO,&saio,NULL);
	
	/*------------------------------------*/
	/* allow the process to receive SIGIO */
	/*------------------------------------*/
	
	fcntl(fd, F_SETOWN, getpid());
	fcntl(fd, F_SETFL, FASYNC);

	return 0;
}

/*========================================================================*/
/* ssm_close resets the serial port and tty drivers to their original     */
/* settings and then closes the device file.                              */
/*========================================================================*/

int ssm_close()
{

	int rc, code;
	struct sigaction saio;

	/*---------------------------*/
	/* Send reset command to ECU */
	/*---------------------------*/

	rc=ssm_reset();
	if ((code == 0) && (rc != 0)) code=-1;

	/*-------------------------------*/
	/* Restore default SIGIO handler */
	/*-------------------------------*/

	saio.sa_handler  = SIG_DFL;
        saio.sa_flags    = 0;
        saio.sa_restorer = NULL;
        sigemptyset(&saio.sa_mask);

        sigaction(SIGIO,&saio,NULL);

	/*----------------------------------*/
	/* Restore original serial settings */
	/*----------------------------------*/

	rc=ioctl(fd, TIOCSSERIAL, &OldSerialInfo);
	if ((code == 0) && (rc != 0)) code=-2;

	/*------------------------*/
	/* Flush TTY input buffer */
	/*------------------------*/

	rc=tcflush(fd, TCIFLUSH);
	if ((code == 0) && (rc != 0)) code=-3;

	/*-------------------------------*/
	/* Restore original TTY settings */
	/*-------------------------------*/

        rc=tcsetattr(fd,TCSANOW,&OldTermios);
	if ((code == 0) && (rc != 0)) code=-4;

	/*-----------------------*/
	/* Close the device file */
	/*-----------------------*/

	rc = close(fd);
	if ((code == 0) && (rc != 0)) code=-5;

	return code;
}

/*===================================================================*/
/* send_reset() sends the reset command to the ECU and then checks   */
/* to see if the ECU has done it or not                              */
/*===================================================================*/

int send_reset()
{
	int rc, timeout;
	unsigned char cmd[4];
	
	/*------------------------*/
	/* Send the reset command */
	/*------------------------*/
	
	cmd[0]=RESET_CMD;
	cmd[1]=0x00;
	cmd[2]=0x00;
	cmd[3]=0x00;

	semW = 1;
	rc=write(fd,cmd,4);
	semW = 0;

	if (rc != 4) return -1;		/* Write error */

	/*--------------------------*/
	/* Wait for the SSM to STFU */
	/*--------------------------*/
	
	timeout=0;
	receive_flag=0;
	//nanosleep(&stfu_delay,NULL);
	psleep(POLL_DELAY);

	while ((receive_flag != 0) && (timeout++ < MAX_WAIT))
	{
		receive_flag=0;
		psleep(POLL_DELAY);
	}

	return 0;
}

/*===================================================*/
/* ssm_reset() commands the ECU to stop sending data */
/*===================================================*/

int ssm_reset()
{
	int rc, retries;
	
	/*------------------------*/
	/* Send the reset command */
	/*------------------------*/
	
	rc=send_reset();
	if (rc != 0) return rc;

	/*--------------------------------------------------------------*/
	/* If the ECU is still sending data then it may have missed the */
	/* reset command, so try again.                                 */
	/*--------------------------------------------------------------*/

	retries=0;
	while ((receive_flag != 0) && (retries++ < MAX_RETRIES))
	{
		rc=send_reset();
		if (rc != 0) return rc;
	}

	if (receive_flag != 0)
	{
		return -2;	/* MAX_RETRIES exceeded */
	}

	indx=0;
	
	return 0;
}

/*====================================================================*/
/* send_query() sends the query command and then waits for the ECU to */
/* start returning data from that address                             */
/*====================================================================*/

int send_query(unsigned char query_command)
{
	unsigned char msb, lsb, cmd[4];
	int rc,timeout;

	last_command=query_command;

	/*--------------------*/
	/* Send Query command */
	/*--------------------*/

	msb = (Query_address & 0xFF00) >> 8;
	lsb = (Query_address & 0x00FF);

	cmd[0]=query_command;
	cmd[1]=msb;
	cmd[2]=lsb;
	cmd[3]=0x00;
	
	semW = 1;
	rc = write(fd, cmd, 4);
	semW = 0;

	if (rc != 4) return -1;		/* Write error */

	/*--------------------------------------------------------*/
	/* Wait for ECU to start reporting data from that address */
	/*--------------------------------------------------------*/

	timeout=0;

	while ((Returned_address != Query_address) && (timeout++ < MAX_WAIT))
	{
		psleep(POLL_DELAY);
	}

	return 0;

}

/*==========================================================================*/
/* ssm_query_ecu() tells the ECU to start sending the contents of a given   */
/* address in it's RAM. It will read the address n times and store the data */
/* in the buffer so that you can analyse how the data changes over time.    */
/*==========================================================================*/

int ssm_query_ecu(int address, __u_char *buffer, int n)
{
	int rc, retries, timeout;

	Query_buffer  = buffer;
	Query_address = address;
	Query_max     = n;
	Query_index   = 0;

	if (last_command != ECU_QUERY_CMD)
	{
		rc=ssm_reset();
	}
	
	rc=send_query(ECU_QUERY_CMD);

	if (rc != 0)
	{
		Query_buffer=NULL;
		return rc;
	}

	/*------------------------------------------------------------*/
	/* If the ECU is still reporting data for a different address */
	/* then it may have missed the query command, so try again    */
	/*------------------------------------------------------------*/

	retries=0;
	while ((Returned_address != Query_address) && (retries++ < MAX_RETRIES))
	{
		rc=ssm_reset();
		rc=send_query(ECU_QUERY_CMD);
		if (rc != 0)
		{
			Query_buffer=NULL;
			return rc;
		}
	}

	if (Returned_address != Query_address)
	{
		Query_buffer=NULL;
		return -2;		/* MAX_RETRIES exceeded */
	}

	/*--------------------------------------*/
	/* Now wait for the buffer to be filled */
	/*--------------------------------------*/

	timeout=0;
	receive_flag=0;
	while ((Query_index < Query_max) && (timeout < MAX_WAIT))
	{
		psleep(POLL_DELAY);
		if (receive_flag == 0)
		{
			timeout++;
		}
		else
		{
			timeout=0;
			receive_flag=0;
		} 
	}

	if (Query_index < Query_max)
	{	
		Query_buffer=NULL;
		return -3;		/* TIMEOUT waiting for data */
	}

	Query_buffer=NULL;
	return 0;
}

/*==========================================================================*/
/* ssm_query_tcu() tells the TCU to start sending the contents of a given   */
/* address in it's RAM. It will read the address n times and store the data */
/* in the buffer so that you can analyse how the data changes over time.    */
/*==========================================================================*/

int ssm_query_tcu(int address, __u_char *buffer, int n)
{
	int rc, retries, timeout;

	Query_buffer  = buffer;
	Query_address = address;
	Query_max     = n;
	Query_index   = 0;

	if (last_command != TCU_QUERY_CMD)
	{
		rc=ssm_reset();
	}

	rc=send_query(TCU_QUERY_CMD);
	
	if (rc != 0)
	{
		Query_buffer=NULL;
		return rc;
	}

	/*------------------------------------------------------------*/
	/* If the TCU is still reporting data for a different address */
	/* then it may have missed the query command, so try again    */
	/*------------------------------------------------------------*/

	retries=0;
	while ((Returned_address != Query_address) && (retries++ < MAX_RETRIES))
	{
		rc=ssm_reset();
		rc=send_query(TCU_QUERY_CMD);
		if (rc != 0)
		{
			Query_buffer=NULL;
			return rc;
		}
	}

	if (Returned_address != Query_address)
	{
		Query_buffer=NULL;
		return -2;		/* MAX_RETRIES exceeded */
	}

	/*--------------------------------------*/
	/* Now wait for the buffer to be filled */
	/*--------------------------------------*/

	timeout=0;
	receive_flag=0;
	while ((Query_index < Query_max) && timeout < MAX_WAIT)
	{
		psleep(POLL_DELAY);
		if (receive_flag == 0)
		{
			timeout++;
		}
		else
		{
			timeout=0;
			receive_flag=0;
		} 
	}

	if (Query_index < Query_max)
	{	
		Query_buffer=NULL;
		return -3;		/* TIMEOUT waiting for data */
	}

	Query_buffer=NULL;
	return 0;
}

/*==========================================================================*/
/* ssm_query_4ws() tells the 4WS to start sending the contents of a given   */
/* address in it's RAM. It will read the address n times and store the data */
/* in the buffer so that you can analyse how the data changes over time.    */
/*==========================================================================*/

int ssm_query_4ws(int address, __u_char *buffer, int n)
{
	int rc, retries, timeout;

	Query_buffer  = buffer;
	Query_address = address;
	Query_max     = n;
	Query_index   = 0;

	if (last_command != FWS_QUERY_CMD)
	{
		rc=ssm_reset();
		Query_address = 0xFFFF;
		rc=send_query(FWS_QUERY_CMD);
		Query_address = address;
	}

	rc=send_query(FWS_QUERY_CMD);
	
	if (rc != 0)
	{
		Query_buffer=NULL;
		return rc;
	}

	/*------------------------------------------------------------*/
	/* If the 4WS is still reporting data for a different address */
	/* then it may have missed the query command, so try again    */
	/*------------------------------------------------------------*/

	retries=0;
	while ((Returned_address != Query_address) && (retries++ < MAX_RETRIES))
	{
		rc=send_query(FWS_QUERY_CMD);
		if (rc != 0)
		{
			Query_buffer=NULL;
			return rc;
		}
	}

	if (Returned_address != Query_address)
	{
		Query_buffer=NULL;
		return -2;		/* MAX_RETRIES exceeded */
	}

	/*--------------------------------------*/
	/* Now wait for the buffer to be filled */
	/*--------------------------------------*/

	timeout=0;
	receive_flag=0;
	while ((Query_index < Query_max) && timeout < MAX_WAIT)
	{
		psleep(POLL_DELAY);
		if (receive_flag == 0)
		{
			timeout++;
		}
		else
		{
			timeout=0;
			receive_flag=0;
		} 
	}

	if (Query_index < Query_max)
	{	
		Query_buffer=NULL;
		return -3;		/* TIMEOUT waiting for data */
	}

	Query_buffer=NULL;
	return 0;
}

/*==============================================================*/
/* write_byte() commands the ECU to write a byte of data at the */
/* given address                                                */
/*==============================================================*/

int write_byte(int address, unsigned char data)
{
	int rc, timeout;
	unsigned char msb, lsb, cmd[4],q;

	msb = (address & 0xFF00) >> 8;
	lsb = (address & 0x00FF);

	cmd[0]=0xAA;
	cmd[1]=msb;
	cmd[2]=lsb;
	cmd[3]=data;

	semW = 1;
	rc = write(fd, cmd, 4);
	semW = 0;

	if (rc != 4) return -4;

	/*--------------------------------------------------------*/
	/* Wait until ECU responds that the data has been written */
	/*--------------------------------------------------------*/

	timeout=0;
	while ((Returned_value != data) && (timeout++ < MAX_WAIT))
	{
		psleep(POLL_DELAY);
	}

	if (Returned_value != data)
	{
		return -5;	/* Timeout */
	}

	return 0;
}

/*=======================================================================*/
/* ssm_write_ecu() copies a byte from the buffer to the ECU starting at  */
/* the given address.                                                    */
/*=======================================================================*/

int ssm_write_ecu(int address, unsigned char data)
{
	int  rc=0;
	unsigned char tmp;

	/* Write only works if the ECU is responding to a query */
	
	rc=ssm_query_ecu(address,&tmp,1);
	if (rc != 0) return rc;

	rc=write_byte(address,data);

	return rc;
}

/*=======================================================================*/
/* ssm_write_tcu() copies a byte from the buffer to the ECU starting at  */
/* the given address.                                                    */
/*=======================================================================*/

int ssm_write_tcu(int address, unsigned char data)
{
	int  rc=0;
	unsigned char tmp;

	/* Write only works if the ECU is responding to a query */
	
	rc=ssm_query_tcu(address,&tmp,1);
	if (rc != 0) return rc;

	rc=write_byte(address,data);

	return rc;
}

/*====================================================*/
/* get_romid() send the get ROM ID command to the ECU */
/*====================================================*/

int get_romid(unsigned char value)
{
	unsigned char cmd[4];
	int rc,timeout;
	
	cmd[0]=0x00;
	cmd[1]='F';	// Fuji
	cmd[2]='H';	// Heavy
	cmd[3]='I';	// Industries

	semW = 1;
	rc = write(fd, cmd, 4);
	semW = 0;

	if (rc != 4) return -4;

	/*--------------------------------------------------------*/
	/* Wait until unit sends something that is not the result */
	/* of the query                                           */
	/*--------------------------------------------------------*/

	timeout=0;
	while ((Returned_value == value) && (timeout++ < MAX_WAIT))
	{
		psleep(POLL_DELAY);
	}
	return 0;
}

/*===============================================*/
/* ssm_romid_ecu() returns the ROM ID of the ECU */
/*===============================================*/

int ssm_romid_ecu(int *romid)
{
	int rc=0,retries;
	unsigned char value;

	/*------------------------------------------*/
	/* Get ROM ID command only works if the ECU */
	/* is responding to a query                 */
	/*------------------------------------------*/

	rc=ssm_query_ecu(0xFFFF,&value,1);
	if (rc != 0) return rc;

	rc=get_romid(value);
	
	if (rc != 0)
	{
		return rc;
	}

	/*------------------------------------------------------------*/
	/* If the ECU is still reporting data for a different address */
	/* then it may have missed the query command, so try again    */
	/*------------------------------------------------------------*/

	retries=0;
	while ((Returned_value == value) && (retries++ < MAX_RETRIES))
	{
		rc=get_romid(value);
		if (rc != 0)
		{
			return rc;
		}
	}

	if (Returned_value == value)
	{
		return -5;		/* MAX_RETRIES exceeded */
	}

	*romid=(response[0] << 16) | (response[1] << 8) | response[2];

	return 0;
	
}

/*===============================================*/
/* ssm_romid_tcu() returns the ROM ID of the TCU */
/*===============================================*/

int ssm_romid_tcu(int *romid)
{
	int rc=0,retries;
	unsigned char value;

	/*------------------------------------------*/
	/* Get ROM ID command only works if the ECU */
	/* is responding to a query                 */
	/*------------------------------------------*/

	rc=ssm_query_tcu(0xFFFF,&value,1);
	if (rc != 0) return rc;

	rc=get_romid(value);
	
	if (rc != 0)
	{
		return rc;
	}

	/*------------------------------------------------------------*/
	/* If the TCU is still reporting data for a different address */
	/* then it may have missed the query command, so try again    */
	/*------------------------------------------------------------*/

	retries=0;
	while ((Returned_value == value) && (retries++ < MAX_RETRIES))
	{
		rc=get_romid(value);
		if (rc != 0)
		{
			return rc;
		}
	}

	if (Returned_value == value)
	{
		return -5;		/* MAX_RETRIES exceeded */
	}

	*romid=(response[0] << 16) | (response[1] << 8) | response[2];

	return 0;
	
}

/*===============================================*/
/* ssm_romid_4ws() returns the ROM ID of the 4WS */
/*===============================================*/

int ssm_romid_4ws(int *romid)
{
	int rc=0,retries;
	unsigned char value;

	/*------------------------------------------*/
	/* Get ROM ID command only works if the ECU */
	/* is responding to a query                 */
	/*------------------------------------------*/

	rc=ssm_query_4ws(0xFFFF,&value,1);
	if (rc != 0) return rc;

	rc=get_romid(value);
	
	if (rc != 0)
	{
		return rc;
	}

	/*------------------------------------------------------------*/
	/* If the 4WS is still reporting data for a different address */
	/* then it may have missed the query command, so try again    */
	/*------------------------------------------------------------*/

	retries=0;
	while ((Returned_value == value) && (retries++ < MAX_RETRIES))
	{
		rc=get_romid(value);
		if (rc != 0)
		{
			return rc;
		}
	}

	if (Returned_value == value)
	{
		return -5;		/* MAX_RETRIES exceeded */
	}

	*romid=(response[0] << 16) | (response[1] << 8) | response[2];

	return 0;
	
}

/*==============================================================*/
/* ssm_current returns the data currently being sent by the ECU */
/*==============================================================*/

int ssm_current(int *address,char *data)
{
	int timeout=0;

	receive_flag=0;

	while ((receive_flag == 0) && (timeout < MAX_WAIT))
	{
		psleep(POLL_DELAY);
		timeout++;
	}

	if (receive_flag == 0) return -1;
	
	*address = Returned_address;
	*data    = Returned_value;

	return 0;
}

