/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * examples/hello/hello_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <pthread.h>

#include <tinyara/pwm.h>
#include <tinyalsa/tinyalsa.h>

#include <media/media_recorder.h>
#include <media/media_player.h>
#include <media/media_utils.h>

#include <apps/shell/tash.h>
#include <slsi_wifi/slsi_wifi_utils.h>
#include <protocols/dhcpc.h>
#include <netutils/netlib.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>  
#include <sys/select.h>

#define BUFF_SIZE 8192

const char CMD_WIFI[]="wifi";
const char ARG_START[]="startsta";
const char ARG_JOIN[]="join";

const char ARG_DEFALUTAP[]="FSRNT 2.4G";
const char ARG_DEFALUTPWD[]="75657565";

const char CMD_IF[]="ifconfig";
const char ARG_WL1[]="wl1";
const char ARG_DHCP[]="dhcp";

uint8_t buf[BUFF_SIZE];

static int pwmfd = -1;

struct _WavHeader{
	uint32_t ChunkID;			//4
	uint32_t ChunkSize;			//4
	uint32_t Format;				//4
	uint32_t fmt;					//4
	uint32_t molla;				//4
	uint16_t AudioFormat;		//2
	uint16_t Channel;			//2
	uint32_t SampleRate;		//4
	uint32_t ByteRate;			//4
	uint16_t Block_Align;		//2
	uint16_t BitPerSample;		//2
	uint32_t molla2;
	uint32_t molla3;
}WavHeader;

static void codec_start(void);	//마스터클럭 클럭발생함수
static void codec_stop(void);		//마스터 클럭 정지 함수

//pthread_t SockDownloadThread;
int sock;	//소켓번호

/****************************************************************************
 * hello_main
 ****************************************************************************/
//#define MEDIARECORD
#define PLAYER_PCM

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int streawav_main(int argc, char *argv[])
#endif
{
	struct sockaddr_in serv_addr;
	const char* cmd[4]={CMD_WIFI,ARG_START};
	char *data_size;
	struct pcm *ppc;	//PCM 번호
	struct pcm_config config; //PCM 설정
	int read_size= 0;
	long totalReadBytes= 0;
	int recv_file_size, readBytes, ret;
	int readed;
	int stopflag= 0;
	//int remain;

	//uint8_t *ptr_Play,*ptr_Down;
	//Wifi 연결코드

	tash_execute_cmd(cmd,2);
	cmd[1]=ARG_JOIN;
	cmd[2]=ARG_DEFALUTAP;
	cmd[3]=ARG_DEFALUTPWD;
	tash_execute_cmd(cmd,4);

	cmd[0]=CMD_IF;
	cmd[1]=ARG_WL1;
	cmd[2]=ARG_DHCP;
	tash_execute_cmd(cmd,3);

	//소켓 연결코드
	sock = socket(PF_INET,SOCK_STREAM,0);
	if(sock==-1){
		printf("sock error\n");return 0;
	}
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr("192.168.219.216");
	serv_addr.sin_port = htons(5555);

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0){
		printf("connect error\n");return 0;
	}
	printf("Socket connected successfully.\n");

	data_size= (char *)malloc(32);
	readBytes= recv(sock, data_size, 32, 0);
	recv_file_size = atol(data_size);
	printf("file size >> %d\n", recv_file_size);
	free(data_size);
	
	codec_start();
	
	write(sock,"had",3);fsync(sock);
	readBytes = recv(sock, (void*)&WavHeader, 44, 0);
	totalReadBytes += readBytes;
	printf("totalReadBytes= %d\n", totalReadBytes);
	
	config.channels = WavHeader.Channel;
	config.rate = WavHeader.SampleRate;
	config.format = PCM_FORMAT_S16_LE;
	config.period_size = CONFIG_AUDIO_BUFSIZE;
	config.period_count = CONFIG_AUDIO_NUM_BUFFERS;
	
	ppc = pcm_open(0, 0, PCM_OUT, &config);
	read_size= pcm_frames_to_bytes(ppc, pcm_get_buffer_size(ppc)); 
	readed= 0;

	while(1){
		readed= 0;
		
		write(sock, "ack", 3); fsync(sock);
		while(readed<read_size){
			if(totalReadBytes == recv_file_size){
				stopflag= 1;
				break;
			}
			readBytes = recv(sock, buf + readed, read_size - readed, 0);
			readed += readBytes;
			totalReadBytes += readBytes;
		}
	
		printf("readed= %d\n", readed);
		printf("totalReadBytes= %d\n", totalReadBytes);

		if(stopflag == 1){
			ret= pcm_write(ppc, buf, sizeof(buf));
			pcm_drain(ppc);
			write(sock, "end", 3); fsync(sock);
			break;
		}
		ret = pcm_write(ppc, buf, readed);	
	}

	printf("finish\n");
	//PCM Close
	pcm_close(ppc);

	//소켓클로즈
	close(sock);
	codec_stop();

	return 0;
}

static void codec_start(void)
{
	struct pwm_info_s pwm_info;

	pwmfd = open("/dev/pwm5", O_RDWR);

	if (pwmfd > 0) {
		pwm_info.frequency = 6500000;
		pwm_info.duty = 50 * 65536 / 100;
		ioctl(pwmfd, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&pwm_info));
		ioctl(pwmfd, PWMIOC_START);
	}
}

static void codec_stop(void)
{
	if (pwmfd > 0) {
		ioctl(pwmfd, PWMIOC_STOP);
		close(pwmfd);
	}
}

