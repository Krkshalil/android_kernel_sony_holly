/******************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
/* BEGIN PN:DTS2013053103858 , Added by d00238048, 2013.05.31*/
/* MM-GL-DISPLAY-panel-21+[ */
#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#endif
#include <cust_gpio_usage.h>
#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif
#if 0
#include <linux/fih_hw_info.h>
#endif
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									1
#define FRAME_WIDTH  										(1080)
#define FRAME_HEIGHT 										(1920)


#define REGFLAG_DELAY             								0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static const unsigned int BL_MIN_LEVEL =20;
static LCM_UTIL_FUNCS lcm_util;
extern unsigned int IS_LCM_INIT_READY;
extern unsigned int IS_LCM_POW_READY;
extern unsigned int lcm_first_suspend;
extern unsigned int IS_GET_LCM_ID;
extern unsigned int LCM_ID;
#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};
//update initial param for IC nt35520 0.01
static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28,0,{}},
	/* MM-GL-DISPLAY-panel-23- *///{REGFLAG_DELAY, 150, {}},
	{REGFLAG_DELAY, 40, {}},/* MM-GL-DISPLAY-panel-23+ */
	{0x10,0,{}},
	/* MM-GL-DISPLAY-panel-23- *///{REGFLAG_DELAY, 150, {}},
	{REGFLAG_DELAY, 80, {}},/* MM-GL-DISPLAY-panel-23+ */
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
static struct LCM_setting_table lcm_initialization_setting[] =
{
	{0xFF,1,{0x20}},
	{0x00,1,{0x01}},
	{0x01,1,{0x55}},
	{0x02,1,{0x45}},
	{0x03,1,{0x55}},
	{0x05,1,{0x50}},
	{0x06,1,{0x4A}},
	{0x07,1,{0x24}},
	{0x08,1,{0x0C}},
	{0x0B,1,{0x87}},
	{0x0C,1,{0x87}},
	{0x0E,1,{0xB0}},
	{0x0F,1,{0xB3}},
	{0x11,1,{0x22}},
	{0x12,1,{0x10}},
	{0x13,1,{0x03}},
	{0x14,1,{0x4A}},
	{0x15,1,{0x12}},
	{0x16,1,{0x12}},
	{0x30,1,{0x01}},
	{0x58,1,{0x82}},
	{0x59,1,{0x00}},
	{0x5A,1,{0x02}},
	{0x5B,1,{0x00}},
	{0x5C,1,{0x82}},
	{0x5D,1,{0x80}},
	{0x5E,1,{0x02}},
	{0x5F,1,{0x00}},
	{0x72,1,{0x31}},
	{0xFB,1,{0x01}},
	{0xFF,1,{0x24}},
	{0x00,1,{0x01}},
	{0x01,1,{0x0B}},
	{0x02,1,{0x0C}},
	{0x03,1,{0x89}},
	{0x04,1,{0x8A}},
	{0x05,1,{0x0F}},
	{0x06,1,{0x10}},
	{0x07,1,{0x10}},
	{0x08,1,{0x1C}},
	{0x09,1,{0x00}},
	{0x0A,1,{0x00}},
	{0x0B,1,{0x00}},
	{0x0C,1,{0x00}},
	{0x0D,1,{0x13}},
	{0x0E,1,{0x15}},
	{0x0F,1,{0x17}},
	{0x10,1,{0x01}},
	{0x11,1,{0x0B}},
	{0x12,1,{0x0C}},
	{0x13,1,{0x89}},
	{0x14,1,{0x8A}},
	{0x15,1,{0x0F}},
	{0x16,1,{0x10}},
	{0x17,1,{0x10}},
	{0x18,1,{0x1C}},
	{0x19,1,{0x00}},
	{0x1A,1,{0x00}},
	{0x1B,1,{0x00}},
	{0x1C,1,{0x00}},
	{0x1D,1,{0x13}},
	{0x1E,1,{0x15}},
	{0x1F,1,{0x17}},
	{0x20,1,{0x00}},
	{0x21,1,{0x01}},
	{0x22,1,{0x00}},
	{0x23,1,{0x40}},
	{0x24,1,{0x40}},
	{0x25,1,{0x6D}},
	{0x26,1,{0x40}},
	{0x27,1,{0x40}},
	{0xBD,1,{0x20}},
	{0xB6,1,{0x21}},
	{0xB7,1,{0x22}},
	{0xB8,1,{0x07}},
	{0xB9,1,{0x07}},
	{0xC1,1,{0x6D}},
	{0xBE,1,{0x07}},
	{0xBF,1,{0x07}},
	{0x29,1,{0xD8}},
	{0x2A,1,{0x2A}},
	{0x4B,1,{0x03}},
	{0x4C,1,{0x11}},
	{0x4D,1,{0x10}},
	{0x4E,1,{0x01}},
	{0x4F,1,{0x01}},
	{0x50,1,{0x10}},
	{0x51,1,{0x00}},
	{0x52,1,{0x80}},
	{0x53,1,{0x00}},
	{0x54,1,{0x07}},
	{0x55,1,{0x25}},
	{0x56,1,{0x00}},
	{0x58,1,{0x07}},
	{0x5B,1,{0x43}},
	{0x5C,1,{0x00}},
	{0x5F,1,{0x73}},
	{0x60,1,{0x73}},
	{0x63,1,{0x22}},
	{0x64,1,{0x00}},
	{0x67,1,{0x08}},
	{0x68,1,{0x04}},
	{0x7A,1,{0x80}},
	{0x7B,1,{0x91}},
	{0x7C,1,{0xD8}},
	{0x7D,1,{0x60}},
	{0x93,1,{0x06}},
	{0x94,1,{0x06}},
	{0xB3,1,{0xC0}},
	{0xB4,1,{0x00}},
	{0xB5,1,{0x00}},
	{0x8A,1,{0x33}},
	{0x9B,1,{0x0F}},
	{0x8B,1,{0xF0}},
	{0xFB,1,{0x01}},
	{0xFF,1,{0x20}},
	{0xFB,1,{0x01}},
	{0x75,1,{0x00}},
	{0x76,1,{0x2E}},
	{0x77,1,{0x00}},
	{0x78,1,{0x5F}},
	{0x79,1,{0x00}},
	{0x7A,1,{0x8B}},
	{0x7B,1,{0x00}},
	{0x7C,1,{0xA8}},
	{0x7D,1,{0x00}},
	{0x7E,1,{0xBF}},
	{0x7F,1,{0x00}},
	{0x80,1,{0xD3}},
	{0x81,1,{0x00}},
	{0x82,1,{0xE3}},
	{0x83,1,{0x00}},
	{0x84,1,{0xF2}},
	{0x85,1,{0x01}},
	{0x86,1,{0x00}},
	{0x87,1,{0x01}},
	{0x88,1,{0x2C}},
	{0x89,1,{0x01}},
	{0x8A,1,{0x4F}},
	{0x8B,1,{0x01}},
	{0x8C,1,{0x86}},
	{0x8D,1,{0x01}},
	{0x8E,1,{0xB1}},
	{0x8F,1,{0x01}},
	{0x90,1,{0xF4}},
	{0x91,1,{0x02}},
	{0x92,1,{0x2A}},
	{0x93,1,{0x02}},
	{0x94,1,{0x2B}},
	{0x95,1,{0x02}},
	{0x96,1,{0x5E}},
	{0x97,1,{0x02}},
	{0x98,1,{0x97}},
	{0x99,1,{0x02}},
	{0x9A,1,{0xBC}},
	{0x9B,1,{0x02}},
	{0x9C,1,{0xEE}},
	{0x9D,1,{0x03}},
	{0x9E,1,{0x11}},
	{0x9F,1,{0x03}},
	{0xA0,1,{0x3E}},
	{0xA2,1,{0x03}},
	{0xA3,1,{0x4B}},
	{0xA4,1,{0x03}},
	{0xA5,1,{0x5A}},
	{0xA6,1,{0x03}},
	{0xA7,1,{0x6A}},
	{0xA9,1,{0x03}},
	{0xAA,1,{0x7C}},
	{0xAB,1,{0x03}},
	{0xAC,1,{0x8F}},
	{0xAD,1,{0x03}},
	{0xAE,1,{0xA3}},
	{0xAF,1,{0x03}},
	{0xB0,1,{0xB4}},
	{0xB1,1,{0x03}},
	{0xB2,1,{0xFF}},
	{0xB3,1,{0x00}},
	{0xB4,1,{0x2E}},
	{0xB5,1,{0x00}},
	{0xB6,1,{0x5F}},
	{0xB7,1,{0x00}},
	{0xB8,1,{0x8B}},
	{0xB9,1,{0x00}},
	{0xBA,1,{0xA8}},
	{0xBB,1,{0x00}},
	{0xBC,1,{0xBF}},
	{0xBD,1,{0x00}},
	{0xBE,1,{0xD3}},
	{0xBF,1,{0x00}},
	{0xC0,1,{0xE3}},
	{0xC1,1,{0x00}},
	{0xC2,1,{0xF2}},
	{0xC3,1,{0x01}},
	{0xC4,1,{0x00}},
	{0xC5,1,{0x01}},
	{0xC6,1,{0x2C}},
	{0xC7,1,{0x01}},
	{0xC8,1,{0x4F}},
	{0xC9,1,{0x01}},
	{0xCA,1,{0x86}},
	{0xCB,1,{0x01}},
	{0xCC,1,{0xB1}},
	{0xCD,1,{0x01}},
	{0xCE,1,{0xF4}},
	{0xCF,1,{0x02}},
	{0xD0,1,{0x2A}},
	{0xD1,1,{0x02}},
	{0xD2,1,{0x2B}},
	{0xD3,1,{0x02}},
	{0xD4,1,{0x5E}},
	{0xD5,1,{0x02}},
	{0xD6,1,{0x97}},
	{0xD7,1,{0x02}},
	{0xD8,1,{0xBC}},
	{0xD9,1,{0x02}},
	{0xDA,1,{0xEE}},
	{0xDB,1,{0x03}},
	{0xDC,1,{0x11}},
	{0xDD,1,{0x03}},
	{0xDE,1,{0x3E}},
	{0xDF,1,{0x03}},
	{0xE0,1,{0x4B}},
	{0xE1,1,{0x03}},
	{0xE2,1,{0x5A}},
	{0xE3,1,{0x03}},
	{0xE4,1,{0x6A}},
	{0xE5,1,{0x03}},
	{0xE6,1,{0x7C}},
	{0xE7,1,{0x03}},
	{0xE8,1,{0x8F}},
	{0xE9,1,{0x03}},
	{0xEA,1,{0xA3}},
	{0xEB,1,{0x03}},
	{0xEC,1,{0xB4}},
	{0xED,1,{0x03}},
	{0xEE,1,{0xFF}},
	{0xEF,1,{0x00}},
	{0xF0,1,{0x2E}},
	{0xF1,1,{0x00}},
	{0xF2,1,{0x32}},
	{0xF3,1,{0x00}},
	{0xF4,1,{0x47}},
	{0xF5,1,{0x00}},
	{0xF6,1,{0x59}},
	{0xF7,1,{0x00}},
	{0xF8,1,{0x69}},
	{0xF9,1,{0x00}},
	{0xFA,1,{0x79}},
	{0xFF,1,{0x21}},
	{0xFB,1,{0x01}},
	{0x00,1,{0x00}},
	{0x01,1,{0x89}},
	{0x02,1,{0x00}},
	{0x03,1,{0x9B}},
	{0x04,1,{0x00}},
	{0x05,1,{0xAD}},
	{0x06,1,{0x00}},
	{0x07,1,{0xDE}},
	{0x08,1,{0x01}},
	{0x09,1,{0x05}},
	{0x0A,1,{0x01}},
	{0x0B,1,{0x49}},
	{0x0C,1,{0x01}},
	{0x0D,1,{0x7F}},
	{0x0E,1,{0x01}},
	{0x0F,1,{0xD4}},
	{0x10,1,{0x02}},
	{0x11,1,{0x16}},
	{0x12,1,{0x02}},
	{0x13,1,{0x17}},
	{0x14,1,{0x02}},
	{0x15,1,{0x4F}},
	{0x16,1,{0x02}},
	{0x17,1,{0x8A}},
	{0x18,1,{0x02}},
	{0x19,1,{0xAF}},
	{0x1A,1,{0x02}},
	{0x1B,1,{0xE1}},
	{0x1C,1,{0x03}},
	{0x1D,1,{0x03}},
	{0x1E,1,{0x03}},
	{0x1F,1,{0x2F}},
	{0x20,1,{0x03}},
	{0x21,1,{0x3D}},
	{0x22,1,{0x03}},
	{0x23,1,{0x4C}},
	{0x24,1,{0x03}},
	{0x25,1,{0x5C}},
	{0x26,1,{0x03}},
	{0x27,1,{0x70}},
	{0x28,1,{0x03}},
	{0x29,1,{0x86}},
	{0x2A,1,{0x03}},
	{0x2B,1,{0xA0}},
	{0x2D,1,{0x03}},
	{0x2F,1,{0xB8}},
	{0x30,1,{0x03}},
	{0x31,1,{0xFF}},
	{0x32,1,{0x00}},
	{0x33,1,{0x2E}},
	{0x34,1,{0x00}},
	{0x35,1,{0x32}},
	{0x36,1,{0x00}},
	{0x37,1,{0x47}},
	{0x38,1,{0x00}},
	{0x39,1,{0x59}},
	{0x3A,1,{0x00}},
	{0x3B,1,{0x69}},
	{0x3D,1,{0x00}},
	{0x3F,1,{0x79}},
	{0x40,1,{0x00}},
	{0x41,1,{0x89}},
	{0x42,1,{0x00}},
	{0x43,1,{0x9B}},
	{0x44,1,{0x00}},
	{0x45,1,{0xAD}},
	{0x46,1,{0x00}},
	{0x47,1,{0xDE}},
	{0x48,1,{0x01}},
	{0x49,1,{0x05}},
	{0x4A,1,{0x01}},
	{0x4B,1,{0x49}},
	{0x4C,1,{0x01}},
	{0x4D,1,{0x7F}},
	{0x4E,1,{0x01}},
	{0x4F,1,{0xD4}},
	{0x50,1,{0x02}},
	{0x51,1,{0x16}},
	{0x52,1,{0x02}},
	{0x53,1,{0x17}},
	{0x54,1,{0x02}},
	{0x55,1,{0x4F}},
	{0x56,1,{0x02}},
	{0x58,1,{0x8A}},
	{0x59,1,{0x02}},
	{0x5A,1,{0xAF}},
	{0x5B,1,{0x02}},
	{0x5C,1,{0xE1}},
	{0x5D,1,{0x03}},
	{0x5E,1,{0x03}},
	{0x5F,1,{0x03}},
	{0x60,1,{0x2F}},
	{0x61,1,{0x03}},
	{0x62,1,{0x3D}},
	{0x63,1,{0x03}},
	{0x64,1,{0x4C}},
	{0x65,1,{0x03}},
	{0x66,1,{0x5C}},
	{0x67,1,{0x03}},
	{0x68,1,{0x70}},
	{0x69,1,{0x03}},
	{0x6A,1,{0x86}},
	{0x6B,1,{0x03}},
	{0x6C,1,{0xA0}},
	{0x6D,1,{0x03}},
	{0x6E,1,{0xB8}},
	{0x6F,1,{0x03}},
	{0x70,1,{0xFF}},
	{0x71,1,{0x00}},
	{0x72,1,{0x2E}},
	{0x73,1,{0x00}},
	{0x74,1,{0x41}},
	{0x75,1,{0x00}},
	{0x76,1,{0x63}},
	{0x77,1,{0x00}},
	{0x78,1,{0x7D}},
	{0x79,1,{0x00}},
	{0x7A,1,{0x93}},
	{0x7B,1,{0x00}},
	{0x7C,1,{0xA7}},
	{0x7D,1,{0x00}},
	{0x7E,1,{0xB7}},
	{0x7F,1,{0x00}},
	{0x80,1,{0xC3}},
	{0x81,1,{0x00}},
	{0x82,1,{0xBD}},
	{0x83,1,{0x00}},
	{0x84,1,{0xE8}},
	{0x85,1,{0x01}},
	{0x86,1,{0x16}},
	{0x87,1,{0x01}},
	{0x88,1,{0x5A}},
	{0x89,1,{0x01}},
	{0x8A,1,{0x8D}},
	{0x8B,1,{0x01}},
	{0x8C,1,{0xDB}},
	{0x8D,1,{0x02}},
	{0x8E,1,{0x18}},
	{0x8F,1,{0x02}},
	{0x90,1,{0x19}},
	{0x91,1,{0x02}},
	{0x92,1,{0x4E}},
	{0x93,1,{0x02}},
	{0x94,1,{0x89}},
	{0x95,1,{0x02}},
	{0x96,1,{0xB0}},
	{0x97,1,{0x02}},
	{0x98,1,{0xE4}},
	{0x99,1,{0x03}},
	{0x9A,1,{0x06}},
	{0x9B,1,{0x03}},
	{0x9C,1,{0x36}},
	{0x9D,1,{0x03}},
	{0x9E,1,{0x41}},
	{0x9F,1,{0x03}},
	{0xA0,1,{0x53}},
	{0xA2,1,{0x03}},
	{0xA3,1,{0x62}},
	{0xA4,1,{0x03}},
	{0xA5,1,{0x74}},
	{0xA6,1,{0x03}},
	{0xA7,1,{0x8F}},
	{0xA9,1,{0x03}},
	{0xAA,1,{0x98}},
	{0xAB,1,{0x03}},
	{0xAC,1,{0x9D}},
	{0xAD,1,{0x03}},
	{0xAE,1,{0xFF}},
	{0xAF,1,{0x00}},
	{0xB0,1,{0x2E}},
	{0xB1,1,{0x00}},
	{0xB2,1,{0x41}},
	{0xB3,1,{0x00}},
	{0xB4,1,{0x63}},
	{0xB5,1,{0x00}},
	{0xB6,1,{0x7D}},
	{0xB7,1,{0x00}},
	{0xB8,1,{0x93}},
	{0xB9,1,{0x00}},
	{0xBA,1,{0xA7}},
	{0xBB,1,{0x00}},
	{0xBC,1,{0xB7}},
	{0xBD,1,{0x00}},
	{0xBE,1,{0xC3}},
	{0xBF,1,{0x00}},
	{0xC0,1,{0xBD}},
	{0xC1,1,{0x00}},
	{0xC2,1,{0xE8}},
	{0xC3,1,{0x01}},
	{0xC4,1,{0x16}},
	{0xC5,1,{0x01}},
	{0xC6,1,{0x5A}},
	{0xC7,1,{0x01}},
	{0xC8,1,{0x8D}},
	{0xC9,1,{0x01}},
	{0xCA,1,{0xDB}},
	{0xCB,1,{0x02}},
	{0xCC,1,{0x18}},
	{0xCD,1,{0x02}},
	{0xCE,1,{0x19}},
	{0xCF,1,{0x02}},
	{0xD0,1,{0x4E}},
	{0xD1,1,{0x02}},
	{0xD2,1,{0x89}},
	{0xD3,1,{0x02}},
	{0xD4,1,{0xB0}},
	{0xD5,1,{0x02}},
	{0xD6,1,{0xE4}},
	{0xD7,1,{0x03}},
	{0xD8,1,{0x06}},
	{0xD9,1,{0x03}},
	{0xDA,1,{0x36}},
	{0xDB,1,{0x03}},
	{0xDC,1,{0x41}},
	{0xDD,1,{0x03}},
	{0xDE,1,{0x53}},
	{0xDF,1,{0x03}},
	{0xE0,1,{0x62}},
	{0xE1,1,{0x03}},
	{0xE2,1,{0x74}},
	{0xE3,1,{0x03}},
	{0xE4,1,{0x8F}},
	{0xE5,1,{0x03}},
	{0xE6,1,{0x98}},
	{0xE7,1,{0x03}},
	{0xE8,1,{0x9D}},
	{0xE9,1,{0x03}},
	{0xEA,1,{0xFF}},

