#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <string.h>
#include <platform/mt_i2c.h>
#include <cust_gpio_usage.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1440)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef BUILD_LK
#define LCM_PRINT printf
#else
#if defined(BUILD_UBOOT)
#define LCM_PRINT printf
#else
#define LCM_PRINT printk
#endif
#endif

#define LCM_DBG(fmt, arg...) \
    LCM_PRINT ("[hx8394_hsd] %s (line:%d) :" fmt "\r\n", __func__, __LINE__, ## arg)
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#ifdef GPIO_LCM_PWR
#define SET_PWR_PIN(v)    \
    mt_set_gpio_mode(GPIO_LCM_PWR,GPIO_MODE_00);    \
    mt_set_gpio_dir(GPIO_LCM_PWR,GPIO_DIR_OUT);     \
    if(v)                                           \
        mt_set_gpio_out(GPIO_LCM_PWR,GPIO_OUT_ONE); \
    else                                            \
        mt_set_gpio_out(GPIO_LCM_PWR,GPIO_OUT_ZERO);
#endif
#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#ifdef BUILD_LK
#define GPIO_LCD_ID (GPIO2 | 0x80000000)
#else
extern void lcm_enn(int onoff);
extern void lcm_enp(int onoff);
extern  int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value);
extern int get_lcm_id_status(void);
#endif

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)            lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                           lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE                          0

#define REGFLAG_END_OF_TABLE                                      0xFD   // END OF REGISTERS MARKER
#define REGFLAG_DELAY                                           0xFC

struct LCM_setting_table
{
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table  lcm_deep_sleep_mode_in_setting_v2[] =
{
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 50, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
};

static struct LCM_setting_table lcm_initialization_setting_v2[] =
{
    
	{0xB9, 3,{0xFF,0x83,0x94}},
	{0xB1,10,{0x50,0x11,0x71,0x09,0x32,0x84,0x71,0x51,0x55,0x30}},
	{0xBA, 6,{0x63,0x03,0x68,0x6B,0xB2,0xC0}},
	{0xB2, 5,{0x00,0x80,0x78,0x0C,0x07}},
	{0xB4,21,{0x01,0x6B,0x01,0x6B,0x01,0x6B,0x01,0x0C,0x7C,0x75,0x00,0x3F,0x01,0x6B,0x01,0x6B,0x01,0x6B,0x01,0x0C,0x7C}},
	{0xD3,33,{0x00,0x00,0x00,0x00,0x3C,0x1C,0x00,0x00,0x32,0x10,0x0A,0x00,0x0A,0x32,0x15,0xAE,0x05,0xAE,0x32,0x00,0x00,0x00,0x00,0x27,0x02,0x0C,0x0C,0x27,0x00,0x00,0x00,0x0C,0x40}},
	{0xD5,44,{0x19,0x19,0x18,0x18,0x1B,0x1B,0x1A,0x1A,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x20,0x21,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x24,0x25,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},
	{0xD6,44,{0x18,0x18,0x19,0x19,0x1B,0x1B,0x1A,0x1A,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x25,0x24,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x21,0x20,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},
	{0xE0,58,{0x00,0x04,0x0C,0x12,0x14,0x18,0x1A,0x18,0x31,0x3F,0x4D,0x4C,0x54,0x65,0x6B,0x70,0x7F,0x82,0x7E,0x8A,0x99,0x4A,0x48,0x49,0x4B,0x4A,0x4C,0x4B,0x7F,0x00,0x04,0x0C,0x11,0x13,0x17,0x1A,0x18,0x31,0x3F,0x4D,0x4C,0x54,0x65,0x6B,0x70,0x7F,0x82,0x7E,0x8A,0x99,0x4A,0x48,0x49,0x4B,0x4A,0x4C,0x4B,0x7F}},
	{0xCC, 1,{0x0B}},
	{0xC0, 2,{0x1F,0x31}},
	{0xB6, 2,{0x6C,0x6C}},
	{0xD4, 1,{0x02}},
	{0xBD, 1,{0x01}},
	{0xB1, 1,{0x00}},
	{0xBD, 1,{0x00}},
	{0xBF, 7,{0x40,0x81,0x50,0x00,0x1A,0xFC,0x01}},
	{0xC6, 1,{0xEF}},
	{0x35, 1,{0x00}},
	{0x11, 1,{0x00}},
    
    {REGFLAG_DELAY,120,{}},
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY,20,{}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;
        switch (cmd)
        {
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
            case REGFLAG_END_OF_TABLE :
                break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }

}


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

    params->physical_width  = 62.10; 
    params->physical_height = 110.40; 

#if (LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
#endif

    // DSI
    /* Command mode setting */
    //1 Three lane or Four lane
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Video mode setting
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active = 2;  // 4
    params->dsi.vertical_backporch = 16;  //22
    params->dsi.vertical_frontporch = 9; // 36
    params->dsi.vertical_active_line = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active = 32;
    params->dsi.horizontal_backporch = 48;
    params->dsi.horizontal_frontporch = 60;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    // Bit rate calculation
    //params->dsi.PLL_CLOCK=205;
    params->dsi.PLL_CLOCK=257;  //LINE </EGAFM-297> <change the mipi clock to reduce disturbing the wifi> <20160413> panzaoyan
    params->dsi.ssc_disable=1;

    //yixuhong 20150511 add esd check function
#ifndef BUILD_LK
    params->dsi.esd_check_enable = 0;
    params->dsi.customization_esd_check_enable = 1;//0:te esd check 1:read register
    params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
    params->dsi.lcm_esd_check_table[0].count = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

    params->dsi.lcm_esd_check_table[1].cmd = 0x0D;
    params->dsi.lcm_esd_check_table[1].count = 1;
    params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
#endif /*BUILD_LK*/

}

#ifdef BUILD_LK
static int gpio_bl_enp   = (53 | 0x80000000);
static int gpio_bl_enn   = (54 | 0x80000000);
static struct mt_i2c_t LCD_i2c;

static kal_uint32 lcd_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    LCD_i2c.id = I2C0;

