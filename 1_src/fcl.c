/*****************************************************************************************************************************
This is a project on 3Dsim, based on ssdsim under the framework of the completion of structures, the main function:
1.Support for 3D commands, for example:mutli plane\interleave\copyback\program suspend/Resume..etc
2.Multi - level parallel simulation
3.Clear hierarchical interface
4.4-layer structure

FileName�� fcl.c
Author: Zuo Lu 		Version: 2.0	Date:2017/02/07
Description:
fcl layer: remove other high-level commands, leaving only mutli plane;

History:
<contributor>		<time>			<version>       <desc>													<e-mail>
Zuo Lu				2017/04/06	      1.0		    Creat 3Dsim											lzuo@hust.edu.cn
Zuo Lu				2017/05/12		  1.1			Support advanced commands:mutli plane					lzuo@hust.edu.cn
Zuo Lu				2017/06/12		  1.2			Support advanced commands:half page read				lzuo@hust.edu.cn
Zuo Lu				2017/06/16		  1.3			Support advanced commands:one shot program				lzuo@hust.edu.cn
Zuo Lu				2017/06/22		  1.4			Support advanced commands:one shot read					lzuo@hust.edu.cn
Zuo Lu				2017/07/07		  1.5			Support advanced commands:erase suspend/resume			lzuo@hust.edu.cn
Zuo Lu				2017/07/24		  1.6			Support static allocation strategy						lzuo@hust.edu.cn
Zuo Lu				2017/07/27		  1.7			Support hybrid allocation strategy						lzuo@hust.edu.cn
Zuo Lu				2017/08/17		  1.8			Support dynamic stripe allocation strategy				lzuo@hust.edu.cn
Zuo Lu				2017/10/11		  1.9			Support dynamic OSPA allocation strategy				lzuo@hust.edu.cn
Jin Li				2018/02/02		  1.91			Add the allocation_method								li1109@hust.edu.cn
Ke wang/Ke Peng		2018/02/05		  1.92			Add the warmflash opsration								296574397@qq.com/2392548402@qq.com
Hao Lv				2018/02/06		  1.93			Solve gc operation bug 									511711381@qq.com
Zuo Lu				2018/02/07        2.0			The release version 									lzuo@hust.edu.cn
***********************************************************************************************************************************************/

#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>

#include "flash.h"
#include "ssd.h"
#include "initialize.h"
#include "buffer.h"
#include "interface.h"
#include "ftl.h"
#include "fcl.h"

extern int secno_num_per_page, secno_num_sub_page;

/**************************************************************************
*This function is also only deal with read requests, processing chip current state is CHIP_WAIT,
*Or the next state is CHIP_DATA_TRANSFER and the next state of the expected time is less than the current time of the chip
***************************************************************************/
Status services_2_r_data_trans(struct ssd_info * ssd, unsigned int channel)
{
	unsigned int chip = 0;
	unsigned int sub_r_count, i = 0;
	unsigned int aim_die;

	struct sub_request ** sub_r_request = NULL;
	sub_r_request = (struct sub_request **)malloc((ssd->parameter->plane_die * PAGE_INDEX) * sizeof(struct sub_request *));
	alloc_assert(sub_r_request, "sub_r_request");
	for (i = 0; i < (ssd->parameter->plane_die * PAGE_INDEX); i++)
		sub_r_request[i] = NULL;

	for (chip = 0; chip < ssd->channel_head[channel].chip; chip++)
	{
		if ((ssd->channel_head[channel].chip_head[chip].current_state == CHIP_DATA_TRANSFER) ||
			((ssd->channel_head[channel].chip_head[chip].next_state == CHIP_DATA_TRANSFER) &&
			(ssd->channel_head[channel].chip_head[chip].next_state_predict_time <= ssd->current_time)))
		{
			for (aim_die = 0; aim_die < ssd->parameter->die_chip; aim_die++)
			{
				if (ssd->parameter->flash_mode == TLC_MODE)
				{
					//�����ж��Ƿ��и߼�����oneshot_mulitplane_read�ĸ߼������ִ��
					if ((ssd->parameter->advanced_commands&AD_MUTLIPLANE) == AD_MUTLIPLANE && (ssd->parameter->advanced_commands&AD_ONESHOT_READ) == AD_ONESHOT_READ)
					{
						sub_r_count = find_read_sub_request(ssd, channel, chip, aim_die, sub_r_request, SR_R_DATA_TRANSFER, ONE_SHOT_READ_MUTLI_PLANE);
						if (sub_r_count == (ssd->parameter->plane_die * PAGE_INDEX))
						{
							go_one_step(ssd, sub_r_request, sub_r_count, SR_R_DATA_TRANSFER, ONE_SHOT_READ_MUTLI_PLANE);
							ssd->channel_head[channel].channel_busy_flag = 1;
							break;
						}
					}

					//�����ܣ����ж��ܷ���one shot read�ĸ߼�����ȥ��
					if ((ssd->parameter->advanced_commands&AD_ONESHOT_READ) == AD_ONESHOT_READ)
					{
						sub_r_count = find_read_sub_request(ssd, channel, chip, aim_die, sub_r_request, SR_R_DATA_TRANSFER, ONE_SHOT_READ);
						if (sub_r_count == PAGE_INDEX)
						{
							ssd->one_shot_read_count++;
							go_one_step(ssd, sub_r_request, sub_r_count, SR_R_DATA_TRANSFER, ONE_SHOT_READ);
							ssd->channel_head[channel].channel_busy_flag = 1;
							break;
						}
					}
				}
				//�����ܣ����ж��ܷ���mutli plane�ĸ߼�����ȥ��
				if ((ssd->parameter->advanced_commands&AD_MUTLIPLANE) == AD_MUTLIPLANE)
				{
					sub_r_count = find_read_sub_request(ssd, channel, chip, aim_die, sub_r_request, SR_R_DATA_TRANSFER, MUTLI_PLANE);
					if ( (sub_r_count > 1) && (sub_r_count <= ssd->parameter->plane_die))
					{
						go_one_step(ssd, sub_r_request, sub_r_count, SR_R_DATA_TRANSFER, MUTLI_PLANE);
						ssd->channel_head[channel].channel_busy_flag = 1;
						break;
					}
				}

				//�߼������֧�֣���ʹ����ͨ��
				sub_r_count = find_read_sub_request(ssd, channel, chip, aim_die, sub_r_request, SR_R_DATA_TRANSFER, NORMAL);
				if (sub_r_count == 1)
				{
					go_one_step(ssd, sub_r_request, sub_r_count, SR_R_DATA_TRANSFER, NORMAL);
					ssd->channel_head[channel].channel_busy_flag = 1;
					break;
				}
				else 
					ssd->channel_head[channel].channel_busy_flag = 0;
			}
		}

		if (ssd->channel_head[channel].channel_busy_flag == 1)
			break;
	}
	for (i = 0; i < (ssd->parameter->plane_die * PAGE_INDEX); i++)
		sub_r_request[i] = NULL;
	free(sub_r_request);
	sub_r_request = NULL;
	return SUCCESS;
}

