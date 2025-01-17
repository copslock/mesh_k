/*
 * BCM47XX NAND flash driver
 *
 * Copyright (C) 2012 Rafał Miłecki <zajec5@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "rtknflash.h"
#include <linux/kernel.h>
#ifdef __UBOOT__
#include <nand.h>
#else
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h> 
#endif

static  DEFINE_SPINLOCK(lock_nand);
extern struct rtknflash *rtkn;

#ifdef RTKN_FLASH_TEST
static int rtkn_test_num = 0;
#endif
#ifdef RTKN_FLASH_TEST_WRITE
static int rtkn_test_write = 0;
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#include <linux/mtd/partitions.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>	
#endif

#include "rtknflash_openwrt.c"

#if !defined(CONFIG_MTD_RTKXX_PARTS)
#include "rtknflash_partition.c"
#elif defined(CONFIG_MTD_RTKXX_PARTS) && defined(CONFIG_RTK_NAND_FLASH_STORAGE)
#include "rtknflash_partition.c"
#endif

#if defined(CONFIG_MTD_RTKXX_PARTS)
/* Define realtek mtd partion hook function */
static const char * const probes[] = { "rtkxxpart", NULL };
#endif
/**************************************************
 * Various helpers
 **************************************************/
/* realtek */
#if CONFIG_MTD_NAND_RTK_PAGE_SIZE==2048
static struct nand_ecclayout nand_bch_oob_64 = {
	.eccbytes = 41, //ecc 40byte, + 1 bbi 
	.eccpos = {
			23,24,25, 26, 27, 28, 29, 30, 31,
			32,33, 34, 35, 36, 37, 38, 39,
			40,41, 42, 43, 44, 45, 46, 47,
			48,49, 50, 51, 52, 53, 54, 55,
			56,57, 58, 59, 60, 61, 62, 63},
	.oobfree = {
		{.offset = 0,
		 .length = 23}
		 }
};

#endif

#if  CONFIG_MTD_NAND_RTK_PAGE_SIZE==4096
static struct nand_ecclayout nand_bch_oob_128 = {
	.eccbytes = 81, //ecc 80byte, + 1 bbi 
	.eccpos = {
			47,  48,  49,  50,  51, 52, 53, 54, 55, 56,
	  	 	57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
		    67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
		    77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
		    87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
		    97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
		    107,108,109,110,111,112,113,114,115,116,
		    117, 118,119,120,121,122,123,124,125,126,127},
	.oobfree = {
		{.offset = 0,
		 .length = 47}
		 }
};
#endif

#if  CONFIG_MTD_NAND_RTK_PAGE_SIZE==8192
static struct nand_ecclayout nand_bch_oob_256 = {
	.eccbytes = 161, //ecc 80byte, + 1 bbi 
	.eccpos = {
			95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
	  	 	105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
		    115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
		    125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
		    135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
		    145, 146, 147, 148, 149, 150, 151, 152, 153, 154,
		    155,156,157,158,159,160,161,162,163,164,
		    165,166,167,168,169,170,171,172,173,174,
		    175,176,177,178,179,180,181,182,183,184,
		    185,186,187,188,189,190,191,192,193,194,
		    195,196,197,198,199,200,201,202,203,204,
		    205,206,207,208,209,210,211,212,213,214,
		    215,216,217,218,219,220,221,222,223,224,
		    225,226,227,228,229,230,231,232,233,234,
		    235,236,237,238,239,240,241,242,243,244,
		    245,236,247,248,249,250,251,252,253,254,255
		    },
	.oobfree = {
		{.offset = 0,
		 .length = 95}
};
#endif

static void check_ready(void)
{
	while(1) {
		if(( rtk_readl(NACR) & 0x80000000) == 0x80000000) 
			break;
	}
}


#if 0 

/**************************************************
 * R/W
 **************************************************/
/* only used in read oob cmd */
static void rtknflash_ops_read(struct mtd_info *mtd, uint8_t *buf,
					   int len)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	struct rtknflash *rtkn = (struct rtknflash *)nand_chip->priv;

	//mod 
	unsigned int cmd,flash_addr1=0,flash_addr2 = 0,i,data_out;
	unsigned int page = rtkn->curr_page_addr,coloum = rtkn->curr_column;
	unsigned int offset = (1<<this->page_shift) 
	

	BUG_ON(rtkn->curr_page_addr & ~nand_chip->pagemask);
	/* Don't validate column using nand_chip->page_shift, it may be bigger
	 * when accessing OOB */

	rtk_writel(0xc0077777, NACR);

	/* Command cycle 1*/
	rtk_writel((CECS0|CMD_PG_READ_C1), NACMR);
	check_ready();

	if(!NAND_ADDR_CYCLE){
		/* 4 or 5 cycle */
		flash_addr1 =  ((page & 0xff) << 16) | offset;
		flash_addr2 = (page >> 8) & 0xffffff;
		
		rtk_writel( (enNextAD|AD2EN|AD1EN|AD0EN|flash_addr1), NAADR);
		rtk_writel( (AD1EN|AD0EN|flash_addr2), NAADR);
	}
	
	/* Command cycle 2*/
	rtk_writel((CECS0|CMD_PG_READ_C2), NACMR);
	check_ready();

	for(i = 0;i < len/4;i++)
	{
		data_out = rtk_readl(NADR);	
		*buf++ = data_out & 0xff;
		*buf++ = (data_out >> 8) & 0xff;
		*buf++ = (data_out >> 16) & 0xff;
		*buf++ = (data_out >> 24) & 0xff;
	}

	rtk_writel(0, NACMR);
	rtk_writel(0, NAADR);

#ifdef SWAP_2K_DATA
	/* should swap,ecclyaout free area donot need swap */
#endif

	return 0;
}

static void rtknflash_ops_write(struct mtd_info *mtd,
					    const uint8_t *buf, int len)
{
	struct nand_chip *nand_chip = (struct nand_chip *)mtd->priv;
	struct rtknflash *rtkn = (struct rtknflash *)nand_chip->priv;

	/* donot need */
	//struct bcma_drv_cc *cc = rtkn->cc;

	u32 ctlcode;
	const u32 *data = (u32 *)buf;
	int i;

	BUG_ON(rtkn->curr_page_addr & ~nand_chip->pagemask);
	/* Don't validate column using nand_chip->page_shift, it may be bigger
	 * when accessing OOB */

	for (i = 0; i < len; i += 4, data++) {
		/* need realtek */
		
		#if 0
		bcma_cc_write32(cc, BCMA_CC_NFLASH_DATA, *data);

		ctlcode = NCTL_CSA | 0x30000000 | NCTL_WRITE;
		if (i == len - 4) /* Last read goes without that */
			ctlcode &= ~NCTL_CSA;
		if (bcm47xxnflash_ops_bcm4706_ctl_cmd(cc, ctlcode)) {
			pr_err("%s ctl_cmd didn't work!\n", __func__);
			return;
		}
		#endif
	}

	rtkn->curr_column += len;
}