    LCD_i2c.addr = (0x73);
    LCD_i2c.mode = ST_MODE;
    LCD_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&LCD_i2c, write_data, len);


    return ret_code;
}
#endif

static void lcm_init(void)
{
    LCM_DBG();

#ifdef BUILD_LK
    mt_set_gpio_mode(gpio_bl_enp,GPIO_MODE_00);
    mt_set_gpio_dir(gpio_bl_enp,GPIO_DIR_OUT);
    mt_set_gpio_mode(gpio_bl_enn,GPIO_MODE_00);
    mt_set_gpio_dir(gpio_bl_enn,GPIO_DIR_OUT);
    mt_set_gpio_out(gpio_bl_enp,GPIO_OUT_ONE);
    MDELAY(1);
    lcd_write_byte(0,0x0F);
    MDELAY(1);
    lcd_write_byte(1,0x0F);
    MDELAY(1);
    mt_set_gpio_out(gpio_bl_enn,GPIO_OUT_ONE);
#else //Kernel driver
    lcm_enn(1);
    MDELAY(1);
    _lcm_i2c_write_bytes(0,0x0F);
    MDELAY(1);
    _lcm_i2c_write_bytes(1,0x0F);
    MDELAY(1);
    lcm_enp(1);
#endif

#ifdef GPIO_LCM_PWR
    SET_PWR_PIN(0);
    MDELAY(20);
    SET_PWR_PIN(1);
    MDELAY(150);
#endif
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);
    LCM_DBG("jacky debug,lcm reset end \n");
    push_table(lcm_initialization_setting_v2, sizeof(lcm_initialization_setting_v2) / sizeof(struct LCM_setting_table), 1);
    LCM_DBG("jacy debug,lcm init end \n");
}

static void lcm_suspend(void)
{
    LCM_DBG();
    push_table(lcm_deep_sleep_mode_in_setting_v2, sizeof(lcm_deep_sleep_mode_in_setting_v2) / sizeof(struct LCM_setting_table), 1);
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(20); // 1ms

    SET_RESET_PIN(1);
    MDELAY(120);
}

static void lcm_resume(void)
{
    LCM_DBG();
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);
    push_table(lcm_initialization_setting_v2, sizeof(lcm_initialization_setting_v2) / sizeof(struct LCM_setting_table), 1);
}

#define READ_LCD_REGISTER
#define LCD_ID 0x8394
static unsigned int lcm_compare_id(void)
{
#ifdef READ_LCD_REGISTER
    char id_high=0;
    char id_low=0;
	int id=0;
	int array[4];
	char buffer[5];

	array[0]=0x00023700;//0x00023700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDA, buffer,1);
	id_high = buffer[0]; ///////////////////////0x83	
	MDELAY(2);
	
	read_reg_v2(0xDB, buffer,1);
	id_low = buffer[0]; ///////////////////////0x94
	MDELAY(2);

	id = (id_high << 8) | id_low;
	LCM_DBG("id = %x\n", id);

	if(LCD_ID == id)
	{
		return 1;
	}
	return 0;

#else //read hw id
    s32 lcd_hw_id = -1;

#ifdef BUILD_LK
    mt_set_gpio_mode(GPIO_LCD_ID,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_ID,GPIO_DIR_IN);
    lcd_hw_id = mt_get_gpio_in(GPIO_LCD_ID);
#else
    lcd_hw_id = get_lcm_id_status();
#endif
    LCM_DBG("lcm_compare_id lcd_hw_id=%d \n",lcd_hw_id);
    if (0 == lcd_hw_id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif /*READ_LCD_REGISTER*/	

}

LCM_DRIVER hx8394_p200_hdplus_dsi_vdo_hsd_lcm_drv =
{
    .name       = "hx8394_hdplus_dsi_vdo_hsd",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
};
