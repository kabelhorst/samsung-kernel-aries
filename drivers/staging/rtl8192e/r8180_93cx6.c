/*
   This files contains card eeprom (93c46 or 93c56) programming routines,
   memory is addressed by 16 bits words.

   This is part of rtl8180 OpenSource driver.
   Copyright (C) Andrea Merello 2004  <andreamrl@tiscali.it>
   Released under the terms of GPL (General Public Licence)

   Parts of this driver are based on the GPL part of the
   official realtek driver.

   Parts of this driver are based on the rtl8180 driver skeleton
   from Patric Schenke & Andres Salomon.

   Parts of this driver are based on the Intel Pro Wireless 2100 GPL driver.

   We want to tanks the Authors of those projects and the Ndiswrapper
   project Authors.
*/

#include "r8180_93cx6.h"

static void eprom_cs(struct r8192_priv *priv, short bit)
{
	if (bit)
		write_nic_byte(priv, EPROM_CMD,
			       (1<<EPROM_CS_SHIFT) |
			       read_nic_byte(priv, EPROM_CMD)); //enable EPROM
	else
		write_nic_byte(priv, EPROM_CMD, read_nic_byte(priv, EPROM_CMD)
			       &~(1<<EPROM_CS_SHIFT)); //disable EPROM

	udelay(EPROM_DELAY);
}


static void eprom_ck_cycle(struct r8192_priv *priv)
{
	write_nic_byte(priv, EPROM_CMD,
		       (1<<EPROM_CK_SHIFT) | read_nic_byte(priv, EPROM_CMD));
	udelay(EPROM_DELAY);
	write_nic_byte(priv, EPROM_CMD,
		       read_nic_byte(priv, EPROM_CMD) & ~(1<<EPROM_CK_SHIFT));
	udelay(EPROM_DELAY);
}


static void eprom_w(struct r8192_priv *priv, short bit)
{
	if (bit)
		write_nic_byte(priv, EPROM_CMD, (1<<EPROM_W_SHIFT) |
			       read_nic_byte(priv, EPROM_CMD));
	else
		write_nic_byte(priv, EPROM_CMD, read_nic_byte(priv, EPROM_CMD)
			       &~(1<<EPROM_W_SHIFT));

	udelay(EPROM_DELAY);
}


static short eprom_r(struct r8192_priv *priv)
{
	short bit;

	bit = (read_nic_byte(priv, EPROM_CMD) & (1<<EPROM_R_SHIFT));
	udelay(EPROM_DELAY);

	if (bit)
		return 1;
	return 0;
}


static void eprom_send_bits_string(struct r8192_priv *priv, short b[], int len)
{
	int i;

	for (i = 0; i < len; i++) {
		eprom_w(priv, b[i]);
		eprom_ck_cycle(priv);
	}
}


u32 eprom_read(struct r8192_priv *priv, u32 addr)
{
	short read_cmd[] = {1, 1, 0};
	short addr_str[8];
	int i;
	int addr_len;
	u32 ret;

	ret = 0;
        //enable EPROM programming
	write_nic_byte(priv, EPROM_CMD,
		       (EPROM_CMD_PROGRAM<<EPROM_CMD_OPERATING_MODE_SHIFT));
	udelay(EPROM_DELAY);

	if (priv->epromtype == EPROM_93c56) {
		addr_str[7] = addr & 1;
		addr_str[6] = addr & (1<<1);
		addr_str[5] = addr & (1<<2);
		addr_str[4] = addr & (1<<3);
		addr_str[3] = addr & (1<<4);
		addr_str[2] = addr & (1<<5);
		addr_str[1] = addr & (1<<6);
		addr_str[0] = addr & (1<<7);
		addr_len = 8;
	} else {
		addr_str[5] = addr & 1;
		addr_str[4] = addr & (1<<1);
		addr_str[3] = addr & (1<<2);
		addr_str[2] = addr & (1<<3);
		addr_str[1] = addr & (1<<4);
		addr_str[0] = addr & (1<<5);
		addr_len = 6;
	}
	eprom_cs(priv, 1);
	eprom_ck_cycle(priv);
	eprom_send_bits_string(priv, read_cmd, 3);
	eprom_send_bits_string(priv, addr_str, addr_len);

	//keep chip pin D to low state while reading.
	//I'm unsure if it is necessary, but anyway shouldn't hurt
	eprom_w(priv, 0);

	for (i = 0; i < 16; i++) {
		//eeprom needs a clk cycle between writing opcode&adr
		//and reading data. (eeprom outs a dummy 0)
		eprom_ck_cycle(priv);
		ret |= (eprom_r(priv)<<(15-i));
	}

	eprom_cs(priv, 0);
	eprom_ck_cycle(priv);

	//disable EPROM programming
	write_nic_byte(priv, EPROM_CMD,
		       (EPROM_CMD_NORMAL<<EPROM_CMD_OPERATING_MODE_SHIFT));
	return ret;
}