	{0xFF,1,{0x10}},
	#ifndef LCM_DSI_CMD_MODE
	{0xFB,1,{0x01}},
	{0x3B,3,{0x03,0x08,0x08}},
	{0xBB,1,{0x03}},
	#endif

	{0x11,0,{}},
	{REGFLAG_DELAY, 120, {}},
	{0x29,0,{}},
	{0x35,1,{0x00}},

	#if LCM_DSI_CMD_MODE
	{0x44,2,{0x03,0x00}},
	#endif
	{REGFLAG_DELAY,40,{}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}

};
#if 0
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 150, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 150, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0xFF, 1, {0x10}},
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 40, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 80, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                if(table[i].count <= 10)
                    MDELAY(table[i].count);
                else
                    MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order 	= LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     	= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      		= LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	params->dsi.packet_size=256;
	//video mode timing

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 2;
	params->dsi.vertical_backporch					= 8;
	params->dsi.vertical_frontporch					= 10;
	params->dsi.vertical_active_line					= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 10;
	params->dsi.horizontal_backporch				= 20;
	params->dsi.horizontal_frontporch				= 40;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	#if LCM_DSI_CMD_MODE
	params->dsi.PLL_CLOCK = 450;
	#else
	params->dsi.PLL_CLOCK = 400;
	#endif

}
static void lcm_init_power(void)
{
//unsigned int product_phase= fih_get_product_phase();
if(IS_LCM_POW_READY == 0)
{
	SET_RESET_PIN(0);
#ifdef BUILD_LK
#if 0
	if(product_phase == PHASE_EVM){
		mt6331_upmu_set_rg_vgp1_vosel(0x03);
		mt6331_upmu_set_rg_vgp1_en(1);
	}
	else{
		mt6331_upmu_set_rg_vgp3_vosel(3);
		mt6331_upmu_set_rg_vgp3_en(1);
	}
#endif
	mt6331_upmu_set_rg_vgp3_vosel(3);
	mt6331_upmu_set_rg_vgp3_en(1);
#else
	printk("%s, begin\n", __func__);
#if 0
	if(product_phase == PHASE_EVM)
		hwPowerOn(MT6331_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
	else
		hwPowerOn(MT6331_POWER_LDO_VGP3, VOL_1800, "LCM_DRV");
#endif
	hwPowerOn(MT6331_POWER_LDO_VGP3, VOL_1800, "LCM_DRV");
	printk("%s, end\n", __func__);
#endif
IS_LCM_POW_READY = 1;
}
}

static void lcm_suspend_power(void)
{
	if(lcm_first_suspend == 0){
		IS_LCM_POW_READY = 0;
		lcm_init_power();
		lcm_first_suspend = 1;
	}

//unsigned int product_phase = fih_get_product_phase();
#ifdef BUILD_LK
#if 0
	if(product_phase == PHASE_EVM)
		mt6331_upmu_set_rg_vgp1_en(0);
	else
		mt6331_upmu_set_rg_vgp3_en(0);
#endif
	mt6331_upmu_set_rg_vgp3_en(0);
#else
	printk("%s, begin\n", __func__);
#if 0
	if(product_phase == PHASE_EVM)
		hwPowerDown(MT6331_POWER_LDO_VGP1, "LCM_DRV");
	else
		hwPowerDown(MT6331_POWER_LDO_VGP3, "LCM_DRV");
#endif
	hwPowerDown(MT6331_POWER_LDO_VGP3, "LCM_DRV");
	printk("%s, end\n", __func__);
#endif
IS_LCM_POW_READY = 0;
}

static void lcm_resume_power(void)
{
//unsigned int product_phase = fih_get_product_phase();
if(IS_LCM_POW_READY == 0)
{
#ifdef BUILD_LK
#if 0
	if(product_phase == PHASE_EVM)
		mt6331_upmu_set_rg_vgp1_en(1);
	else
		mt6331_upmu_set_rg_vgp3_en(1);
#endif
	mt6331_upmu_set_rg_vgp3_en(1);
#else
	printk("%s, begin\n", __func__);
	MDELAY(20);
#if 0
	if(product_phase == PHASE_EVM)
		hwPowerOn(MT6331_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
	else
		hwPowerOn(MT6331_POWER_LDO_VGP3, VOL_1800, "LCM_DRV");
#endif
	hwPowerOn(MT6331_POWER_LDO_VGP3, VOL_1800, "LCM_DRV");
	printk("%s, end\n", __func__);
#endif
IS_LCM_POW_READY = 1;
}
}
static void lcm_init(void)
{
    #if 1
    if(IS_LCM_INIT_READY == 0)
		{
			mt_set_gpio_mode(GPIO_LCD_ENP, GPIO_MODE_00);
			mt_set_gpio_dir(GPIO_LCD_ENP, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO_LCD_ENP, GPIO_OUT_ONE);
			mt_set_gpio_mode(GPIO_LCD_ENN, GPIO_MODE_00);
			mt_set_gpio_dir(GPIO_LCD_ENN, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO_LCD_ENN, GPIO_OUT_ONE);
			MDELAY(10);
			SET_RESET_PIN(1);
			MDELAY(1);
			SET_RESET_PIN(0);
			MDELAY(13);

			SET_RESET_PIN(1);
			MDELAY(11);
			IS_LCM_INIT_READY = 1;
		}
    #else
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(10);
    #endif

	// when phone initial , config output high, enable backlight drv chip
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	MDELAY(2);
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	mt_set_gpio_mode(GPIO_LCD_ENN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_ENN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_ENN, GPIO_OUT_ZERO);
	MDELAY(1);
	mt_set_gpio_mode(GPIO_LCD_ENP, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_ENP, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_ENP, GPIO_OUT_ZERO);
	MDELAY(2);
	SET_RESET_PIN(0);
	IS_LCM_INIT_READY = 0;
}
static void lcm_resume(void)
{

lcm_init();
}
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
         /*BEGIN PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/
         //delete high speed packet
	//data_array[0]=0x00290508;
	//dsi_set_cmdq(data_array, 1, 1);
         /*END PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#define LCM_NT35695_TRULY_ID     (0x91)
static unsigned int lcm_compare_id(void)
{
	unsigned char buffer[2];
	unsigned int array[16];

	if(IS_LCM_INIT_READY == 0)
	{
		mt_set_gpio_mode(GPIO_LCD_ENP, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO_LCD_ENP, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_LCD_ENP, GPIO_OUT_ONE);
		MDELAY(3);
		mt_set_gpio_mode(GPIO_LCD_ENN, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO_LCD_ENN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_LCD_ENN, GPIO_OUT_ONE);

		MDELAY(13);

		SET_RESET_PIN(1);
		MDELAY(11);
		IS_LCM_INIT_READY = 1;
	}

	if(IS_GET_LCM_ID == 0)
	{
		array[0] = 0x00023700;// read id return tree byte
		dsi_set_cmdq(array, 1, 1);

		read_reg_v2(0xDA, buffer, 2);
		LCM_ID = buffer[0]; //we only need ID

#ifdef BUILD_LK
		dprintf(0, "%s, LK LCM ID debug: LCM id = 0x%08x\n", __func__, LCM_ID);
#else
		printk("%s, kernel LCM ID horse debug: LCM id = 0x%08x\n", __func__, LCM_ID);
#endif
		IS_GET_LCM_ID = 1;
	}
	if(LCM_ID == LCM_NT35695_TRULY_ID)
		return 1;
	else
		return 0;
}

LCM_DRIVER nt35695_fhd_dsi_cmd_truly_lcm_drv=
{
    .name           = "nt35695_fhd_dsi_cmd_truly_lcm_drv",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,/*tianma init fun.*/
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
     .compare_id    = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
		.init_power			= lcm_init_power,
    .resume_power   = lcm_resume_power,
    .suspend_power  = lcm_suspend_power,

};
/* END PN:DTS2013053103858 , Added by d00238048, 2013.05.31*/
/* MM-GL-DISPLAY-panel-21+] */