static void rtknflash_ops_read_buf(struct mtd_info *mtd,
					       uint8_t *buf, int len)
{
	struct nand_chip *nand_chip = (struct nand_chip *)mtd->priv;
	struct rtknflash *rtkn = (struct rtknflash *)nand_chip->priv;

	switch (rtkn->curr_command) {
	case NAND_CMD_READOOB:
		rtknflash_ops_read(mtd, buf, len);
		return;
	}

	pr_err("Invalid command for buf read: 0x%X\n", rtkn->curr_command);
}

static void rtknflash_ops_write_buf(struct mtd_info *mtd,
						const uint8_t *buf, int len)
{
	struct nand_chip *nand_chip = (struct nand_chip *)mtd->priv;
	struct rtknflash *rtkn = (struct rtknflash *)nand_chip->priv;

	switch (rtkn->curr_command) {
	case NAND_CMD_SEQIN:
		rtknflash_ops_write(mtd, buf, len);
		return;
	}

	pr_err("Invalid command for buf write: 0x%X\n", rtkn->curr_command);
}

#endif
/**************************************************
 * NAND chip ops
 **************************************************/
static void rtknflash_ops_select_chip(struct mtd_info *mtd,
						  int chip)
{
	unsigned long flags_nand = 0;
	spin_lock_irqsave(&lock_nand, flags_nand);
	switch(chip) {
		case -1:
			rtk_writel(0x0, NACMR);
			break;			
		case 0:
			rtk_writel(CECS0, NACMR);
			break;
		case 1:
			rtk_writel(CECS1, NACMR);
			break;
		default:
			rtk_writel(0x0, NACMR);  //SD5 only support chip1 & chip0
	}
    spin_unlock_irqrestore(&lock_nand, flags_nand);
}

/*
 * Default nand_command and nand_command_lp don't match BCM4706 hardware layout.
 * For example, reading chip id is performed in a non-standard way.
 * Setting column and page is also handled differently, we use a special
 * registers of ChipCommon core. Hacking cmd_ctrl to understand and convert
 * standard commands would be much more complicated.
 */
 
/* need modify to support full id */
static void rtknflash_read_id_cmd(struct rtknflash *rtkn)
{	
	int id_chain;
	unsigned long flags_nand = 0;
	spin_lock_irqsave(&lock_nand, flags_nand);
#if 0
	if(chip_sel>1)  //SD5 only supports chip0, chip1
	{
		spin_unlock_irqrestore(&lock_nand, flags_nand);
		return;
	}
#endif
	check_ready();
	rtk_writel( (rtk_readl(NACR) |ECC_enable|RBO|WBO), NACR);

	rtk_writel( (CECS0|CMD_READ_ID) , NACMR);          //read ID command
	check_ready();

	rtk_writel( (0x0 |AD2EN|AD1EN|AD0EN) , NAADR);  	//dummy address cycle
	check_ready();

	id_chain = rtk_readl(NADR);
	rtkn->id_data[0] = id_chain & 0xff;
	rtkn->id_data[1] = (id_chain >> 8) & 0xff;
	rtkn->id_data[2] = (id_chain >> 16) & 0xff;
	rtkn->id_data[3] = (id_chain >> 24) & 0xff;

	id_chain = rtk_readl(NADR);
	rtkn->id_data[4] = id_chain & 0xff;
	/* full id */
	rtkn->id_data[5] = (id_chain >> 8) & 0xff;
	rtkn->id_data[6] = (id_chain >> 16) & 0xff;
	rtkn->id_data[7] = (id_chain >> 24) & 0xff;

	rtk_writel( 0x0, NACMR);
	rtk_writel( 0x0, NAADR);

	spin_unlock_irqrestore(&lock_nand, flags_nand);
}

/* reset cmd */
static void rtknflash_reset_cmd(struct rtknflash *rtkn)
{
	unsigned long flags_nand = 0;
	spin_lock_irqsave(&lock_nand, flags_nand);

	check_ready();
	rtk_writel( (rtk_readl(NACR) |ECC_enable|RBO|WBO), NACR);
	rtk_writel( (CECS0|CMD_RESET) , NACMR);          
	check_ready();
	
	rtk_writel( 0x0, NACMR);
	spin_unlock_irqrestore(&lock_nand, flags_nand);
}

/* read_status cmd */
static void rtknflash_read_status_cmd(struct rtknflash *rtkn)
{
	unsigned char status;
	unsigned long flags_nand = 0;
	spin_lock_irqsave(&lock_nand, flags_nand);

	check_ready();
	rtk_writel( (rtk_readl(NACR) |ECC_enable|RBO|WBO), NACR);
	rtk_writel( (CECS0|CMD_READ_STATUS) , NACMR);          
	check_ready();

	/* read */
	status =  rtk_readl(NADR) & 0xff;
	rtkn->status = status;
	
	rtk_writel( 0x0, NACMR);
	spin_unlock_irqrestore(&lock_nand, flags_nand);
}



/* erase cmd */
static int rtknflash_erase1_cmd(struct mtd_info* mtd,struct rtknflash *rtkn)
{	
	int addr_cycle[5], page_shift,res = 0;
	int page_addr = rtkn->curr_page_addr;
	unsigned long flags_nand = 0;

	spin_lock_irqsave(&lock_nand, flags_nand);	
#ifdef CONFIG_RTK_REMAP_BBT
	page_addr = rtkn_bbt_get_realpage(mtd,page_addr);
#endif

	//printk("[%s]:%d,page=%x\n",__func__,__LINE__,page_addr);

	check_ready();
	rtk_writel( (rtk_readl(NACR) |ECC_enable), NACR);
	rtk_writel((NWER|NRER|NDRS|NDWS), NASR);
	rtk_writel(0x0, NACMR);
	rtk_writel((CECS0|CMD_BLK_ERASE_C1),NACMR);
	check_ready();
	addr_cycle[0] = addr_cycle[1] =0;
	for(page_shift=0; page_shift<3; page_shift++){
		addr_cycle[page_shift+2] = (page_addr>>(8*page_shift)) & 0xff;
	}
	//rtk_writel( (enNextAD|AD2EN|AD1EN|AD0EN|(addr_cycle[2]<<CE_ADDR2)),NAADR);
	rtk_writel( (((~enNextAD) & AD2EN)|AD1EN|AD0EN|
			(addr_cycle[2]<<CE_ADDR0) |(addr_cycle[3]<<CE_ADDR1)|(addr_cycle[4]<<CE_ADDR2)),NAADR);
	check_ready();
	rtk_writel((CECS0|CMD_BLK_ERASE_C2),NACMR);
	check_ready();
	rtk_writel((CECS0|CMD_BLK_ERASE_C3),NACMR);
	check_ready();

#ifdef RTKN_FLASH_TEST
	/* test case:simulate bak block*/
	if(rtkn_test_num % RTKN_BLOCK_BAD == 0)
	{
		/* as bad block */
		rtkn_test_num++;
		printk("[%s]:%d,page=%x\n",__func__,__LINE__,page_addr);
		goto Error;
	}
	rtkn_test_num++;
#else
	if ( rtk_readl(NADR) & 0x01 ){
		goto Error;	
	}
#endif
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	return 0;

Error:
#ifdef CONFIG_RTK_REMAP_BBT
	printk("[%s]:%d,write fail,page=%x\n",__func__,__LINE__,page_addr);
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	res = rtk_remapBBT_erase_fail(mtd,page_addr);
	return res;
#else
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	//printk("[%s] erasure is not completed at block %d\n", __FUNCTION__, page/ppb);
	printk("erase error\n");
	return -1;
#endif
}
 