Status services_2_r_read(struct ssd_info * ssd)
{
	unsigned int i,j,subs_count = 0,aim_die;
	unsigned int mask;

	struct sub_request ** subs = NULL;
	subs = (struct sub_request **)malloc((ssd->parameter->plane_die*PAGE_INDEX) * sizeof(struct sub_request *));
	alloc_assert(subs, "subs");
	for (i = 0; i < (ssd->parameter->plane_die*PAGE_INDEX); i++)
		subs[i] = NULL;

	mask = ~(0xffffffff << (ssd->parameter->subpage_page));
	for (i = 0; i < ssd->parameter->channel_number; i++)                                    
	{
		for (j = 0; j < ssd->parameter->chip_channel[i]; j++)
		{
			if ((ssd->channel_head[i].chip_head[j].current_state == CHIP_READ_BUSY) ||
				((ssd->channel_head[i].chip_head[j].next_state == CHIP_READ_BUSY) &&
				(ssd->channel_head[i].chip_head[j].next_state_predict_time <= ssd->current_time)))
			{
				for (aim_die = 0; aim_die < ssd->parameter->die_chip; aim_die++)
				{					
					if (ssd->parameter->flash_mode == TLC_MODE)
					{
						//�����ж��Ƿ��и߼�����oneshot_mulitplane_read�ĸ߼������ִ��
						if ((ssd->parameter->advanced_commands&AD_MUTLIPLANE) == AD_MUTLIPLANE && (ssd->parameter->advanced_commands&AD_ONESHOT_READ) == AD_ONESHOT_READ)
						{
							subs_count = find_read_sub_request(ssd, i, j, aim_die, subs, SR_R_READ, ONE_SHOT_READ_MUTLI_PLANE);
							if (subs_count == (ssd->parameter->plane_die * PAGE_INDEX))
							{
								go_one_step(ssd, subs, subs_count, SR_R_READ, ONE_SHOT_READ_MUTLI_PLANE);
								break;
							}
						}

						//�����ܣ����ж��ܷ���one shot read�ĸ߼�����ȥ��
						if ((ssd->parameter->advanced_commands&AD_ONESHOT_READ) == AD_ONESHOT_READ)
						{
							subs_count = find_read_sub_request(ssd, i, j, aim_die, subs, SR_R_READ, ONE_SHOT_READ);
							if (subs_count == PAGE_INDEX)
							{

								go_one_step(ssd, subs, subs_count, SR_R_READ, ONE_SHOT_READ);
								break;
							}
						}
					}
					//�����ܣ����ж��ܷ���mutli plane�ĸ߼�����ȥ��
					if ((ssd->parameter->advanced_commands&AD_MUTLIPLANE) == AD_MUTLIPLANE)
					{
						subs_count = find_read_sub_request(ssd, i, j, aim_die, subs, SR_R_READ, MUTLI_PLANE);
						if ((subs_count >1) && (subs_count <= ssd->parameter->plane_die))
						{
							go_one_step(ssd, subs, subs_count, SR_R_READ, MUTLI_PLANE);
							break;
						}
					}

					//������и߼������֧�֣��������ͨ�Ķ���half page read
					subs_count = find_read_sub_request(ssd, i, j, aim_die, subs, SR_R_READ, NORMAL);
					if (subs_count == 1)
					{
						if ((ssd->parameter->advanced_commands&AD_HALFPAGE_READ) == AD_HALFPAGE_READ)
						{
							if ((subs[0]->state&mask) == 0x000c || (subs[0]->state&mask) == 0x0003)
								go_one_step(ssd, subs, subs_count, SR_R_READ, HALF_PAGE);
							else
								go_one_step(ssd, subs, subs_count, SR_R_READ, NORMAL);
						}
						else
							go_one_step(ssd, subs, subs_count, SR_R_READ, NORMAL);

						break;
					}
				}
			}
		}
	}

	for (i = 0; i < (ssd->parameter->plane_die*PAGE_INDEX); i++)
		subs[i] = NULL;
	free(subs);
	subs = NULL;

	return SUCCESS;
}

/**************************************************************************************
*Function function is given in the channel, chip, die above looking for reading requests
*The request for this child ppn corresponds to the ppn of the corresponding plane's register
*****************************************************************************************/
unsigned int find_read_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die, struct sub_request ** subs, unsigned int state, unsigned int command)
{
	struct sub_request * sub = NULL;
	unsigned int add_reg, i, j = 0;

	for (i = 0; i < (ssd->parameter->plane_die * PAGE_INDEX); i++)
		subs[i] = NULL;
	
	j = 0;
	sub = ssd->channel_head[channel].subs_r_head;
	while (sub != NULL)
	{
		if (sub->location->chip == chip && sub->location->die == die)
		{
			if (sub->next_state == state && sub->next_state_predict_time <= ssd->current_time)
			{
				if (command == ONE_SHOT_READ_MUTLI_PLANE)
				{
					if (sub->oneshot_mutliplane_flag == 1)
					{
						subs[j] = sub;
						j++;
					}
					if (j == (ssd->parameter->plane_die * PAGE_INDEX))
						break;
				}
				else if (command == ONE_SHOT_READ)
				{
					if (sub->oneshot_flag == 1)
					{
						subs[j] = sub;
						j++;
					}
					if (j == PAGE_INDEX)
						break;
				}
				else if (command == MUTLI_PLANE)
				{
					if (sub->mutliplane_flag == 1)
					{
						subs[j] = sub;
						j++;
						/*
						add_reg = ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].add_reg_ppn;
						if (sub->ppn == add_reg)
						{
							subs[j] = sub;
							j++;
						}
						else
						{
							printf("error add_reg is wrong!\n");
							getchar();
						}*/
					}
					if (j == ssd->parameter->plane_die)
						break;
				}
				else if (command == NORMAL)
				{
					if ((sub->oneshot_mutliplane_flag != 1) && (sub->oneshot_flag != 1) && (sub->mutliplane_flag != 1))
					{
						subs[j] = sub;
						j++;
						/*
						add_reg = ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].add_reg_ppn;
						if (sub->ppn == add_reg)
						{
							subs[j] = sub;
							j++;
						}
						else
						{
							printf("error add_reg is wrong!\n");
							getchar();
						}
						*/
					}
					if (j == 1)
						break; 
				}
			}
		}
		sub = sub->next_node;
	}
 

	if (j > (ssd->parameter->plane_die * PAGE_INDEX))
	{
		printf("error,beyong plane_die* PAGE_INDEX\n");
		getchar();
	}

	return j;
}

/*********************************************************************************************
* function that specifically serves a read request
*1��Only when the current state of the sub request is SR_R_C_A_TRANSFER
*2��The current state of the read request is SR_COMPLETE or the next state is SR_COMPLETE and 
*the next state arrives less than the current time
**********************************************************************************************/
Status services_2_r_complete(struct ssd_info * ssd)
{
	unsigned int i = 0;
	struct sub_request * sub = NULL, *p = NULL;
	
	for (i = 0; i<ssd->parameter->channel_number; i++)                                       /*This loop does not require the channel time, when the read request is completed, it will be removed from the channel queue*/
	{
		sub = ssd->channel_head[i].subs_r_head;
		p = NULL;
		while (sub != NULL)
		{
			if ((sub->current_state == SR_COMPLETE) || ((sub->next_state == SR_COMPLETE) && (sub->next_state_predict_time <= ssd->current_time)))
			{
				if (sub != ssd->channel_head[i].subs_r_head)                         
				{
					if (sub == ssd->channel_head[i].subs_r_tail)
					{
						ssd->channel_head[i].subs_r_tail = p;
						p->next_node = NULL;
					}
					else
					{
						p->next_node = sub->next_node;
						sub = p->next_node;
					}
				}
				else
				{
					if (ssd->channel_head[i].subs_r_head != ssd->channel_head[i].subs_r_tail)
					{
						ssd->channel_head[i].subs_r_head = sub->next_node;
						sub = sub->next_node;
						p = NULL;
					}
					else
					{
						ssd->channel_head[i].subs_r_head = NULL;
						ssd->channel_head[i].subs_r_tail = NULL;
						break;
					}
				}
			}
			else
			{
				p = sub;
				sub = sub->next_node;
			}
			
		}
	}

	return SUCCESS;
}


