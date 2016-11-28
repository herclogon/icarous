/**
 * Interface
 * 
 * This is a class that will enable establishing a socket or
 * a serial interface between ICAROUS and any component that can
 * stream MAVLink messages
 *
 * Contact: Swee Balachandran (swee.balachandran@nianet.org)
 * 
 * 
 * Copyright (c) 2011-2016 United States Government as represented by
 * the National Aeronautics and Space Administration.  No copyright
 * is claimed in the United States under Title 17, U.S.Code. All Other
 * Rights Reserved.
 *
 * Notices:
 *  Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space Administration. 
 *  All rights reserved.
 *     
 * Disclaimers:
 *  No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER EXPRESSED, 
 *  IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS, ANY
 *  IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT, 
 *  ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, 
 *  WILL CONFORM TO THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN ENDORSEMENT BY GOVERNMENT 
 *  AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS, RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS 
 *  RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY DISCLAIMS ALL WARRANTIES AND 
 *  LIABILITIES REGARDING THIRD-PARTY SOFTWARE, IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
 *
 * Waiver and Indemnity:  
 *   RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST THE UNITED STATES GOVERNMENT, 
 *   ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL AS ANY PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE 
 *   RESULTS IN ANY LIABILITIES, DEMANDS, DAMAGES,
 *   EXPENSES OR LOSSES ARISING FROM SUCH USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM, 
 *   RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD HARMLESS THE UNITED STATES GOVERNMENT, 
 *   ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.  
 *   RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE, UNILATERAL TERMINATION OF THIS AGREEMENT.
 */

#include "Interface.h"

Interface::Interface(MAVLinkInbox* msgs){
    pthread_mutex_init(&lock, NULL);
    RcvdMessages = msgs;
}

int Interface::GetMAVLinkMsg(){

    int n = ReadData();
    mavlink_message_t message;
    mavlink_status_t status;
 
    bool msgReceived = false;

    for(int i=0;i<n;i++){
        uint8_t cp = recvbuffer[i];
        msgReceived = mavlink_parse_char(MAVLINK_COMM_1, cp, &message, &status);

        if(msgReceived){
            RcvdMessages->DecodeMessage(message);
        }
    }

    return n;
}

void Interface::SendMAVLinkMsg(mavlink_message_t msg){
    uint16_t len = mavlink_msg_to_send_buffer(sendbuffer, &msg);
    WriteData(sendbuffer,len);
}

uint8_t* Interface::GetRecvBuffer(){
    return recvbuffer;
}


SerialInterface::SerialInterface(char name[],int brate,int pbit,MAVLinkInbox *msgs)
:Interface(msgs){

  portname = name;
  baudrate = brate;
  parity   = pbit;
  
  fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0)
  {
    printf("Error operning port");
    return;
  }
  
  set_interface_attribs ();            // set baudrate, 8n1 (no parity)
  set_blocking (0);                    // set no blocking

}

int SerialInterface::set_interface_attribs()
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("error in tcgetattr 1");
        return -1;
    }

    cfsetospeed (&tty, baudrate);
    cfsetispeed (&tty, baudrate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);  // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        printf("error from tcsetattr 2");
        return -1;
    }
    return 0;
}

void SerialInterface::set_blocking (int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("error from tcsetattr 3");
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;                      // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        printf("error from tcsetattr 4");
	
}

int SerialInterface::ReadData(){
    
    char buf;
    int n = 0;
    pthread_mutex_lock(&lock);
    n = read (fd, &buf, 1);
    recvbuffer[0] = buf;
    pthread_mutex_lock(&lock);

    return n;
}

void SerialInterface::WriteData(uint8_t buffer[],uint16_t len){

    pthread_mutex_lock(&lock);
    write(fd,buffer,len);
    pthread_mutex_unlock(&lock);
}


// Socet interface class definition
SocketInterface::SocketInterface(char targetip[], int inportno, int outportno,MAVLinkInbox *msgs)
:Interface(msgs){

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(&locAddr, 0, sizeof(locAddr));
    locAddr.sin_family      = AF_INET;
    locAddr.sin_addr.s_addr = INADDR_ANY;
    locAddr.sin_port        = htons(inportno);
    
    /* Bind the socket to port 14551 - necessary to receive packets from qgroundcontrol */ 
    if (-1 == bind(sock,(struct sockaddr *)&locAddr, sizeof(struct sockaddr))){
      printf("error: bind failed");
      close(sock);
      exit(EXIT_FAILURE);
    } 
    
    if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0){
      printf("error setting nonblocking\n");
      close(sock);
      exit(EXIT_FAILURE);
    }
    
    memset(&targetAddr, 0, sizeof(targetAddr));
    targetAddr.sin_family      = AF_INET;
    targetAddr.sin_addr.s_addr = inet_addr(targetip);
    targetAddr.sin_port        = htons(outportno);
    
}

int SocketInterface::ReadData(){
    memset(recvbuffer, 0, BUFFER_LENGTH);
    int n = 0;
    pthread_mutex_lock(&lock);
    n = recvfrom(sock, (void *)recvbuffer, BUFFER_LENGTH, 0, (struct sockaddr *)&targetAddr, &recvlen);
    pthread_mutex_unlock(&lock);
    return n;
}

void SocketInterface::WriteData(uint8_t buffer[],uint16_t len){
    pthread_mutex_lock(&lock);
    sendto(sock, buffer, len, 0, (struct sockaddr*)&targetAddr, sizeof (struct sockaddr_in));
    pthread_mutex_unlock(&lock);
}