static void rtknflash_ops_cmdfunc(struct mtd_info *mtd,
					      unsigned command, int column,
					      int page_addr)
{
	struct nand_chip *nand_chip = (struct nand_chip *)mtd->priv;
	struct rtknflash *rtkn = (struct rtknflash *)nand_chip->priv;
	/* donot need */
	//struct bcma_drv_cc *cc = rtkn->cc;
	//u32 ctlcode;
	//int i;

	if (column != -1)
		rtkn->curr_column = column;
	if (page_addr != -1)
		rtkn->curr_page_addr = page_addr;
		
	/* default val */
	rtkn->cmd_status = 0;
	
	switch (command) {
	case NAND_CMD_RESET:
		/* not support now */
		rtknflash_reset_cmd(rtkn);
		break;
	case NAND_CMD_READID:
		/* read nand chip id; store in id_data */
		rtknflash_read_id_cmd(rtkn);
		break;
	case NAND_CMD_STATUS:
		/* not support now */
		rtknflash_read_status_cmd(rtkn);
		break;
	#if 1
	case NAND_CMD_READ0:
		/* not support now */
		break;
	case NAND_CMD_READOOB:
		/* not support now */
		break;
	#endif
	case NAND_CMD_ERASE1:
		/* erase1 */
		if(rtknflash_erase1_cmd(mtd,rtkn) < 0)
			rtkn->cmd_status = NAND_STATUS_FAIL;
		break;
	case NAND_CMD_ERASE2:
		/* erase2 */
		break;
	#if 1
	case NAND_CMD_SEQIN:
		/* not support now*/
		break;
	case NAND_CMD_PAGEPROG:
		/* not support now */		
		break;
	#endif
	default:
		pr_err("Command 0x%X unsupported\n", command);
		break;
	}
	rtkn->curr_command = command;
}


static u8 rtknflash_ops_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = (struct nand_chip *)mtd->priv;
	struct rtknflash *rtkn = (struct rtknflash *)nand_chip->priv;
	//u32 tmp = 0;

	switch (rtkn->curr_command) {
	case NAND_CMD_READID:
		if (rtkn->curr_column >= ARRAY_SIZE(rtkn->id_data)) {
			pr_err("Requested invalid id_data: %d\n",
			       rtkn->curr_column);
			return 0;
		}
		return rtkn->id_data[rtkn->curr_column++];
	case NAND_CMD_STATUS:
		return rtkn->status; 
#if 0
	case NAND_CMD_READOOB:
		rtknflash_ops_read(mtd, (u8 *)&tmp, 4);
		return tmp & 0xFF;
#endif
	}
	

	pr_err("Invalid command for byte read: 0x%X\n", rtkn->curr_command);
	return 0;
}

int rtk_check_allone(int page, int offset)
{
	unsigned int flash_addr1, flash_addr2;
	unsigned int data_out;
	int real_page, i, rlen;;

	real_page = page;

	/* rlen is equal to (512 + 16) */
	rlen = 528; 
	
	rtk_writel(0xc00fffff, NACR);

	/* Command cycle 1*/
	rtk_writel((CECS0|CMD_PG_READ_C1), NACMR);

	check_ready();

	flash_addr1 =  ((real_page & 0xff) << 16) | offset;
	flash_addr2 = (real_page >> 8) & 0xffffff;

	/* Give address */
	rtk_writel( (enNextAD|AD2EN|AD1EN|AD0EN|flash_addr1), NAADR);
	rtk_writel( (AD1EN|AD0EN|flash_addr2), NAADR);

	/* Command cycle 2*/
	rtk_writel((CECS0|CMD_PG_READ_C2), NACMR);

	check_ready();

	for(i=0; i<(rlen/4); i++){
		data_out = rtk_readl(NADR);	
		if( data_out != 0xffffffff)
			return -1;
	}

	check_ready();
	rtk_writel(0, NACMR);

	return 0;

}

int rtk_PIO_read_basic(int page, int offset)
{
	int i;
	unsigned int flash_addr1=0;
	unsigned int data_out;
	int rlen, real_page;
	unsigned int cmd;
	real_page = page;
	
	rlen = 2048+64;
	cmd = CMD_PG_READ_A;

	rtk_writel(0xc0077777, NACR);

	/* Command cycle 1*/
	rtk_writel((CECS0|cmd), NACMR);

	check_ready();

	flash_addr1 |= ((real_page & 0xffffff) << 8) ;

	/* Give address */
	rtk_writel( (AD2EN|AD1EN|AD0EN|flash_addr1), NAADR);

	check_ready();

	for(i=0; i<(rlen/4); i++){
		data_out = rtk_readl(NADR);
		//printf("[%3d] 0x%08X \n",i, data_out);
		if(data_out!=0xffffffff){
			printk("[%3d] 0x%08X \n",i, data_out);
			printk("%s: page %d offset %d i %d rlen %d\n",__FUNCTION__, page, offset, i, rlen);			
			return -1;
		}
	}

	check_ready();
	rtk_writel(0, NACMR);
	
	return 0;
	
}

int rtk_check_allone_512(int page)
{
	int rc=0;

	//printf("[%s] page = %d\n",__func__,page);
	rc = rtk_PIO_read_basic(page,0);
	if(rc < 0)
		goto read_finish;

read_finish:
	return rc;
}