/*****************************************************************************************
*This function is also a service that only reads the child request, and is in a wait state
******************************************************************************************/
Status services_2_r_wait(struct ssd_info * ssd, unsigned int channel)
{
	struct sub_request ** sub_place = NULL;
	unsigned int sub_r_req_count, i ,chip;

	sub_place = (struct sub_request **)malloc(ssd->parameter->plane_die * PAGE_INDEX * sizeof(struct sub_request *));
	alloc_assert(sub_place, "sub_place");
	sub_r_req_count = 0;
	for (chip = 0; chip < ssd->parameter->chip_channel[channel]; chip++)
	{
		/*************************************************************************************************************************************/
		//�ж��յ���gc�źţ������resume���źţ�����лָ���������
		if (ssd->channel_head[channel].chip_head[chip].gc_signal != SIG_NORMAL)
		{
			if ((ssd->channel_head[channel].chip_head[chip].current_state == CHIP_IDLE) || ((ssd->channel_head[channel].chip_head[chip].next_state == CHIP_IDLE) &&
				(ssd->channel_head[channel].chip_head[chip].next_state_predict_time <= ssd->current_time)))
			{
				if (ssd->current_time >= ssd->channel_head[channel].chip_head[chip].erase_cmplt_time)
				{
					if (ssd->channel_head[channel].chip_head[chip].gc_signal == SIG_ERASE_SUSPEND)
					{
						//��ʣ��Ĳ��������ƶ�chip��ʱ����
						ssd->channel_head[channel].chip_head[chip].current_state = CHIP_ERASE_BUSY;
						ssd->channel_head[channel].chip_head[chip].next_state = CHIP_IDLE;
						ssd->channel_head[channel].chip_head[chip].next_state_predict_time += ssd->channel_head[channel].chip_head[chip].erase_rest_time;

						resume_erase_operation(ssd, channel, chip);
						
						ssd->resume_count++;
						ssd->channel_head[channel].channel_busy_flag = 1;
						ssd->channel_head[channel].chip_head[chip].gc_signal = SIG_NORMAL;
						continue;
					}
					else
					{
						resume_erase_operation(ssd, channel, chip);
						ssd->channel_head[channel].channel_busy_flag = 0;
						ssd->channel_head[channel].chip_head[chip].gc_signal = SIG_NORMAL;
					}
				}
			}
		}
		/***************************************************************************************************************************************/
		if (ssd->parameter->flash_mode == TLC_MODE)							 //ֻ����tlc mode �²��ܽ���one shot mutli plane read/one shot read
		{
			//�ж��Ƿ�����ø߼�����oneshot_mutliplane_read
			if ((ssd->parameter->advanced_commands&AD_ONESHOT_READ) == AD_ONESHOT_READ && (ssd->parameter->advanced_commands&AD_MUTLIPLANE) == AD_MUTLIPLANE)
			{
				sub_r_req_count = find_r_wait_sub_request(ssd, channel, chip, sub_place, ONE_SHOT_READ_MUTLI_PLANE);
				if (sub_r_req_count == (PAGE_INDEX*ssd->parameter->plane_die))
				{
					go_one_step(ssd, sub_place, sub_r_req_count, SR_R_C_A_TRANSFER, ONE_SHOT_READ_MUTLI_PLANE);
					ssd->channel_head[channel].channel_busy_flag = 1;
					continue;
				}
			}
			//�ж��ܷ���one shot read�߼��������
			if ((ssd->parameter->advanced_commands&AD_ONESHOT_READ) == AD_ONESHOT_READ)
			{
				sub_r_req_count = find_r_wait_sub_request(ssd, channel, chip, sub_place, ONE_SHOT_READ);
				if (sub_r_req_count == PAGE_INDEX)
				{

					for (i = sub_r_req_count; i < (ssd->parameter->plane_die * PAGE_INDEX); i++)
						sub_place[i] = NULL;

					go_one_step(ssd, sub_place, sub_r_req_count, SR_R_C_A_TRANSFER, ONE_SHOT_READ);
					ssd->channel_head[channel].channel_busy_flag = 1;
					continue;
				}
			}
		}
		//�ж��ܷ���mutli plane�߼��������
		if ((ssd->parameter->advanced_commands&AD_MUTLIPLANE) == AD_MUTLIPLANE)
		{
			sub_r_req_count = find_r_wait_sub_request(ssd, channel, chip, sub_place, MUTLI_PLANE);
			if ((sub_r_req_count >1) && (sub_r_req_count <= ssd->parameter->plane_die))
			{
				for (i = sub_r_req_count; i < (ssd->parameter->plane_die * PAGE_INDEX); i++)
					sub_place[i] = NULL;

				go_one_step(ssd, sub_place, sub_r_req_count, SR_R_C_A_TRANSFER, MUTLI_PLANE);
				ssd->channel_head[channel].channel_busy_flag = 1;
				continue;
			}
		}

		//�����ܣ���ʾ���еĸ߼���������У���ȥִ����ͨ�Ķ�����,����ͨ�Ķ�����δ�ҵ����򷵻أ���chip����Ч������ִ��
		sub_r_req_count = find_r_wait_sub_request(ssd, channel, chip, sub_place, NORMAL);
		if (sub_r_req_count == 1)
		{
			for (i = sub_r_req_count; i < (ssd->parameter->plane_die * PAGE_INDEX); i++)
				sub_place[i] = NULL;

			go_one_step(ssd, sub_place, sub_r_req_count, SR_R_C_A_TRANSFER, NORMAL);
			ssd->channel_head[channel].channel_busy_flag = 1;
		}
		else if (sub_r_req_count == 0)
		{
			ssd->channel_head[channel].channel_busy_flag = 0;
		}
		
		/***************************************************************************************************************************************/
		//�ж��Ƿ���suspend�����������Ķ���������ǣ�����Щ��������ڶ�Ӧchannel��������
		if (ssd->channel_head[channel].chip_head[chip].gc_signal != SIG_NORMAL)
		{
			if (sub_r_req_count != 0)
			{
				if (ssd->channel_head[channel].chip_head[chip].gc_signal == SIG_ERASE_WAIT)
				{
					ssd->channel_head[channel].chip_head[chip].erase_rest_time = ssd->channel_head[channel].chip_head[chip].erase_cmplt_time - ssd->current_time;
					ssd->suspend_count++;
				}
				ssd->channel_head[channel].chip_head[chip].gc_signal = SIG_ERASE_SUSPEND;
				ssd->suspend_read_count += sub_r_req_count;
			}
		}
		/***************************************************************************************************************************************/

	}

	for (i = 0; i < (ssd->parameter->plane_die * PAGE_INDEX); i++)
		sub_place[i] = NULL;
	free(sub_place);
	sub_place = NULL;

	return SUCCESS;
}

//�ж������ǲ���suspend��block
Status check_req_in_suspend(struct ssd_info * ssd, unsigned int channel, unsigned int chip, struct sub_request * sub_plane_request)
{
	unsigned  int i;
	struct suspend_location * loc = NULL;

	loc = ssd->channel_head[channel].chip_head[chip].suspend_location;
	if (sub_plane_request->location->die == loc->die)
	{
		for (i = 0; i < ssd->parameter->plane_die; i++)
		{
			if ((sub_plane_request->location->plane == i) && (sub_plane_request->location->block == loc->block[i]))
				return FAILURE;
		}
	}
	return SUCCESS;
}

//Ѱ����one shot read���������
unsigned int find_r_wait_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, struct sub_request ** sub_place, unsigned int command)
{
	unsigned int i,j,flag;
	unsigned int aim_die, aim_plane, aim_block, aim_group;
	unsigned int sub_count, plane_flag;
	struct sub_request * sub_plane_request = NULL;

	//��ʼ��
	for (i = 0; i < (ssd->parameter->plane_die * PAGE_INDEX); i++)
		sub_place[i] = NULL;

	flag = 0;
	i = 0;
	sub_count = 0;
	sub_plane_request = ssd->channel_head[channel].subs_r_head;
	while (sub_plane_request != NULL)
	{
		if (sub_plane_request->current_state == SR_WAIT && sub_plane_request->location->chip == chip)
		{
			/*************************************************************************************/
			if (ssd->channel_head[channel].chip_head[chip].gc_signal != SIG_NORMAL)
			{
				if (check_req_in_suspend(ssd, channel, chip, sub_plane_request) == FAILURE)
				{
					sub_plane_request = sub_plane_request->next_node;
					continue;
				}
			}
			/*************************************************************************************/

		    if (command == ONE_SHOT_READ_MUTLI_PLANE)
			{
				if (sub_count % PAGE_INDEX == 0)
				{
					if (((ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].current_state == CHIP_IDLE) ||
						((ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].next_state == CHIP_IDLE) &&
						(ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].next_state_predict_time <= ssd->current_time))))
					{
						if (sub_count == 0)
						{
							aim_die = sub_plane_request->location->die;
							aim_plane = sub_plane_request->location->plane;
							aim_block = sub_plane_request->location->block;
							aim_group = sub_plane_request->location->page / PAGE_INDEX;
							i = 0;
							flag = 1;
						}
						else   //����һ��plane�Ѿ�Ѱ�����ˣ����濪ʼѰ�ҵڶ�����Ŀ��aimplane\aim_block
						{
							if (ssd->parameter->scheduling_algorithm == FCFS)
							{
								if ((sub_plane_request->location->plane != aim_plane) && (sub_place[i*PAGE_INDEX] != NULL))
								{
									aim_plane = sub_plane_request->location->plane;
									aim_block = sub_plane_request->location->block;
									i++;
									flag = 1;
								}
							}
						}
					}
				}
				if (flag == 1)
				{
					if (sub_plane_request->location->die == aim_die && sub_plane_request->location->plane == aim_plane && sub_plane_request->location->block == aim_block)
					{
						for (j = 0; j < PAGE_INDEX; j++)
						{
							if (sub_plane_request->location->page == (j + aim_group*PAGE_INDEX))
							{
								if (sub_place[j + (i*PAGE_INDEX)] != NULL)
								{
									printf("read the same page!\n");
									getchar();
								}
								sub_place[j + (i*PAGE_INDEX)] = sub_plane_request;
								sub_count++;
								break;
							}

						}
					}
					if (sub_count == (ssd->parameter->plane_die * PAGE_INDEX))
						break;
				}
			}
			else if (command == ONE_SHOT_READ)
			{
				if (flag == 0)
				{
					if (((ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].current_state == CHIP_IDLE) ||
						((ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].next_state == CHIP_IDLE) &&
						(ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].next_state_predict_time <= ssd->current_time))))
					{
						aim_die = sub_plane_request->location->die;
						aim_plane = sub_plane_request->location->plane;
						aim_block = sub_plane_request->location->block;
						aim_group = sub_plane_request->location->page / PAGE_INDEX;
						flag = 1;
					}
				}
				if (flag == 1)
				{
					if (sub_plane_request->location->die == aim_die && sub_plane_request->location->plane == aim_plane && sub_plane_request->location->block == aim_block)
					{
						for (j = 0; j < PAGE_INDEX; j++)
						{
							if (sub_plane_request->location->page == (j + aim_group*PAGE_INDEX))
							{
								if (sub_place[j] != NULL)
								{
									printf("read the same page!\n");
									getchar();
								}
								sub_place[j] = sub_plane_request;
								sub_count++;
								break;
							}

						}
					}
				}
			}
			else if (command == MUTLI_PLANE)
			{
				if (i == 0)
				{
					if (((ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].current_state == CHIP_IDLE) ||
						((ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].next_state == CHIP_IDLE) &&
						(ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].next_state_predict_time <= ssd->current_time))))
					{
						sub_place[0] = sub_plane_request;
						i++;
						sub_count++;
					}
				}
				else
				{
					if ((sub_place[0]->location->chip == sub_plane_request->location->chip) &&
						(sub_place[0]->location->die == sub_plane_request->location->die) &&
						(sub_place[0]->location->page == sub_plane_request->location->page))
					{
						plane_flag = 0;
						for (j = 0; j < i; j++)
						{
							if (sub_place[j]->location->plane == sub_plane_request->location->plane)
								plane_flag = 1;
						}
						if (plane_flag == 0)
						{
							sub_place[i] = sub_plane_request;
							i++;
							sub_count++;
						}
					}
				}
				if (i == ssd->parameter->plane_die)
					break;
			}
			else if (command == NORMAL)
			{
				if (((ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].current_state == CHIP_IDLE) ||
					((ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].next_state == CHIP_IDLE) &&
					(ssd->channel_head[sub_plane_request->location->channel].chip_head[sub_plane_request->location->chip].next_state_predict_time <= ssd->current_time))))
				{
					sub_place[0] = sub_plane_request;
					sub_count = 1;
					break;
				}
			}
		}
		sub_plane_request = sub_plane_request->next_node;
	}

	if (sub_count > ssd->parameter->plane_die * PAGE_INDEX)
	{
		printf("error,beyong plane_die * PAGE_INDEX\n");
		getchar();
	}
	return sub_count;
}