int rtk_check_pageData(struct mtd_info *mtd, u16 chipnr,int page, int offset, int isLastSector)
{
	//struct nand_chip *this = (struct nand_chip *) mtd->priv;
	//unsigned int chip_section = (chipnr * this->page_num) >> 5;
	//unsigned int chip_section = 0;
	//unsigned int section = page >> 5;
	//unsigned int index = page & (32-1);

	int rc = 0;
	int error_count,status;

	status = rtk_readl(NASR);
//printk("[%s:%d] status=%x page %x offset %x idx %x\n",__FUNCTION__,__LINE__,status, page,offset, index);

	if( (status & NDRS)== NDRS){		

		#if 0
		if (this->erase_page_flag[chip_section+section] & (1<< index) ){
			//printk("[%s:%d]RRR page=%u status=%x\n",__FUNCTION__,__LINE__,page, status);
			;
		}else
		#endif
		
		if( status & NRER) {
			error_count = (status & 0xf0) >> 4;
			if(error_count <=4 && error_count > 0 ) {
				printk("[%s] R: Correctable HW ECC Error at page=%u, status=0x%08X\n\r", __FUNCTION__, page,status);
				status &= 0x0f; //clear NECN
				rtk_writel(status, NASR);
				return 0;
			}else{
			
			    if(!NAND_ADDR_CYCLE){
			        if( rtk_check_allone(page,offset) == 0 ){
				        //printf("[%s] Page %d is all one page, bypass it !!\n\r",__func__,page);
						status &= 0x0f; //clear NECN
						rtk_writel(status, NASR);
				        return 0;
			        }
			    }else{
				    if( rtk_check_allone_512(page) == 0 ){
					    //printk("[%s] Page %d is all one page, bypass it !!\n\r",__func__,page);
						status &= 0x0f; //clear NECN
					    rtk_writel(status, NASR);
					    return 0;
				    }
			    }			
				printk("[%s] R: Un-Correctable HW ECC Error at page=%u, status=0x%08X\n\r", __FUNCTION__, page,status);
				status &= 0x0f; //clear NECN
				rtk_writel(status, NASR);
				return -1;				
			}
		}
		
	}
	else if( (status & NDWS)== NDWS){
			#if 0
		 	if (this->erase_page_flag[chip_section+section] & (1<< index) ){
			    //printk("[%s] AAAA NAND Flash write at page=%u, status=0x%08X\n\r", __FUNCTION__, page,status);
			    //printk("[%s] this->erase_page_flag[%d]:0x%x, %x:%x\n\r", __FUNCTION__,chip_section+section,this->erase_page_flag[chip_section+section],index, 1 << index);				
			;
			}else
			#endif
			
			if( status & NWER) {
			    printk("[%s] A NAND Flash write failed at page=%u, status=0x%08X\n\r", __FUNCTION__, page,status);

			    status &= 0x0f; //clear NECN
				rtk_writel(status, NASR);
			    //rtk_writel(status, NASR);
			    return -1;				
		    }
			//printk("[%s] B NAND Flash write at page=%u, status=0x%08X\n\r", __FUNCTION__, page,status);

	}

	status &= 0x0f; //clear NECN
	rtk_writel(status, NASR);
//printk("[%s:%d] status=%x page %u offset %u idx %d\n",__FUNCTION__,__LINE__,status, page,offset, index);

	return rc;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static int rtkn_ecc_read_page(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf)
#else
static int rtkn_ecc_read_page(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, int oob_required, int page)
#endif
{
	//struct rtknflash *rtkn = (struct rtknflash *)chip->priv;
	int dma_counter,page_shift,page_num[3],buf_pos=0;
	int dram_sa, oob_sa;
	unsigned long flash_addr_t=0,flash_addr2_t=0;
	unsigned long flags_nand = 0;
	uint8_t* oobBuf = chip->oob_poi;
	/* for 2.6.30 have only three param */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	int realpage = rtkn->curr_page_addr;
#else
	int realpage = page;
#endif
	int orig_block = realpage/(mtd->erasesize/mtd->writesize);
	uint8_t *oob_area,*data_area,data_area0[512+16+CACHELINE_SIZE+CACHELINE_SIZE-4]; //data,oob point must 32 cache aligment
	
	//printk("[%s]:%d,page=%x\n",__func__,__LINE__,page);
	nand_printf("[%s]:%d\n",__func__,__LINE__);
	spin_lock_irqsave(&lock_nand, flags_nand);

	/* get the real page */
#ifdef CONFIG_RTK_REMAP_BBT
	realpage = rtkn_bbt_get_realpage(mtd,realpage);
#endif

	//__flush_cache_all();
	/* addr */
	memset(data_area0, 0xff, 512+16+CACHELINE_SIZE+CACHELINE_SIZE-4);
	data_area = (uint8_t*) ((uint32_t)(data_area0 + CACHELINE_SIZE-4) & 0xFFFFFFF0); 
	oob_area=(uint8_t*) data_area+512;
	oob_sa =  ( (uint32_t)(oob_area ) & (~M_mask));
	dram_sa = ((uint32_t)data_area) & (~M_mask);	

	/* flash addr */
	dma_counter = (mtd->writesize) >> 9;
	for(page_shift=0;page_shift<3; page_shift++) {
		page_num[page_shift] = ((realpage>>(8*page_shift)) & 0xff);
		if(mtd->writesize == 2048){
			flash_addr_t |= (page_num[page_shift] << (12+8*page_shift));
		}else if(mtd->writesize == 4096){
		  flash_addr_t |= (page_num[page_shift] << (13+8*page_shift));
		}else if(mtd->writesize == 8192){
		  flash_addr_t |= (page_num[page_shift] << (14+8*page_shift));
		}else if(mtd->writesize == 512){ 
			flash_addr_t |= (page_num[page_shift] << (9+8*page_shift));
		}
	}

	/* if nand flash chip > 2G byte */
	flash_addr2_t= (realpage >> 20);
	
#if 0
	for(page_shift=0;page_shift<3; page_shift++) {
	    page_num[page_shift] = ((realpage>>(8*page_shift)) & 0xff);
		flash_addr_t |= (page_num[page_shift] << (12+8*page_shift));
	}
#endif

	rtk_writel(0xC00FFFFF, NACR);     //Enable ECC	
	rtk_writel(0x0000000F, NASR);	  //clear NAND flash status register

	while(dma_counter>0){

  		int lastSec = dma_counter-1;
		//set DMA RAM DATA start address		
  		rtk_writel(dram_sa, NADRSAR);  		
		//set DMA RAM oob start address , always use oob_sa buffer
		rtk_writel(oob_sa, NADTSAR);
		//set DMA flash start address,
 		rtk_writel( flash_addr_t, NADFSAR);
		//set DMA flash start address2,
 		//rtk_writel( flash_addr2_t, NADFSAR2);

		dma_cache_wback_inv((unsigned long)data_area,528);		
		//DMA read command
	    rtk_writel( ((~TAG_DIS)&(DESC0|DMARE|LBC_64)),NADCRR);
		check_ready();
		
		if(lastSec<=0)
			lastSec =1;
		else
			lastSec =0;

		if(rtk_check_pageData(mtd,0,realpage,buf_pos*(512+16),lastSec)==-1)
		{
		    goto Error;
		}

		//copy data
		memcpy(buf+(buf_pos*512), data_area, 512);
		//copy oob
		memcpy(oobBuf +(buf_pos*6), oob_area, 6);			

		flash_addr_t += 528;
		dma_counter--;
		buf_pos++; 
	}

	#ifdef SWAP_2K_DATA
	if(orig_block >= BOOT_BLOCK ){
		if(!NAND_ADDR_CYCLE)
		{
			/*switch bad block info*/
			unsigned char temp_val=0;
			{
				temp_val = buf[DATA_BBI_OFF];
				{
					buf[DATA_BBI_OFF] = oobBuf[OOB_BBI_OFF];
					oobBuf[OOB_BBI_OFF] = temp_val;				
				}
			}
		}
	}
	#endif

	nand_printf("[%s]:%d\n",__func__,__LINE__);
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	return 0;

Error:
/* read function donot need do bbt */
	/* read may ecc error; */
#ifdef CONFIG_RTK_REMAP_BBT
	printk("[%s]:%d,read fail\n",__func__,__LINE__);
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	rtk_remapBBT_read_fail(mtd,page);
	return 0;
#else
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	printk("rtk_check_pageData return fail...\n");
	return -1;
#endif
}

#ifdef CONFIG_JFFS2_FS
/* jffs2 clearmark at first page of block */
/* if write first page of block and find oob_area is not 0xff,need reerase this block */
#define  	JFFS2_NEED_REERAE 			0x1
#define 	JFFS2_NO_NEED_REERAE		0x2

static int rtkn_jffs2_write_patch_check(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t *buf, int oob_required,int page)
{
	int i,res;
	struct rtknflash* rtkn = (struct rtknflash*)chip->priv;
	unsigned int oobsize=mtd->oobsize,pagesize=mtd->writesize;
	unsigned int ppb = (mtd->erasesize)/(mtd->writesize);

	if(page % ppb == 0){
		memset(rtkn->buf,0xff,MAX_RTKN_BUF_SIZE);
		res = rtk_scan_read_oob(mtd,rtkn->buf,page,pagesize);
		if(res){
			printk("[%s]:%d:read ecc page fail\n",__func__,__LINE__);
			return res;
		}
		for(i = 0;i< oobsize;i++){
			if(rtkn->buf[pagesize+i] != 0xff)
				return JFFS2_NEED_REERAE;
		}

	}

	return JFFS2_NO_NEED_REERAE;
}

static int rtkn_jffs2_write_patch(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t *buf, int oob_required,int page)
{
	int res = 0,i;
	struct rtknflash* rtkn = (struct rtknflash*)chip->priv;
	unsigned int oobsize=mtd->oobsize,pagesize=mtd->writesize;
	unsigned char* blocktmp=NULL;
	unsigned int ppb = (mtd->erasesize)/(mtd->writesize);

	{
		/* this page have write;need reerase the whole block */
		
		blocktmp = (unsigned char*)kmalloc((pagesize+oobsize)*ppb,GFP_KERNEL);
		if(!blocktmp){
			return -1;
		}
		memset(blocktmp,0xff,(pagesize+oobsize)*ppb);

		memcpy(blocktmp,buf,pagesize);
		memcpy(blocktmp+pagesize,rtkn->buf+pagesize,oobsize);

		/* read */
		for(i = 1;i<ppb;i++){
			res = rtk_scan_read_oob(mtd,blocktmp+(pagesize+oobsize)*i,page+i,pagesize);
			if(res < 0)
				goto Error;
		}

		/* erase */
		if(rtk_scan_erase_bbt(mtd,page) < 0)
			goto Error;

		/*write*/
		for(i = 0;i< ppb;i++){
			memset(rtkn->buf,0xff,MAX_RTKN_BUF_SIZE);
			if(memcmp(blocktmp + i*(pagesize+oobsize),rtkn->buf,pagesize+oobsize) == 0) 
				continue;
		
			res = rtk_scan_write_bbt(mtd,page+i,pagesize,blocktmp+i*(pagesize+oobsize),blocktmp+i*(pagesize+oobsize)+pagesize);
			if(res < 0){
				goto Error;
			}
		}
		goto FINISH;
	}

Error:
	if(blocktmp)
		kfree(blocktmp);
	return -1;
FINISH:
	if(blocktmp)
		kfree(blocktmp);
	return 0;
}
#endif

static int rtkn_ecc_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t *buf, int oob_required)
{
	struct rtknflash *rtkn = (struct rtknflash *)chip->priv;
	int page = rtkn->curr_page_addr,dma_counter = 4,page_shift,page_num[3],buf_pos=0;
	int dram_sa, oob_sa;
	unsigned long flash_addr_t=0,flash_addr2_t=0;
	unsigned long flags_nand = 0;
	unsigned int ppb = mtd->erasesize/mtd->writesize;
	uint8_t* dataBuf = (uint8_t*)buf,*oobBuf = (uint8_t*)chip->oob_poi;
	int orig_block = page/ppb,res = 0;
	uint8_t *oob_area,*data_area,data_area0[512+16+CACHELINE_SIZE+CACHELINE_SIZE-4]; //data,oob point must 32 cache aligment
		
	spin_lock_irqsave(&lock_nand, flags_nand);
	 //__flush_cache_all();

#ifdef CONFIG_RTK_REMAP_BBT
	page = rtkn_bbt_get_realpage(mtd,page);
#endif


#ifdef CONFIG_JFFS2_FS
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	res = rtkn_jffs2_write_patch_check(mtd,chip,buf,oob_required,page);
	if(res < 0){
		printk("donot write page\n");
		return res;
	}
	else if(res == JFFS2_NEED_REERAE){
		//printk("need reerase block,page=%x\n",page);
		res = rtkn_jffs2_write_patch(mtd,chip,buf,oob_required,page);
		return res;
	}
	
	spin_lock_irqsave(&lock_nand, flags_nand);
#endif

	/* addr */
	memset(data_area0, 0xff, 512+16+CACHELINE_SIZE+CACHELINE_SIZE-4);
	data_area = (uint8_t*) ((uint32_t)(data_area0 + 12) & 0xFFFFFFF0);
	oob_area=(uint8_t*) data_area+512;
	oob_sa =  ( (uint32_t)(oob_area ) & (~M_mask));
	dram_sa = ((uint32_t)data_area) & (~M_mask);

	#ifdef SWAP_2K_DATA
	if(orig_block >= BOOT_BLOCK){		
		if(!NAND_ADDR_CYCLE)
		{
			unsigned char temp_val;
			{
				temp_val = dataBuf[DATA_BBI_OFF];
				{
					dataBuf[DATA_BBI_OFF] = oobBuf[OOB_BBI_OFF];
					oobBuf[OOB_BBI_OFF] = temp_val;
				}
			}
		}
	}
	#endif

	/* flash addr */
	dma_counter = (mtd->writesize) >> 9;
	for(page_shift=0;page_shift<3; page_shift++) {
		page_num[page_shift] = ((page>>(8*page_shift)) & 0xff);
		if(mtd->writesize == 2048){
			flash_addr_t |= (page_num[page_shift] << (12+8*page_shift));
		}else if(mtd->writesize == 4096){
		  flash_addr_t |= (page_num[page_shift] << (13+8*page_shift));
		}else if(mtd->writesize == 8192){
		  flash_addr_t |= (page_num[page_shift] << (14+8*page_shift));
		}else if(mtd->writesize == 512){ 
			flash_addr_t |= (page_num[page_shift] << (9+8*page_shift));
		}
	}
	#if 0
	for(page_shift=0;page_shift<3; page_shift++) {
	    page_num[page_shift] = ((page>>(8*page_shift)) & 0xff);
		flash_addr_t |= (page_num[page_shift] << (12+8*page_shift));
	}
	#endif
	/* if nand flash chip > 2G byte */
	flash_addr2_t= (page >> 20);
	
	rtk_writel(0xC00FFFFF, NACR);     //Enable ECC	
	rtk_writel(0x0000000F, NASR);	  //clear NAND flash status register
	
	while(dma_counter>0){

  		int lastSec = dma_counter - 1;
		memcpy(oob_area, oobBuf+(buf_pos*6), 6);	
		memcpy(data_area, dataBuf+(buf_pos*512), 512);
		dma_cache_wback_inv((unsigned long)data_area,528);//512+16
		rtk_writel( rtk_readl(NACR) & (~RBO) & (~WBO) , NACR);
		//write data/oob address
		rtk_writel(dram_sa, NADRSAR);
		rtk_writel(oob_sa, NADTSAR);
		rtk_writel(flash_addr_t, NADFSAR);
		//rtk_writel(flash_addr2_t, NADFSAR2);
		
		rtk_writel( ((DESC0|DMAWE|LBC_64) & (~TAG_DIS)),NADCRR);
		check_ready();

		if(lastSec<=0)
			lastSec =1;
		else
			lastSec =0;

#ifndef RTKN_FLASH_TEST_WRITE
		if(rtk_check_pageData(mtd,0,page,buf_pos*(512+16),lastSec)==-1)
		{
#ifdef SWAP_2K_DATA
			if(orig_block >= BOOT_BLOCK){		
				if(!NAND_ADDR_CYCLE)
				{
					unsigned char temp_val;
					{
						temp_val = dataBuf[DATA_BBI_OFF];
						{
							dataBuf[DATA_BBI_OFF] = oobBuf[OOB_BBI_OFF];
							oobBuf[OOB_BBI_OFF] = temp_val;
						}
					}
				}
			}
#endif

			goto Error;

		}
#else
		if(rtkn_test_write % RTKN_BLOCK_BAD ==0)
		{
			rtkn_test_write++;
#ifdef SWAP_2K_DATA
			if(orig_block >= BOOT_BLOCK){		
				if(!NAND_ADDR_CYCLE)
				{
					unsigned char temp_val;
					{
						temp_val = dataBuf[DATA_BBI_OFF];
						{
							dataBuf[DATA_BBI_OFF] = oobBuf[OOB_BBI_OFF];
							oobBuf[OOB_BBI_OFF] = temp_val;
						}
					}
				}
			}
#endif
			goto Error;
		}
		rtkn_test_write++;
#endif
		rtk_writel(0xF, NASR);	

		flash_addr_t += (528); //512+16 one unit

		dma_counter--;
		buf_pos++;
	}
	#ifdef SWAP_2K_DATA
	if(orig_block >= BOOT_BLOCK){		
		if(!NAND_ADDR_CYCLE)
		{
			unsigned char temp_val;
			{
				temp_val = dataBuf[DATA_BBI_OFF];
				{
					dataBuf[DATA_BBI_OFF] = oobBuf[OOB_BBI_OFF];
					oobBuf[OOB_BBI_OFF] = temp_val;
				}
			}
		}
	}
	#endif
	
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	return 0;

Error:
#ifdef CONFIG_RTK_REMAP_BBT
	printk("[%s]:%d,write fail,page=%x\n",__func__,__LINE__,page);
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	res = rtk_remapBBT_write_fail(mtd,page,buf,oob_required);
	return res;
#else
	spin_unlock_irqrestore(&lock_nand, flags_nand);
	printk("rtk_check_pageData return fail...\n");
	return -1;
#endif

	
}

static int rtkn_ecc_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
				int page)
{
	struct rtknflash *rtkn = (struct rtknflash *)chip->priv;
	//pr_debug("page number is %d\n", page);
	/* clear the OOB buffer */
	memset(chip->oob_poi, ~0, mtd->oobsize);
	memset(rtkn->buf,~0,MAX_RTKN_BUF_SIZE);



#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	/* for 2.6.30 kernel */
	rtkn->curr_page_addr = page;
	rtkn_ecc_read_page(mtd,chip,rtkn->buf);
#else
	rtkn_ecc_read_page(mtd,chip,rtkn->buf,1,page);
#endif
	return 0;
}


static int rtkn_ecc_write_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	struct rtknflash *rtkn = (struct rtknflash *)chip->priv;

	/*check if page is have written */
	memset(rtkn->buf,~0,MAX_RTKN_BUF_SIZE);
	rtkn->curr_page_addr = page;
	
	rtkn_ecc_write_page(mtd,chip,rtkn->buf,1);

	return 0;
}

static int rtknflash_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	struct rtknflash *rtkn = (struct rtknflash *)chip->priv;
	return rtkn->cmd_status;
}
/**************************************************
 * Init
 **************************************************/ 
int rtknflash_ops_init(struct rtknflash *rtkn)
{
	int err;
#if defined(CONFIG_RTK_NAND_FLASH_STORAGE)
#if defined(CONFIG_RTL_8198C)
	unsigned int reg_setval = 0;
#endif
#endif

	/* basic */
	rtkn->nand_chip->select_chip 		= rtknflash_ops_select_chip;
	rtkn->nand_chip->cmdfunc 			= rtknflash_ops_cmdfunc;
	rtkn->nand_chip->waitfunc			= rtknflash_wait;

	rtkn->nand_chip->read_byte 			= rtknflash_ops_read_byte;
#if 0
	rtkn->nand_chip.read_buf 			= rtknflash_ops_read_buf;
	rtkn->nand_chip.write_buf 			= rtknflash_ops_write_buf;
#endif
	/* bbt */
#if defined(CONFIG_RTK_REMAP_BBT) || defined(CONFIG_RTK_NORMAL_BBT)
	rtkn->nand_chip->scan_bbt			= rtkn_scan_bbt;
	rtkn->nand_chip->block_bad			= rtkn_block_bad;
	rtkn->nand_chip->block_markbad		= rtkn_block_markbad;
#else
	/* use default bbt: not support */
	rtkn->nand_chip->bbt_options 		= NAND_BBT_USE_FLASH | NAND_BBT_NO_OOB;
#endif
	/* ecc */
	rtkn->nand_chip->ecc.read_page		= rtkn_ecc_read_page;
	rtkn->nand_chip->ecc.write_page		= rtkn_ecc_write_page;
	rtkn->nand_chip->ecc.read_oob		= rtkn_ecc_read_oob;
	rtkn->nand_chip->ecc.write_oob		= rtkn_ecc_write_oob;
	/* raw function */
	rtkn->nand_chip->ecc.read_page_raw	= rtkn_ecc_read_page;
	rtkn->nand_chip->ecc.write_page_raw	= rtkn_ecc_write_page;
	
	/* ecc mode */
	rtkn->nand_chip->ecc.mode			= NAND_ECC_HW;

	rtkn->nand_chip->ecc.size			= 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
	rtkn->nand_chip->ecc.strength		= 8;
#endif

	/* 8k page */
#if CONFIG_MTD_NAND_RTK_PAGE_SIZE==8192
	rtkn->nand_chip->ecc.layout 		= &nand_bch_oob_256;
#endif
	/* 4k page */
#if CONFIG_MTD_NAND_RTK_PAGE_SIZE==4096
	rtkn->nand_chip->ecc.layout 		= &nand_bch_oob_128;
#endif
	/* 2k page */
#if CONFIG_MTD_NAND_RTK_PAGE_SIZE==2048
	rtkn->nand_chip->ecc.layout 		= &nand_bch_oob_64;
#endif

	/* chip option */
	rtkn->nand_chip->options			|= NAND_NO_SUBPAGE_WRITE;
	/* for 2.6.30 kernel used */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	rtkn->nand_chip->options			|= NAND_NO_AUTOINCR;
#endif

	/* Enable NAND flash access */
#ifdef CONFIG_RTL_8325D_SUPPORT
	#define PINMUX_REG_8196D_97D   0xb8000040
	REG32(PINMUX_REG_8196D_97D) = (REG32(PINMUX_REG_8196D_97D) & ~((7<<0) | (3<<3) | (3<<8) | (3<<12)))
		| ((0<<0) | (1<<3) | (1<<8) | (1<<12));
#elif !defined(CONFIG_RTL_8198C)
	#define PINMUX_REG_8196D_97D   0xb8000040
	#define REG32(reg)	(*(volatile unsigned int   *)((unsigned int)reg))
	REG32 (PINMUX_REG_8196D_97D) &= 0xFFFFCCE9;
	REG32 (PINMUX_REG_8196D_97D) = 0x1508;
#endif

#if defined(CONFIG_RTK_NAND_FLASH_STORAGE)
#if defined(CONFIG_RTL_8198C)
	#define clk_manage_REG	0xb8000010
	#define PINMUX_SEL_1 	0xb8000100
	#define PINMUX_SEL_2 	0xb8000104
	#define PINMUX_SEL_3 	0xb8000108

	REG32(clk_manage_REG) |= ((1 << 28)| (3<<12)| (7<<18));
	REG32 (PINMUX_SEL_1) |=((1<<26));
	REG32 (PINMUX_SEL_2) |=((1<<23));
	REG32 (PINMUX_SEL_3) |=((1<<15) | (1<<18) | (1<<21) | (1<<24) | (1<<27)  | (1<<30) ); 

	#define HW_Strap_REG 0xb8000008
	#define RC_Pages8KB  (3<<28)
	#define RC_Pages4KB (2<<28)
	#define RC_Pages2KB (1<<28)
	#define RC_Pages512B  (0<<28)

	#define AC_5cycles  (2<<26)
	#define AC_4cycles  (1<<26)
	#define AC_3cycles (0<<26)

	/* page size */
#if CONFIG_MTD_NAND_RTK_PAGE_SIZE == 2048
	reg_setval |= RC_Pages2KB;
#elif CONFIG_MTD_NAND_RTK_PAGE_SIZE == 4096
	reg_setval |= RC_Pages4KB;
#elif CONFIG_MTD_NAND_RTK_PAGE_SIZE == 8192
	reg_setval |= RC_Pages8KB;
#else
	#error "page size not support"
#endif

	/* cycle value */
#if CONFIG_MTD_NAND_RTK_CYCLE ==  4
	reg_setval |= AC_4cycles;
#elif CONFIG_MTD_NAND_RTK_CYCLE == 5
	reg_setval |= AC_5cycles;
#else
	#error "cycle not support"
#endif

	//REG32 (HW_Strap_REG) |=( RC_Pages2KB |AC_4cycles );
	REG32 (HW_Strap_REG) |= reg_setval;
#endif
#endif
	
	/* Scan NAND */
	err = nand_scan(rtkn->mtd,1);
	if (err) {
		pr_err("Could not scan NAND flash: %d\n", err);
		goto exit;
	}
	
exit:
	return err;
}