/***********************************************************************************************************
*1.The state transition of the child request, and the calculation of the time, are handled by this function
*2.The state of the execution of the normal command, and the calculation of the time, are handled by this function
****************************************************************************************************************/
Status go_one_step(struct ssd_info * ssd, struct sub_request ** subs, unsigned int subs_count, unsigned int aim_state, unsigned int command)
{
	unsigned int i = 0, j = 0, k = 0, m = 0;
	long long read_time = 0;
	long long time = 0;
	unsigned int plane_number = 0;

	struct sub_request * sub = NULL;
	struct local * location = NULL;

	for (i = 0; i < subs_count; i++)
	{
		if (subs[i] == NULL)
		{
			printf("ERROR! no subs state jump\n");
			getchar();
			return ERROR;
		}
	}

	/***************************************************************************************************
	*When dealing with ordinary commands, the target state of the read request is divided into the following
	*cases: SR_R_READ, SR_R_C_A_TRANSFER, SR_R_DATA_TRANSFER
	*
	*The target status of the write request is only SR_W_TRANSFER
	****************************************************************************************************/
	if (command == NORMAL)
	{
		sub = subs[0];
		location = subs[0]->location;

		switch (aim_state)
		{
		case SR_R_C_A_TRANSFER:
		{
			/*******************************************************************************************************
			*When the target state is the command address transfer, the next state of sub is SR_R_READ
			*This state and channel, chip, so to modify the channel, chip status were CHANNEL_C_A_TRANSFER, CHIP_C_A_TRANSFER
			*The next status is CHANNEL_IDLE, CHIP_READ_BUSY
			*******************************************************************************************************/
			sub->current_time = ssd->current_time;
			sub->current_state = SR_R_C_A_TRANSFER;
			sub->next_state = SR_R_READ;
			sub->next_state_predict_time = ssd->current_time + 7 * ssd->parameter->time_characteristics.tWC;
			sub->begin_time = ssd->current_time;

			ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn = sub->ppn;

			ssd->channel_head[location->channel].current_state = CHANNEL_C_A_TRANSFER;
			ssd->channel_head[location->channel].current_time = ssd->current_time;
			ssd->channel_head[location->channel].next_state = CHANNEL_IDLE;
			ssd->channel_head[location->channel].next_state_predict_time = ssd->current_time + 7 * ssd->parameter->time_characteristics.tWC;

			ssd->channel_head[location->channel].chip_head[location->chip].current_state = CHIP_C_A_TRANSFER;
			ssd->channel_head[location->channel].chip_head[location->chip].current_time = ssd->current_time;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state = CHIP_READ_BUSY;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + 7 * ssd->parameter->time_characteristics.tWC;

			break;

		}
		case SR_R_READ:
		{
			/*****************************************************************************************************
			*This target state is flash in the state of reading data, sub the next state should be transmitted data SR_R_DATA_TRANSFER.
			*Then has nothing to do with the channel, only with the chip so to modify the chip status CHIP_READ_BUSY, the next state is CHIP_DATA_TRANSFER
			******************************************************************************************************/
			sub->current_time = ssd->current_time;
			sub->current_state = SR_R_READ;
			sub->next_state = SR_R_DATA_TRANSFER;
			sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.tR;

			ssd->read_count++;
			ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_read_count++;

			ssd->channel_head[location->channel].chip_head[location->chip].current_state = CHIP_READ_BUSY;
			ssd->channel_head[location->channel].chip_head[location->chip].current_time = ssd->current_time;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state = CHIP_DATA_TRANSFER;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.tR;
			
		
			break;
		}

		case SR_R_DATA_TRANSFER:
		{
			/**************************************************************************************************************
			*When the target state is data transfer, the next state of sub is the completion state. SR_COMPLETE
			*The state of the deal with the channel, chip, so channel, chip current state into CHANNEL_DATA_TRANSFER, CHIP_DATA_TRANSFER
			*The next state is CHANNEL_IDLE, CHIP_IDLE.
			***************************************************************************************************************/
			sub->current_time = ssd->current_time;
			sub->current_state = SR_R_DATA_TRANSFER;
			sub->next_state = SR_COMPLETE;
			sub->next_state_predict_time = ssd->current_time + (sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;
			sub->complete_time = sub->next_state_predict_time;

			ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn = -1;

			if (sub->update_read_flag == 1)
				sub->update_read_flag = 0;

			ssd->channel_head[location->channel].current_state = CHANNEL_DATA_TRANSFER;
			ssd->channel_head[location->channel].current_time = ssd->current_time;
			ssd->channel_head[location->channel].next_state = CHANNEL_IDLE;
			ssd->channel_head[location->channel].next_state_predict_time = sub->next_state_predict_time;

			ssd->channel_head[location->channel].chip_head[location->chip].current_state = CHIP_DATA_TRANSFER;
			ssd->channel_head[location->channel].chip_head[location->chip].current_time = ssd->current_time;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state = CHIP_IDLE;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = sub->next_state_predict_time;

			/*
			//�ж��Ƿ���suspend read
			if (sub->suspend_req_flag == SUSPEND_TYPE && ssd->channel_head[location->channel].chip_head[location->chip].gc_signal == SIG_ERASE_SUSPEND)
			{
				sub->suspend_req_flag = NORMAL_TYPE;
				if (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time >= ssd->channel_head[location->channel].chip_head[location->chip].erase_cmplt_time)
				{
					ssd->channel_head[location->channel].chip_head[location->chip].gc_signal = SIG_ERASE_RESUME;
				}
			}
			*/
			break;
		}
		case SR_W_TRANSFER:
		{
			/******************************************************************************************************
			*This is the time to deal with write requests, state changes, and time calculations
			*Write requests are from the top of the plane to transfer data, so that you can put a few states as a state
			*to deal with, as SR_W_TRANSFER this state to deal with, sub next state is complete state
			*At this time channel, chip current state into CHANNEL_TRANSFER, CHIP_WRITE_BUSY
			*The next state changes to CHANNEL_IDLE, CHIP_IDLE
			*******************************************************************************************************/
			sub->current_time = ssd->current_time;
			sub->current_state = SR_W_TRANSFER;
			sub->next_state = SR_COMPLETE;
			sub->next_state_predict_time = ssd->current_time + 7 * ssd->parameter->time_characteristics.tWC + 
			(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tPROG;
			
			sub->complete_time = sub->next_state_predict_time;
			time = sub->complete_time;

			ssd->channel_head[location->channel].current_state = CHANNEL_TRANSFER;
			ssd->channel_head[location->channel].current_time = ssd->current_time;
			ssd->channel_head[location->channel].next_state = CHANNEL_IDLE;
			ssd->channel_head[location->channel].next_state_predict_time = ssd->current_time + 7 * ssd->parameter->time_characteristics.tWC + (sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;

			ssd->channel_head[location->channel].chip_head[location->chip].current_state = CHIP_WRITE_BUSY;
			ssd->channel_head[location->channel].chip_head[location->chip].current_time = ssd->current_time;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state = CHIP_IDLE;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = time;

			break;
		}
		default:  return ERROR;
		}
	}
	else if ((command == MUTLI_PLANE) || (command == HALF_PAGE) || (command == ONE_SHOT_READ) || (command == ONE_SHOT_READ_MUTLI_PLANE))
	{
		/**********************************************************************************************
		*Advanced order MUTLI_PLANE processing, where the MUTLI_PLANE advanced command is a high-level command to read the child request
		*State transition and ordinary command, the difference is in SR_R_C_A_TRANSFER when the calculation of time is serial, because the sharing of a channel channel
		*Also SR_R_DATA_TRANSFER also share a channel
		**********************************************************************************************/
		location = subs[0]->location;

		switch (aim_state)
		{
			//��Ϊ�����ַ�Ǵ��еģ�������ʲô�߼�����Ǵ��д���ģ�ֻ�Ǵ���ĸ���subs_count��ͬ
			//ͬʱ������ִ�еĲ�ͬ�ĸ߼���������Ǹ��Դ��ϱ��λ
			case SR_R_C_A_TRANSFER:
			{
				for (i = 0; i < subs_count; i++)
				{
					//�����������ʱ���ߣ���ַ�Ĵ����Ǵ��е�
					if (i == 0)
						subs[i]->current_time = ssd->current_time;
					else
						subs[i]->current_time = subs[i - 1]->next_state_predict_time;

					subs[i]->current_state = SR_R_C_A_TRANSFER;
					subs[i]->next_state = SR_R_READ;
					subs[i]->next_state_predict_time = subs[i]->current_time + 7 * ssd->parameter->time_characteristics.tWC;
					subs[i]->begin_time = ssd->current_time;

					//���µ�ַ�Ĵ���
					ssd->channel_head[subs[i]->location->channel].chip_head[subs[i]->location->chip].die_head[subs[i]->location->die].plane_head[subs[i]->location->plane].add_reg_ppn = subs[i]->ppn;    //��Ҫд��ĵ�ַ���͵���ַ�Ĵ���

					//������������
					if (command == MUTLI_PLANE)
						subs[i]->mutliplane_flag = 1;
					else if (command == ONE_SHOT_READ)
						subs[i]->oneshot_flag = 1;
					else if (command == ONE_SHOT_READ_MUTLI_PLANE)
						subs[i]->oneshot_mutliplane_flag = 1;
				}
				i--;
				//����channel/chip��ʱ����
				ssd->channel_head[location->channel].current_state = CHANNEL_C_A_TRANSFER;
				ssd->channel_head[location->channel].current_time = ssd->current_time;
				ssd->channel_head[location->channel].next_state = CHANNEL_IDLE;
				ssd->channel_head[location->channel].next_state_predict_time = subs[i]->next_state_predict_time;   //muitli plane ����ַ����һ��channelͨ�����˵�ַ�����ʱ���Ǵ��е�

				ssd->channel_head[location->channel].chip_head[location->chip].current_state = CHIP_C_A_TRANSFER;
				ssd->channel_head[location->channel].chip_head[location->chip].current_time = ssd->current_time;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state = CHIP_READ_BUSY;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = subs[i]->next_state_predict_time;


				break;
			}
			case SR_R_READ:
			{
				//�߼�����Ĳ�ͬ��Զ����ʵ�ʱ�䲻ͬ����ν�ĸ߼�����Ҳ���Ƕ����ʵ�ʱ����٣������ݴ�������һ�µ�
				if (command == MUTLI_PLANE){
					read_time = ssd->parameter->time_characteristics.tR;
					ssd->m_plane_read_count++;
				}
				else if (command == HALF_PAGE){
					read_time = ssd->parameter->time_characteristics.tR * 0.5;
					ssd->half_page_read_count++;
				}
				else if (command == ONE_SHOT_READ){
					read_time = ssd->parameter->time_characteristics.tR * 0.8;
					ssd->one_shot_read_count++;
				}
				else if (command == ONE_SHOT_READ_MUTLI_PLANE){
					read_time = ssd->parameter->time_characteristics.tR * 0.8;
					ssd->one_shot_mutli_plane_count++;
				}

				//�����������ʱ����
				for (i = 0; i < subs_count; i++)
				{
					subs[i]->current_time = ssd->current_time;
					subs[i]->current_state = SR_R_READ;
					subs[i]->next_state = SR_R_DATA_TRANSFER;
					subs[i]->next_state_predict_time = subs[i]->current_time + read_time;

					//���¶������ļ���ֵ
					ssd->channel_head[subs[i]->location->channel].chip_head[subs[i]->location->chip].die_head[subs[i]->location->die].plane_head[subs[i]->location->plane].blk_head[subs[i]->location->block].page_read_count++;    //read��������ֵ����
					ssd->read_count++;

				}

				//����chip��ʱ����
				ssd->channel_head[location->channel].chip_head[location->chip].current_state = CHIP_READ_BUSY;
				ssd->channel_head[location->channel].chip_head[location->chip].current_time = ssd->current_time;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state = CHIP_DATA_TRANSFER;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + read_time;

				break;
			}
			case SR_R_DATA_TRANSFER:
			{
				//�߼�����Զ����������ݴ�����Ӱ�죬���ݴ���Ҳ�Ǵ��е�
				//�����������ʱ����
				for (i = 0; i < subs_count; i++)
				{
					if (i == 0)
						subs[i]->current_time = ssd->current_time;
					else
						subs[i]->current_time = subs[i - 1]->next_state_predict_time;

					subs[i]->current_state = SR_R_DATA_TRANSFER;
					subs[i]->next_state = SR_COMPLETE;
					subs[i]->next_state_predict_time = subs[i]->current_time + (subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;
					subs[i]->complete_time = subs[i]->next_state_predict_time;

					//����ַ�Ĵ������
					ssd->channel_head[subs[i]->location->channel].chip_head[subs[i]->location->chip].die_head[subs[i]->location->die].plane_head[subs[i]->location->plane].add_reg_ppn = -1;

					//pre read��ɣ�������ɵı�־λ
					if (subs[i]->update_read_flag == 1)
						subs[i]->update_read_flag = 0;

					/*
					//�ж��Ƿ���suspend read
					if (subs[i]->suspend_req_flag == SUSPEND_TYPE && ssd->channel_head[location->channel].chip_head[location->chip].gc_signal == SIG_ERASE_SUSPEND)
					{
						subs[i]->suspend_req_flag = NORMAL_TYPE;
						if (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time >= ssd->channel_head[location->channel].chip_head[location->chip].erase_cmplt_time)
						{
							ssd->channel_head[location->channel].chip_head[location->chip].gc_signal = SIG_ERASE_RESUME;
						}
					}
					*/
				}
				i--;
				//����channel/chip��ʱ����
				ssd->channel_head[location->channel].current_state = CHANNEL_DATA_TRANSFER;
				ssd->channel_head[location->channel].current_time = ssd->current_time;
				ssd->channel_head[location->channel].next_state = CHANNEL_IDLE;
				ssd->channel_head[location->channel].next_state_predict_time = subs[i]->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state = CHIP_DATA_TRANSFER;
				ssd->channel_head[location->channel].chip_head[location->chip].current_time = ssd->current_time;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state = CHIP_IDLE;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = subs[i]->next_state_predict_time;
				break;
			}
			default:  return ERROR;
		}
	}
	/*
	else if (command == HALF_PAGE)    //half page read�����Ǹ��Ķ����ʵ�ʱ�䣬����״̬����תΪ��ͨ״̬��ת
	{
		sub = subs[0];
		location = subs[0]->location;

		switch (aim_state)
		{
			case SR_R_READ:
			{
				sub->current_time = ssd->current_time;
				sub->current_state = SR_R_READ;
				sub->next_state = SR_R_DATA_TRANSFER;
				sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.tR/2;

				ssd->read_count++;
				ssd->half_page_read_count++;
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_read_count++;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state = CHIP_READ_BUSY;
				ssd->channel_head[location->channel].chip_head[location->chip].current_time = ssd->current_time;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state = CHIP_DATA_TRANSFER;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.tR/2;

				break;
			}
			default:
			{
				printf("\nERROR: Unexpected state !\n");
				return ERROR;
			}
		}

	}*/
	else
	{
		printf("\nERROR: Unexpected command !\n");
		return ERROR;
	}

	return SUCCESS;
}

/****************************************
Write the request function of the request
*****************************************/
Status services_2_write(struct ssd_info * ssd, unsigned int channel)
{
	int j = 0,i = 0;
	unsigned int chip_token = 0;
	struct sub_request *sub = NULL;

	/************************************************************************************************************************
	*Because it is dynamic allocation, all write requests hanging in ssd-> subs_w_head, that is, do not know which allocation before writing on the channel
	*************************************************************************************************************************/
	if (ssd->subs_w_head != NULL || ssd->channel_head[channel].subs_w_head != NULL)
	{
		//�ж�tlcģʽ�£�һ��process���������ϵ�������û��С��3�����󣬲���ִ��one��shot
		if (ssd->channel_head[channel].subs_w_head != NULL)
		{
			sub = ssd->channel_head[channel].subs_w_head;
			while (sub != NULL)
			{
				i++;	
				sub = sub->next_node;
			}
			if (ssd->parameter->flash_mode == TLC_MODE)
			{
				if (i < PAGE_INDEX)
					printf("\n less than 3 subs \n");
			}
		}
		
		//����flagȥ�жϵ��ױ���״̬ת��ִ�����ַ�ʽ
		if (ssd->parameter->allocation_scheme == DYNAMIC_ALLOCATION || ssd->parameter->allocation_scheme == HYBRID_ALLOCATION)
		{
			for (j = 0; j<ssd->channel_head[channel].chip; j++)							  //Traverse all the chips
			{
				if (ssd->subs_w_head == NULL)											  //The loop is stopped when the request is processed
				{
					break;
				}

				chip_token = ssd->channel_head[channel].token;                           
				if (ssd->channel_head[channel].channel_busy_flag == 0)
				{
					if ((ssd->channel_head[channel].chip_head[chip_token].current_state == CHIP_IDLE) || ((ssd->channel_head[channel].chip_head[chip_token].next_state == CHIP_IDLE) && (ssd->channel_head[channel].chip_head[chip_token].next_state_predict_time <= ssd->current_time)))
					{
						if (dynamic_advanced_process(ssd, channel, chip_token) == NULL)
							ssd->channel_head[channel].channel_busy_flag = 0;
						else   
							ssd->channel_head[channel].channel_busy_flag = 1;
					}
					ssd->channel_head[channel].token = (ssd->channel_head[channel].token + 1) % ssd->parameter->chip_channel[channel];  //The current chip is busy and jumps to the next chip execution
				}
			}
		}
		else if (ssd->parameter->allocation_scheme == STATIC_ALLOCATION)
		{
			for (j = 0; j < ssd->channel_head[channel].chip; j++)
			{
				if (ssd->channel_head[channel].subs_w_head == NULL)
					continue;

				if (ssd->channel_head[channel].channel_busy_flag == 0)
				{
					if ((ssd->channel_head[channel].chip_head[j].current_state == CHIP_IDLE) || ((ssd->channel_head[channel].chip_head[j].next_state == CHIP_IDLE) && (ssd->channel_head[channel].chip_head[j].next_state_predict_time <= ssd->current_time)))
					{
						if (dynamic_advanced_process(ssd, channel, j) == NULL)
							ssd->channel_head[channel].channel_busy_flag = 0;
						else
							ssd->channel_head[channel].channel_busy_flag = 1;
					}
				}
			}
		}
	}
	else
	{
		ssd->channel_head[channel].channel_busy_flag = 0;
	}
	return SUCCESS;
}

/****************************************************************************************************************************
*When ssd supports advanced commands, the function of this function is to deal with high-level command write request
*According to the number of requests, decide which type of advanced command to choose (this function only deal with write requests, 
*read requests have been assigned to each channel, so the implementation of the election between the corresponding command)
*****************************************************************************************************************************/
struct ssd_info *dynamic_advanced_process(struct ssd_info *ssd, unsigned int channel, unsigned int chip)
{
	unsigned int subs_count = 0;
	unsigned int update_count = 0;                                                                                                                     /*record which plane has sub request in static allocation*/
	struct sub_request *sub = NULL, *p = NULL;
	struct sub_request ** subs = NULL;
	unsigned int max_sub_num = 0, aim_subs_count;
	unsigned int die_token = 0, plane_token = 0;

	unsigned int mask = 0x00000001;
	unsigned int i = 0, j = 0, k = 0;
	unsigned int aim_die;

	max_sub_num = (ssd->parameter->die_chip)*(ssd->parameter->plane_die)*PAGE_INDEX;
	subs = (struct sub_request **)malloc(max_sub_num*sizeof(struct sub_request *));
	alloc_assert(subs, "sub_request");
	update_count = 0;
	for (i = 0; i < max_sub_num; i++)
		subs[i] = NULL;  
	

	//�����Ƕ�̬���仹�Ǿ�̬���䣬ѡ��ͬ�Ĺ��ص�
	if (ssd->parameter->allocation_scheme == DYNAMIC_ALLOCATION || ssd->parameter->allocation_scheme == HYBRID_ALLOCATION)
		sub = ssd->subs_w_head;
	else if (ssd->parameter->allocation_scheme == STATIC_ALLOCATION)
	{
		sub = ssd->channel_head[channel].subs_w_head;
		aim_die = sub->location->die;
	}

	//1.�����������ϵ�ǰ���е�����
	subs_count = 0;
	while ((sub != NULL) && (subs_count < max_sub_num))
	{
		if (sub->current_state == SR_WAIT)
		{
			if ((sub->update == NULL) || ((sub->update != NULL) && ((sub->update->current_state == SR_COMPLETE) || ((sub->update->next_state == SR_COMPLETE) && (sub->update->next_state_predict_time <= ssd->current_time)))))    //û����Ҫ��ǰ������ҳ
			{
				if (ssd->parameter->allocation_scheme == STATIC_ALLOCATION)								//����Ǿ�̬���䣬����Ҫ��֤�ҵ��Ŀ�������������ͬһ��channel,chip,die
				{
					if (sub->location->chip == chip && sub->location->die == aim_die)
					{
						subs[subs_count] = sub;
						subs_count++;
					}
				}
				else if (ssd->parameter->allocation_scheme == DYNAMIC_ALLOCATION)						//��̬���䣬����֪���߼�ҳ�ֵ�����һ���̶���ҳ������ǲ�ȷ����
				{
					//if (sub->location->chip == chip)
					//{
					//	subs[subs_count] = sub;
					//	subs_count++;
					//}
					subs[subs_count] = sub;
					subs_count++;
				}
				else if (ssd->parameter->allocation_scheme == HYBRID_ALLOCATION)						//�����׽ڵ��location�ж�����ȫ��̬��д�뻹��stripe��д��
				{
					if (ssd->subs_w_head->location->channel == -1)
					{
						if (sub->location->chip == -1 && sub->location->channel == -1)
						{
							subs[subs_count] = sub;
							subs_count++;
						}
					}
					else
					{
						if (sub->location->chip == chip && sub->location->channel == channel)
						{
							subs[subs_count] = sub;
							subs_count++;
						}
					}
				}
			}
		}
		if (sub->update_read_flag == 1)
			update_count++;

		sub = sub->next_node;
	}

	//2.�Ҳ������е�������򷵻�null,��ʾ��ǰû������ִ��
	if (subs_count == 0)
	{
		for (i = 0; i < max_sub_num; i++)
			subs[i] = NULL;
		free(subs);
		subs = NULL;
		return NULL;
	}

	//3.����������������������������������еĹ���	
	if (update_count > ssd->update_sub_request)
		ssd->update_sub_request = update_count;

	//�������¶�����ȣ���trace�ļ���ȡ����
	if (update_count > ssd->parameter->update_reqeust_max)
	{
		printf("update sub request is full!\n");
		ssd->buffer_full_flag = 1;  //blcok the buffer
	}
	else
		ssd->buffer_full_flag = 0;
		
	//4.���ݲ�ͬģʽ��ѡ���Ӧ�ĸ߼��������
	if (ssd->parameter->flash_mode == SLC_MODE)
	{
		if ((ssd->parameter->advanced_commands&AD_MUTLIPLANE) == AD_MUTLIPLANE)
		{
			aim_subs_count = ssd->parameter->plane_die;
			service_advance_command(ssd, channel, chip, subs, subs_count, aim_subs_count, MUTLI_PLANE);
		}
		else   //����֧�ָ߼������ʱ��ʹ��one page  program
		{
			for (i = 1; i<subs_count; i++)
			{
				subs[i] = NULL;
			}
			subs_count = 1;
			get_ppn_for_normal_command(ssd, channel, chip, subs[0]);
			//printf("lz:normal program\n");
			//getchar();
		}
	}
	else if (ssd->parameter->flash_mode == TLC_MODE)
	{
		if ((ssd->parameter->advanced_commands&AD_ONESHOT_PROGRAM) == AD_ONESHOT_PROGRAM)
		{
			if ((ssd->parameter->advanced_commands&AD_MUTLIPLANE) == AD_MUTLIPLANE)
			{
				aim_subs_count = ssd->parameter->plane_die * PAGE_INDEX;
				service_advance_command(ssd, channel, chip, subs, subs_count, aim_subs_count, ONE_SHOT_MUTLI_PLANE);
			}
			else
			{
				aim_subs_count = PAGE_INDEX;
				service_advance_command(ssd, channel, chip, subs, subs_count, aim_subs_count, ONE_SHOT);
			}
		}
		else
		{
			printf("Error! tlc mode match advanced commamd failed!\n");
			getchar();
		}
	}

	//5.������ɣ��ͷ���������ռ䣬������ssd�ṹ�壬��ʾ������ִ��
	for (i = 0; i < max_sub_num; i++)
	{
		subs[i] = NULL;
	}
	free(subs);
	subs = NULL;
	return ssd;
}


//���ݲ�ͬ�ĸ߼�����ȥ��������ĸ���
Status service_advance_command(struct ssd_info *ssd, unsigned int channel, unsigned int chip, struct sub_request ** subs, unsigned int subs_count, unsigned int aim_subs_count, unsigned int command)
{
	unsigned int i = 0;
	unsigned int max_sub_num;
	struct sub_request *sub = NULL, *p = NULL;

	max_sub_num = (ssd->parameter->die_chip)*(ssd->parameter->plane_die)*PAGE_INDEX;

	//���������ˣ�ֻʣ���˵����������ʱ�����ʹ����ͨ��one page programȥд��
	if ((ssd->trace_over_flag == 1 && ssd->request_work == NULL) || (ssd->parameter->warm_flash == 1 && ssd->trace_over_flag == 1 && ssd->warm_flash_cmplt == 0))
	{
		for (i = 0; i < subs_count;i++)
			get_ppn_for_normal_command(ssd, channel, chip, subs[i]);

		return SUCCESS;
	}
	
	if (subs_count >= aim_subs_count)
	{
		for (i = aim_subs_count; i < subs_count; i++)
			subs[i] = NULL;

		subs_count = aim_subs_count;
		get_ppn_for_advanced_commands(ssd, channel, chip, subs, subs_count, command);
	}
	else if (subs_count > 0)
	{
		/*
		while (subs_count < aim_subs_count)
		{
			getout2buffer(ssd, NULL, subs[0]->total_request);
			//���±�������д��������ȡ����Ӧ������д������
			for (i = 0; i < max_sub_num; i++)
			{
				subs[i] = NULL;
			}
			sub = ssd->subs_w_head;
			subs_count = 0;
			while ((sub != NULL) && (subs_count < max_sub_num))
			{
				if (sub->current_state == SR_WAIT)
				{
					if ((sub->update == NULL) || ((sub->update != NULL) && ((sub->update->current_state == SR_COMPLETE) || ((sub->update->next_state == SR_COMPLETE) && (sub->update->next_state_predict_time <= ssd->current_time)))))    //û����Ҫ��ǰ������ҳ
					{
						subs[subs_count] = sub;
						subs_count++;
					}
				}
				p = sub;
				sub = sub->next_node;
			}
		}
		if (subs_count == aim_subs_count)
			get_ppn_for_advanced_commands(ssd, channel, chip, subs, subs_count, command);

		*/
		return FAILURE;
	}
	else
	{
		printf("there is no vaild subs\n");
	}
	return SUCCESS;
}

/******************************************************************************************************
*The function of the function is to find two pages of the same horizontal position for the two plane 
*command, and modify the statistics, modify the status of the page
*******************************************************************************************************/
Status find_level_page(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, struct sub_request **sub, unsigned int subs_count)
{
	unsigned int i,j, aim_page = 0, old_plane;
	struct gc_operation *gc_node;
	unsigned int gc_add;

	unsigned int plane,active_block, page,equal_flag;
	unsigned int *page_place;

	page_place = (unsigned int *)malloc(ssd->parameter->plane_die*sizeof(page_place));
	old_plane = ssd->channel_head[channel].chip_head[chip].die_head[die].token;

	//��֤����sub����Ч��
	if (subs_count != ssd->parameter->plane_die)
	{
		printf("find level failed\n");
		getchar();
		return ERROR;
	}

	for (i = 0; i < ssd->parameter->plane_die; i++)
	{
		find_active_block(ssd, channel, chip, die, i);
		active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[i].active_block;
		page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[i].blk_head[active_block].last_write_page + 1;
		page_place[i] = page;
	}

	equal_flag = 1;
	for (i = 0; i < (ssd->parameter->plane_die - 1); i++)
	{
		if (page_place[i] != page_place[i + 1])
		{
			equal_flag = 0;
			break;
		}
	}

	//�ж����е�page�Ƿ���ȣ������ȣ�ִ��mutli plane���������ȣ�̰����ʹ�ã������е�page������page����
	if (equal_flag == 1)	//pageƫ�Ƶ�ַһ��
	{
		for (i = 0; i < ssd->parameter->plane_die; i++)
		{
			active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[i].active_block;
			flash_page_state_modify(ssd, sub[i], channel, chip, die, i, active_block, page_place[i]);
		}
	}
	else				    //pageƫ�Ƶ�ַ��һ��
	{
		if (ssd->parameter->greed_MPW_ad == 1)                                          
		{			
			//�鿴page����ҳ����aim_page
			for (i = 0; i < ssd->parameter->plane_die; i++)
			{
				if (page_place[i] > aim_page)
					aim_page = page_place[i];
			}
			/*
			for (i = 0; i < ssd->parameter->plane_die; i++)
			{
				if (page_place[i] != aim_page)
				{
					if (aim_page - page_place[i] != 1)
						getchar();
				}
			}*/

			//���ȼ���Ƿ������0-63��63-0�Ŀ�ҳ����������
			if ((page_place[0] == (ssd->parameter->page_block - 1) && page_place[1] == 0) || (page_place[0] == 0 && page_place[1] == (ssd->parameter->page_block - 1)))
			{
				for (i = 0; i < ssd->parameter->plane_die; i++)
				{
					if (page_place[i] == (ssd->parameter->page_block - 1))
					{
						//���Ƚ����63��ҳ����Ч
						active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[i].active_block;
						make_same_level(ssd, channel, chip, die, i, active_block, ssd->parameter->page_block);

						//Ѱ���µ�block,��������Ч��63��ҳ�����Ѱ�ҵ������һ�����п�
						find_active_block(ssd, channel, chip, die, i);
						active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[i].active_block;
						page_place[i] = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[i].blk_head[active_block].last_write_page + 1;

						//����aim_pageΪ0
						aim_page = 0;
						break;
					}
				}
			}
			
			for (i = 0; i < ssd->parameter->plane_die; i++)
			{
				active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[i].active_block;
				if (page_place[i] != aim_page)
					make_same_level(ssd, channel, chip, die, i, active_block, aim_page);

				flash_page_state_modify(ssd, sub[i], channel, chip, die, i, active_block, aim_page);
			}
			ssd->channel_head[channel].chip_head[chip].die_head[die].token = old_plane;
		}
		else                                                                            
		{
			ssd->channel_head[channel].chip_head[chip].die_head[die].token = old_plane;
			for (i = 0; i < subs_count; i++)
				sub[i] = NULL;
			free(page_place);
			return FAILURE;
		}
	}

	//�ж��Ƿ�ƫ�Ƶ�ַfree pageһ��
	/*
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[0].free_page != ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[1].free_page)
	{
		printf("free page don't equal\n");
		getchar();
	}*/


	//�ж��Ƿ�ƫ�Ƶ�ַfree pageһ��
	/*
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[0].free_page != ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[1].free_page)
	{
	printf("free page don't equal\n");
	getchar();
	}*/


	gc_add = 0;
	ssd->channel_head[channel].gc_soft = 0;
	ssd->channel_head[channel].gc_hard = 0;
	for (i = 0; i < ssd->parameter->plane_die; i++)
	{
		if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[i].free_page <= (ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_soft_threshold))
		{
			ssd->channel_head[channel].gc_soft = 1;
			gc_add = 1;
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[i].free_page <= (ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
			{
				ssd->channel_head[channel].gc_hard = 1;
			}
		}
	}
	if (gc_add == 1)		//produce a gc reqeuest and add gc_node to the channel
	{

		gc_node = (struct gc_operation *)malloc(sizeof(struct gc_operation));
		alloc_assert(gc_node, "gc_node");
		memset(gc_node, 0, sizeof(struct gc_operation));
		if (ssd->channel_head[channel].gc_soft == 1)
		{
			gc_node->soft = 1;
		}
		else
		{
			gc_node->soft = 0;
		}
		if (ssd->channel_head[channel].gc_hard == 1)
		{
			gc_node->hard = 1;
		}
		else
		{
			gc_node->hard = 0;
		}
		gc_node->next_node = NULL;
		gc_node->channel = channel;
		gc_node->chip = chip;
		gc_node->die = die;
		gc_node->plane = old_plane;
		gc_node->block = 0xffffffff;
		gc_node->page = 0;
		gc_node->state = GC_WAIT;
		gc_node->priority = GC_UNINTERRUPT;
		gc_node->next_node = ssd->channel_head[channel].gc_command;
		ssd->channel_head[channel].gc_command = gc_node;					//inserted into the head of the gc chain
		ssd->gc_request++;
	}
	free(page_place);
	return SUCCESS;
}

/*************************************************************
*the function is to have two different page positions the same
**************************************************************/
struct ssd_info *make_same_level(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int block, unsigned int aim_page)
{
	int i = 0, step, page;
	struct direct_erase *new_direct_erase, *direct_erase_node = NULL;

	page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page + 1;                  /*The page number of the current block that needs to be adjusted*/
	step = aim_page - page;
	while (i<step)
	{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page + i].valid_state = 0;     
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page + i].free_state = 0;     
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page + i].lpn = 0;

		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num++;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;

		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_write_count++;
		i++;
	}

	ssd->waste_page_count += step;

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page = aim_page - 1;

	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num == ssd->parameter->page_block)    /*The block is invalid in the page, it can directly delete*/
	{
		new_direct_erase = (struct direct_erase *)malloc(sizeof(struct direct_erase));
		alloc_assert(new_direct_erase, "new_direct_erase");
		memset(new_direct_erase, 0, sizeof(struct direct_erase));

		new_direct_erase->block = block;
		new_direct_erase->next_node = NULL;
		direct_erase_node = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
		if (direct_erase_node == NULL)
		{
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node = new_direct_erase;
		}
		else
		{
			new_direct_erase->next_node = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node = new_direct_erase;
		}
	}

	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page>(ssd->parameter->page_block-1))
	{
		printf("error! the last write page larger than max!!\n");
		while (1){}
	}

	return ssd;
}

/****************************************************************************
*this function is to calculate the processing time and the state transition 
*of the processing when processing the write request for the advanced command
*****************************************************************************/
struct ssd_info *compute_serve_time(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, struct sub_request **subs, unsigned int subs_count, unsigned int command)
{
	unsigned int i = 0;
	struct sub_request * last_sub = NULL;
	int prog_time = 0;

	//����д�����У�д��ַ�����д���ݵĴ����Ǵ��еģ�����һ������
	for (i = 0; i < subs_count; i++)
	{
		subs[i]->current_state = SR_W_TRANSFER;
		if (last_sub == NULL)
		{
			subs[i]->current_time = ssd->current_time;
		}
		else
		{
			subs[i]->current_time = last_sub->complete_time + ssd->parameter->time_characteristics.tDBSY;
		}

		subs[i]->update_read_flag = 0;
		subs[i]->next_state = SR_COMPLETE;
		subs[i]->next_state_predict_time = subs[i]->current_time + 7 * ssd->parameter->time_characteristics.tWC + (subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		subs[i]->complete_time = subs[i]->next_state_predict_time;
		last_sub = subs[i];

		delete_from_channel(ssd, channel, subs[i]);
	}

	ssd->channel_head[channel].current_state = CHANNEL_TRANSFER;
	ssd->channel_head[channel].current_time = ssd->current_time;
	ssd->channel_head[channel].next_state = CHANNEL_IDLE;
	ssd->channel_head[channel].next_state_predict_time = last_sub->complete_time;

	//��ͬ�ĸ߼�������д���ʵ�ʱ�����������������������ʱ���chip��ʱ������
	if (command == ONE_SHOT_MUTLI_PLANE)
	{
		prog_time = ssd->parameter->time_characteristics.tPROGO;
		ssd->mutliplane_oneshot_prog_count++;
		ssd->m_plane_prog_count += 3;
	}
	else if (command == MUTLI_PLANE)
	{
		prog_time = ssd->parameter->time_characteristics.tPROG;
		ssd->m_plane_prog_count++;
	}
	else if (command == ONE_SHOT)
	{
		prog_time = ssd->parameter->time_characteristics.tPROGO;
		ssd->ontshot_prog_count++;
	}
	else if (command == NORMAL)
	{
		prog_time = ssd->parameter->time_characteristics.tPROG;
	}
	else
	{
		printf("Error! commond error\n");
		getchar();
	}

	ssd->channel_head[channel].chip_head[chip].current_state = CHIP_WRITE_BUSY;
	ssd->channel_head[channel].chip_head[chip].current_time = ssd->current_time;
	ssd->channel_head[channel].chip_head[chip].next_state = CHIP_IDLE;
	ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + prog_time;

	//����ǰ�������д���ʵ�ʱ��
	for (i = 0; i < subs_count; i++)
	{
		subs[i]->next_state_predict_time = subs[i]->next_state_predict_time + prog_time;
		subs[i]->complete_time = subs[i]->next_state_predict_time;
	}

	return ssd;

}


/*****************************************************************************************
*Function is to remove the request from ssd-> subs_w_head or ssd-> channel_head [channel] .subs_w_head
******************************************************************************************/
struct ssd_info *delete_from_channel(struct ssd_info *ssd, unsigned int channel, struct sub_request * sub_req)
{
	struct sub_request *sub = NULL, *p;