/* nand info proc */
#ifndef __UBOOT__
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int rtk_read_proc_nandinfo(struct seq_file *s, void *v)
{
	struct nand_chip	*this = (struct nand_chip *) rtkn->nand_chip;
	struct mtd_info		*mtd = (struct mtd_info *)rtkn->mtd;
	//int wlen = 0;

	//seq_printf(s,"nand_size:%u\n", this->device_size);
	seq_printf(s,"chip_size:%lld\n", this->chipsize);
	seq_printf(s,"block_size:%u\n", mtd->erasesize);
	seq_printf(s,"page_size:%u\n", mtd->writesize);
	seq_printf(s,"oob_size:%u\n", mtd->oobsize);
	seq_printf(s,"ppb:%u\n", mtd->erasesize/mtd->writesize);
#ifdef CONFIG_RTK_REMAP_BBT
	seq_printf(s,"RBA:%u\n", rtkn->RBA);
#endif	
#ifdef CONFIG_RTK_NORMAL_BBT
	seq_printf(s,"bbt_num:%u\n", rtkn->bbt_num);
#endif	
	seq_printf(s,"BBs:%u\n", rtkn->BBs);

	return 0;
	
}
#else
int rtk_read_proc_nandinfo(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	struct nand_chip	*this = (struct nand_chip *) rtkn->nand_chip;
	struct mtd_info		*mtd = (struct mtd_info *)rtkn->mtd;
	int wlen = 0;

	//wlen += sprintf(buf+wlen,"nand_PartNum:%s\n", rtk_mtd->PartNum); //czyao
	//wlen += sprintf(buf+wlen,"nand_size:%u\n", this->device_size);
	wlen += sprintf(buf+wlen,"chip_size:%lld\n", this->chipsize);
	wlen += sprintf(buf+wlen,"block_size:%u\n", mtd->erasesize);
	wlen += sprintf(buf+wlen,"page_size:%u\n", mtd->writesize);
	wlen += sprintf(buf+wlen,"oob_size:%u\n", mtd->oobsize);
	wlen += sprintf(buf+wlen,"ppb:%u\n", mtd->erasesize/mtd->writesize);
#ifdef CONFIG_RTK_REMAP_BBT
	wlen += sprintf(buf+wlen,"RBA:%u\n", rtkn->RBA);
#endif	
#ifdef CONFIG_RTK_NORMAL_BBT
	wlen += sprintf(buf+wlen,"bbt_num:%u\n", rtkn->bbt_num);
#endif	
	wlen += sprintf(buf+wlen,"BBs:%u\n", rtkn->BBs);
	return wlen;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
int rtk_nand_read_open(struct inode *inode, struct file *file)
{
	return(single_open(file, rtk_read_proc_nandinfo, NULL));
}

static const struct file_operations rtk_nand_flash_ops = {
	.owner = THIS_MODULE,
	.open			= rtk_nand_read_open,
	.read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};
#endif
#endif

/*************************************************************************/
enum NAND_PARAM_TYPE
{
	NAND_BLOCKSIZE = 0,
	NAND_PAGESIZE,
	NAND_OOBSIZE,
	NAND_SIZE,
};

#define SKIP_RESERVE_PERSECT	5
#define REMAP_RESERVE_PERSECT	5

static int rtknflash_get_nand_param(enum NAND_PARAM_TYPE type,void* value)
{
	if(value == NULL)
		return -1;

	switch(type){
	case NAND_BLOCKSIZE:
		*((unsigned int*)value) = rtkn->mtd->erasesize;
		break;
	case NAND_PAGESIZE:
		*((unsigned int*)value) = rtkn->mtd->writesize;
		break;
	case NAND_OOBSIZE:
		*((unsigned int*)value) = rtkn->mtd->oobsize;
		break;
	case NAND_SIZE:
		*((unsigned int*)value) = rtkn->mtd->size;
		break;
	default:
		return -1;
	}
	return 0;
}


int rtkn_check_nand_partition(struct mtd_partition *parts,int partnum)
{
	unsigned int size,blocksize,blocknum,nandsize;
	int i,maxpart = 0;
	unsigned int maxsize = 0;

	for(i=0;i < partnum;i++){
		if((parts[i].offset + parts[i].size) > maxsize){
			maxsize = parts[i].offset + parts[i].size;
			maxpart = i;
		}
	}
	
	if(rtknflash_get_nand_param(NAND_SIZE,&size) != 0){
		return -1;
	}

	if(rtknflash_get_nand_param(NAND_BLOCKSIZE,&blocksize) != 0){
		return -1;
	}

	blocknum = size/blocksize;
	blocknum -= (blocknum/(SKIP_RESERVE_PERSECT+REMAP_RESERVE_PERSECT)+1);
	nandsize = blocknum*blocksize;
	
	if((parts[maxpart].offset+parts[maxpart].size) > nandsize){
		printk("nand flash must reserve %d skip_bbt size and %d remap_bbt_size\n",SKIP_RESERVE_PERSECT,REMAP_RESERVE_PERSECT);
		printk("last partition offset:%llx,size:%llx,nand flash max size: %x\n",parts[maxpart].offset,
										parts[maxpart].size,nandsize);	
		return -1;
	}
	
	return 0;
}
EXPORT_SYMBOL(rtkn_check_nand_partition);

/*************************************************************************/

int  rtknflash_lowinit(struct mtd_info *mtd,struct nand_chip* nand)
{
	int err = 0;

	/* nand */
	rtkn = kmalloc(sizeof(struct rtknflash),GFP_KERNEL);
	if(!rtkn)
	{
		err = -ENOMEM;
		goto  out;
	}

	/* nand */
	rtkn->nand_chip = nand;
	rtkn->mtd = mtd;
	nand->priv = rtkn;
	mtd->owner = THIS_MODULE;
	mtd->priv = nand;
	mtd->name = "rtk_nand";
	
	err = rtknflash_ops_init(rtkn);
	if (err) {
		printk("Initialization failed: %d\n", err);
		goto err_init;
	}
	
	/* create partition and nandinfo proc */
#ifndef __UBOOT__
#ifndef CONFIG_WRT_BARRIER_BREAKER
#if 0//def CONFIG_MTD_CMDLINE_PARTS
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	int pnum = 0;
        struct mtd_partition *mtd_parts;

        pnum = parse_mtd_partitions (mtd, probes, &mtd_parts, 0);
        if (pnum <= 0) {
                printk(KERN_NOTICE "RTK: using the whole nand as a partitoin\n");
                if(add_mtd_device(mtd)) {
                        printk(KERN_WARNING "RTK: Failed to register new nand device\n");
                        err = EAGAIN;
			goto err_init;
                }
        }else{
                printk(KERN_NOTICE "RTK: using dynamic nand partition\n");
                if (add_mtd_partitions (mtd, mtd_parts, pnum)) {
                        printk("%s: Error, cannot add %s device\n",
                                        __FUNCTION__, mtd->name);
                        mtd->size = 0;
                        err = EAGAIN;
 			goto err_init;
                }
        }
#else
	err = mtd_device_parse_register(mtd, probes, NULL, NULL, 0);
	if (err) {
		pr_err("Failed to register MTD device: %d\n", err);
		goto err_init;
	}
#endif
#else
	#if !defined(CONFIG_RTK_NAND_FLASH_STORAGE)
	#if !defined(CONFIG_MTD_RTKXX_PARTS)
	/* if need ???? */
	if(detect_rootfs_flash_map() < 0){
		printk("dynamic alloc partition fail\n");
		mtd->size = 0;
        err = EAGAIN;
		goto err_init;
	}

	/* check if nand reserve %10 space for skip/remap bbt */
	if(rtkn_check_nand_partition(rtl8196_parts1,ARRAY_SIZE(rtl8196_parts1)) < 0){
		err = -1;
		goto err_init;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	if (add_mtd_partitions (mtd, rtl8196_parts1, ARRAY_SIZE(rtl8196_parts1))) {
        printk("%s: Error, cannot add %s device\n",
                        __FUNCTION__, mtd->name);
        mtd->size = 0;
        err = EAGAIN;
		goto err_init;
	}
#else
	if(mtd_device_parse_register(mtd,NULL,NULL,rtl8196_parts1,ARRAY_SIZE(rtl8196_parts1))){
		pr_err("%s: Error, cannot add %s device\n", 
					__FUNCTION__, mtd->name);
		goto err_init;
	}
#endif
	#else
	// defined CONFIG_MTD_RTKXX_PARTS
	if(mtd_device_parse_register(mtd, probes, NULL, NULL, 0)){
		pr_err("%s: Error, cannot add %s device\n", 
					__FUNCTION__, mtd->name);
		goto err_init;
	}
	#endif
	#else
	// defined(CONFIG_RTK_NAND_FLASH_STORAGE)
	/* check if nand reserve %10 space for skip/remap bbt */
	if(rtkn_check_nand_partition(rtl8196_parts1,ARRAY_SIZE(rtl8196_parts1)) < 0){
		err = -1;
		goto err_init;
	}
	#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	if (add_mtd_partitions (mtd, rtl8196_parts1, ARRAY_SIZE(rtl8196_parts1))) {
        printk("%s: Error, cannot add %s device\n",
                        __FUNCTION__, mtd->name);
        mtd->size = 0;
        err = EAGAIN;
		goto err_init;
	}
	#else
	if(mtd_device_parse_register(mtd,NULL,NULL,rtl8196_parts1,ARRAY_SIZE(rtl8196_parts1))){
		pr_err("%s: Error, cannot add %s device\n", 
					__FUNCTION__, mtd->name);
		goto err_init;
	}
	#endif
	#endif
#endif
#else
	// openwrt use 
	rtk_init_flash_proc();
	detect_nand_flash_map(mtd,rtl8196_parts1);
	/* check if nand reserve %10 space for skip/remap bbt */
	if(rtkn_check_nand_partition(rtl8196_parts1,ARRAY_SIZE(rtl8196_parts1)) < 0){
		err = -1;
		goto err_init;
	}
	if(mtd_device_parse_register(mtd,NULL,NULL,rtl8196_parts1,ARRAY_SIZE(rtl8196_parts1))){
		pr_err("%s: Error, cannot add %s device\n", 
					__FUNCTION__, mtd->name);
		goto err_init;
	}
#endif
#endif

	/* nand info proc */
#ifndef __UBOOT__
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	create_proc_read_entry("nandinfo", 0, NULL, rtk_read_proc_nandinfo, NULL);
#else	
	proc_create("nandinfo", 0, NULL, &rtk_nand_flash_ops);
#endif
#endif
	
	return 0;

err_init:
	kfree(rtkn);
out:
	return err;
}


/* init function */
static struct nand_chip rtkn_nand_chip;
#ifdef __UBOOT__
void  board_nand_init(void)
{
	struct mtd_info *mtd = &nand_info[0];
	struct nand_chip *nand = &rtkn_nand_chip;

	if(rtknflash_lowinit(mtd,nand))
		hang();
}
#else
struct mtd_info rtkn_mtd_info;
int board_nand_init(void)
{
	struct mtd_info *mtd = &rtkn_mtd_info;
	struct nand_chip *nand = &rtkn_nand_chip;

	if(rtknflash_lowinit(mtd,nand))
		return -1;
	return 0;
} 
#endif