	if (ssd->parameter->allocation_scheme == DYNAMIC_ALLOCATION || ssd->parameter->allocation_scheme == HYBRID_ALLOCATION)
		sub = ssd->subs_w_head;
	else if (ssd->parameter->allocation_scheme == STATIC_ALLOCATION)
		sub = ssd->channel_head[channel].subs_w_head; 

	p = sub;
	while (sub != NULL)
	{
		if (sub == sub_req)
		{
			if (ssd->parameter->allocation_scheme == DYNAMIC_ALLOCATION || ssd->parameter->allocation_scheme == HYBRID_ALLOCATION)
			{
				if (sub == ssd->subs_w_head)
				{
					if (ssd->subs_w_head != ssd->subs_w_tail)
					{
						ssd->subs_w_head = sub->next_node;
						sub = ssd->subs_w_head;
						continue;
					}
					else
					{
						ssd->subs_w_head = NULL;
						ssd->subs_w_tail = NULL;
						p = NULL;
						break;
					}
				}
				else
				{
					if (sub->next_node != NULL)
					{
						p->next_node = sub->next_node;
						sub = p->next_node;
						continue;
					}
					else
					{
						ssd->subs_w_tail = p;
						ssd->subs_w_tail->next_node = NULL;
						break;
					}
				}
			}
			else if (ssd->parameter->allocation_scheme == STATIC_ALLOCATION)
			{
				if (sub == ssd->channel_head[channel].subs_w_head)
				{
					if (ssd->channel_head[channel].subs_w_head != ssd->channel_head[channel].subs_w_tail)
					{
						ssd->channel_head[channel].subs_w_head = sub->next_node;
						sub = ssd->channel_head[channel].subs_w_head;
						continue;
					}
					else
					{
						ssd->channel_head[channel].subs_w_head = NULL;
						ssd->channel_head[channel].subs_w_tail = NULL;
						p = NULL;
						break;
					}
				}
				else
				{
					if (sub->next_node != NULL)
					{
						p->next_node = sub->next_node;
						sub = p->next_node;
						continue;
					}
					else
					{
						ssd->channel_head[channel].subs_w_tail = p;
						ssd->channel_head[channel].subs_w_tail->next_node = NULL;
						break;
					}
				}
			}
		}
		p = sub;
		sub = sub->next_node;
	}

	return ssd;
}