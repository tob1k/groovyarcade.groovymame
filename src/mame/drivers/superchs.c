/****************************************************************************

    Super Chase                         (c) 1992 Taito

    Driver by Bryan McPhail & David Graves.

    Board Info:

        CPU board:
        68000
        68020
        TC0570SPC (Taito custom)
        TC0470LIN (Taito custom)
        TC0510NIO (Taito custom)
        TC0480SCP (Taito custom)
        TC0650FDA (Taito custom)
        ADC0809CCN

        X2=26.686MHz
        X1=40MHz
        X3=32MHz

        Sound board:
        68000
        68681
        MB8421 (x2)
        MB87078
        Ensoniq 5510
        Ensoniq 5505

    (Acknowledgments and thanks to Richard Bush and the Raine team
    for their preliminary Super Chase driver.)

***************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "video/taitoic.h"
#include "machine/eeprom.h"
#include "sound/es5506.h"
#include "audio/taito_en.h"

#include "superchs.lh"
#include "includes/superchs.h"


/*********************************************************************/

READ16_MEMBER(superchs_state::shared_ram_r)
{
	if ((offset&1)==0) return (m_shared_ram[offset/2]&0xffff0000)>>16;
	return (m_shared_ram[offset/2]&0x0000ffff);
}

WRITE16_MEMBER(superchs_state::shared_ram_w)
{
	if ((offset&1)==0) {
		if (ACCESSING_BITS_8_15)
			m_shared_ram[offset/2]=(m_shared_ram[offset/2]&0x00ffffff)|((data&0xff00)<<16);
		if (ACCESSING_BITS_0_7)
			m_shared_ram[offset/2]=(m_shared_ram[offset/2]&0xff00ffff)|((data&0x00ff)<<16);
	} else {
		if (ACCESSING_BITS_8_15)
			m_shared_ram[offset/2]=(m_shared_ram[offset/2]&0xffff00ff)|((data&0xff00)<< 0);
		if (ACCESSING_BITS_0_7)
			m_shared_ram[offset/2]=(m_shared_ram[offset/2]&0xffffff00)|((data&0x00ff)<< 0);
	}
}

WRITE32_MEMBER(superchs_state::cpua_ctrl_w)
{
	/*
    CPUA writes 0x00, 22, 72, f2 in that order.
    f2 seems to be the standard in-game value.
    ..x...x.
    .xxx..x.
    xxxx..x.
    is there an irq enable in the top nibble?
    */

	if (ACCESSING_BITS_8_15)
	{
		cputag_set_input_line(machine(), "sub", INPUT_LINE_RESET, (data &0x200) ? CLEAR_LINE : ASSERT_LINE);
		if (data&0x8000) cputag_set_input_line(machine(), "maincpu", 3, HOLD_LINE); /* Guess */
	}

	if (ACCESSING_BITS_0_7)
	{
		/* Lamp control bits of some sort in the lsb */
	}
}

WRITE32_MEMBER(superchs_state::superchs_palette_w)
{
	int a,r,g,b;
	COMBINE_DATA(&m_generic_paletteram_32[offset]);

	a = m_generic_paletteram_32[offset];
	r = (a &0xff0000) >> 16;
	g = (a &0xff00) >> 8;
	b = (a &0xff);

	palette_set_color(machine(),offset,MAKE_RGB(r,g,b));
}

READ32_MEMBER(superchs_state::superchs_input_r)
{
	switch (offset)
	{
		case 0x00:
			return ioport("INPUTS")->read();

		case 0x01:
			return m_coin_word<<16;
	}

	return 0xffffffff;
}

WRITE32_MEMBER(superchs_state::superchs_input_w)
{

	#if 0
	{
	char t[64];
	COMBINE_DATA(&m_mem[offset]);
	sprintf(t,"%08x %08x",m_mem[0],m_mem[1]);
	//popmessage(t);
	}
	#endif

	switch (offset)
	{
		case 0x00:
		{
			if (ACCESSING_BITS_24_31)	/* $300000 is watchdog */
			{
				machine().watchdog_reset();
			}

			if (ACCESSING_BITS_0_7)
			{
				eeprom_device *eeprom = machine().device<eeprom_device>("eeprom");
				eeprom->set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
				eeprom->write_bit(data & 0x40);
				eeprom->set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
				return;
			}

			return;
		}

		/* there are 'vibration' control bits somewhere! */

		case 0x01:
		{
			if (ACCESSING_BITS_24_31)
			{
				coin_lockout_w(machine(), 0,~data & 0x01000000);
				coin_lockout_w(machine(), 1,~data & 0x02000000);
				coin_counter_w(machine(), 0, data & 0x04000000);
				coin_counter_w(machine(), 1, data & 0x08000000);
				m_coin_word=(data >> 16) &0xffff;
			}
		}
	}
}

READ32_MEMBER(superchs_state::superchs_stick_r)
{
	UINT8 b0 = ioport("UNKNOWN")->read();
	UINT8 b1 = ((ioport("SOUND")->read() * 255) / 100) ^ 0xff; // 00 = full, ff = silent
	UINT8 b2 = ioport("ACCEL")->read() ^ 0xff;
	UINT8 b3 = ioport("WHEEL")->read();

	return b3 << 24 | b2 << 16 | b1 << 8 | b0;
}

WRITE32_MEMBER(superchs_state::superchs_stick_w)
{
	/* This is guess work - the interrupts are in groups of 4, with each writing to a
        different byte in this long word before the RTE.  I assume all but the last
        (top) byte cause an IRQ with the final one being an ACK.  (Total guess but it works). */
	if (mem_mask != 0xff000000)
		cputag_set_input_line(machine(), "maincpu", 3, HOLD_LINE);
}

/***********************************************************
             MEMORY STRUCTURES
***********************************************************/

static ADDRESS_MAP_START( superchs_map, AS_PROGRAM, 32, superchs_state )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x11ffff) AM_RAM AM_SHARE("ram")
	AM_RANGE(0x140000, 0x141fff) AM_RAM AM_SHARE("spriteram")
	AM_RANGE(0x180000, 0x18ffff) AM_DEVREADWRITE_LEGACY("tc0480scp", tc0480scp_long_r, tc0480scp_long_w)
	AM_RANGE(0x1b0000, 0x1b002f) AM_DEVREADWRITE_LEGACY("tc0480scp", tc0480scp_ctrl_long_r, tc0480scp_ctrl_long_w)
	AM_RANGE(0x200000, 0x20ffff) AM_RAM AM_SHARE("shared_ram")
	AM_RANGE(0x240000, 0x240003) AM_WRITE(cpua_ctrl_w)
	AM_RANGE(0x280000, 0x287fff) AM_RAM_WRITE(superchs_palette_w) AM_SHARE("paletteram")
	AM_RANGE(0x2c0000, 0x2c07ff) AM_RAM AM_SHARE("snd_shared")
	AM_RANGE(0x300000, 0x300007) AM_READWRITE(superchs_input_r, superchs_input_w)	/* eerom etc. */
	AM_RANGE(0x340000, 0x340003) AM_READWRITE(superchs_stick_r, superchs_stick_w)	/* stick int request */
ADDRESS_MAP_END

static ADDRESS_MAP_START( superchs_cpub_map, AS_PROGRAM, 16, superchs_state )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x600000, 0x60ffff) AM_DEVWRITE_LEGACY("tc0480scp", tc0480scp_word_w) /* Only written upon errors */
	AM_RANGE(0x800000, 0x80ffff) AM_READWRITE(shared_ram_r, shared_ram_w)
	AM_RANGE(0xa00000, 0xa001ff) AM_RAM	/* Extra road control?? */
ADDRESS_MAP_END

/***********************************************************/

static INPUT_PORTS_START( superchs )
	PORT_START("INPUTS")
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_SPECIAL )	PORT_READ_LINE_DEVICE_MEMBER("eeprom", eeprom_device, read_bit)	/* reserved for EEROM */
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW,  IPT_SERVICE2 ) PORT_NAME("Seat Center")	/* seat center (cockpit only) */
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_NAME("Nitro")
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW,  IPT_BUTTON4 ) PORT_NAME("Shifter") PORT_TOGGLE
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_NAME("Brake Switch")	/* upright doesn't have brake? */
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_BIT( 0x00010000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_DIPNAME( 0x00080000, 0x00, "Freeze Screen" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00080000, DEF_STR( On ) )
	PORT_SERVICE_NO_TOGGLE( 0x00100000, IP_ACTIVE_LOW )
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x01000000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04000000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08000000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20000000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40000000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80000000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	// 4 analog ports
	PORT_START("WHEEL")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE PORT_NAME("Steering Wheel")

	PORT_START("ACCEL")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_SENSITIVITY(100) PORT_KEYDELTA(15) PORT_NAME("Gas Pedal")	/* in upright cab, it is a digital (1 bit) switch instead */

	PORT_START("SOUND")
	PORT_ADJUSTER( 75, "PCB - Sound Volume" )

	PORT_START("UNKNOWN") // unused?
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/***********************************************************
                GFX DECODING
**********************************************************/

static const gfx_layout tile16x16_layout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64,  2*64,  3*64,  4*64,  5*64,  6*64,  7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	64*16	/* every sprite takes 128 consecutive bytes */
};

static const gfx_layout charlayout =
{
	16,16,    /* 16*16 characters */
	RGN_FRAC(1,1),
	4,        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 5*4, 4*4, 3*4, 2*4, 7*4, 6*4, 9*4, 8*4, 13*4, 12*4, 11*4, 10*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8     /* every sprite takes 128 consecutive bytes */
};

static GFXDECODE_START( superchs )
	GFXDECODE_ENTRY( "gfx2", 0x0, tile16x16_layout,  0, 512 )
	GFXDECODE_ENTRY( "gfx1", 0x0, charlayout,        0, 512 )
GFXDECODE_END


/***********************************************************
                 MACHINE DRIVERS
***********************************************************/

static const eeprom_interface superchs_eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/* read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/* unlock command */
	"0100110000",	/* lock command */
};

static const tc0480scp_interface superchs_tc0480scp_intf =
{
	1, 2,		/* gfxnum, txnum */
	0,			/* pixels */
	0x20, 0x08,	/* x_offset, y_offset */
	-1, 0,		/* text_xoff, text_yoff */
	0, 0,		/* flip_xoff, flip_yoff */
	0			/* col_base */
};

static MACHINE_CONFIG_START( superchs, superchs_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68EC020, XTAL_32MHz/2)
	MCFG_CPU_PROGRAM_MAP(superchs_map)
	MCFG_CPU_VBLANK_INT("screen", irq2_line_hold)

	MCFG_CPU_ADD("sub", M68000, XTAL_32MHz/2)
	MCFG_CPU_PROGRAM_MAP(superchs_cpub_map)
	MCFG_CPU_VBLANK_INT("screen", irq4_line_hold)

	MCFG_QUANTUM_TIME(attotime::from_hz(480)) /* Need to interleave CPU 1 & 3 */

	MCFG_EEPROM_ADD("eeprom", superchs_eeprom_interface)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(40*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 40*8-1, 2*8, 32*8-1)
	MCFG_SCREEN_UPDATE_STATIC(superchs)

	MCFG_GFXDECODE(superchs)
	MCFG_PALETTE_LENGTH(8192)

	MCFG_VIDEO_START(superchs)

	MCFG_TC0480SCP_ADD("tc0480scp", superchs_tc0480scp_intf)

	/* sound hardware */
	MCFG_FRAGMENT_ADD(taito_en_sound)
MACHINE_CONFIG_END

/***************************************************************************/

ROM_START( superchs )
	ROM_REGION( 0x100000, "maincpu", 0 )	/* 1024K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "d46-35.ic27", 0x00000, 0x40000, CRC(1575c9a7) SHA1(e3441d6018ed3315c62c5e5c4534d8712b025ae2) )
	ROM_LOAD32_BYTE( "d46-34.ic25", 0x00001, 0x40000, CRC(c72a4d2b) SHA1(6ef64de15e52007406ce3255071a1f856e0e8b49) )
	ROM_LOAD32_BYTE( "d46-33.ic23", 0x00002, 0x40000, CRC(3094bcd0) SHA1(b6779b81a3ebec440a9359868dc43fc3a631ee11) )
	ROM_LOAD32_BYTE( "d46-31.ic21", 0x00003, 0x40000, CRC(38b983a3) SHA1(c4859cecc2f3506b7090c462cecd3e4eaabe85aa) )

	ROM_REGION( 0x140000, "audiocpu", 0 )	/* Sound cpu */
	ROM_LOAD16_BYTE( "d46-37.ic8", 0x100000, 0x20000, CRC(60b51b91) SHA1(0d0b017808e0a3bdabe8ef5a726bbe16428db06b) )
	ROM_LOAD16_BYTE( "d46-36.ic7", 0x100001, 0x20000, CRC(8f7aa276) SHA1(b3e330e33099d3cbf4cdc43063119b041e9eea3a) )

	ROM_REGION( 0x40000, "sub", 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "d46-24.ic127", 0x00000, 0x20000, CRC(a006baa1) SHA1(e691ddab6cb79444bd6c3fc870e0dff3051d8cf9) )
	ROM_LOAD16_BYTE( "d46-23.ic112", 0x00001, 0x20000, CRC(9a69dbd0) SHA1(13eca492f1db834c599656750864e7003514f3d4) )

	ROM_REGION( 0x200000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "d46-05.ic87", 0x00000, 0x100000, CRC(150d0e4c) SHA1(9240b32900be733b8f44868ed5d64f5f1aaadb47) )	/* SCR 16x16 tiles */
	ROM_LOAD16_BYTE( "d46-06.ic88", 0x00001, 0x100000, CRC(321308be) SHA1(17e724cce39b1331650c1f08d693d057dcd43a3f) )

	ROM_REGION( 0x800000, "gfx2", 0 )
	ROM_LOAD32_BYTE( "d46-01.ic64", 0x000003, 0x200000, CRC(5c2ae92d) SHA1(bee2caed4729a27fa0569d952d6d12170c2aa2a8) )	/* OBJ 16x16 tiles: each rom has 1 bitplane */
	ROM_LOAD32_BYTE( "d46-02.ic65", 0x000002, 0x200000, CRC(a83ca82e) SHA1(03759be87a8d62c0044e8a44e90c47308e32d3e5) )
	ROM_LOAD32_BYTE( "d46-03.ic66", 0x000001, 0x200000, CRC(e0e9cbfd) SHA1(b7deb2c58320af9d1b4273ad2758ce927d2e279c) )
	ROM_LOAD32_BYTE( "d46-04.ic67", 0x000000, 0x200000, CRC(832769a9) SHA1(136ead19edeee90b5be91a6e2f434193dc670fd8) )

	ROM_REGION16_LE( 0x80000, "user1", 0 )
	ROM_LOAD16_WORD( "d46-07.ic34", 0x00000, 0x80000, CRC(c3b8b093) SHA1(f34364248ca7fdaaa1a0f8f6f795f9b4bc935fb9) )	/* STY, used to create big sprites on the fly */

	ROM_REGION16_BE( 0x1000000, "ensoniq.0" , ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "d46-10.ic2", 0xc00000, 0x200000, CRC(306256be) SHA1(e6e5d4a4c0b98470f2aff2e94624dd19af73ec5d) )
	ROM_LOAD16_BYTE( "d46-12.ic4", 0x000000, 0x200000, CRC(a24a53a8) SHA1(5d5fb87a94ceabda89360064d7d9b6d23c4c606b) )
	ROM_RELOAD     (               0x400000, 0x200000 )
	ROM_LOAD16_BYTE( "d46-11.ic5", 0x800000, 0x200000, CRC(d4ea0f56) SHA1(dc8d2ed3c11d0b6f9ebdfde805188884320235e6) )

	ROM_REGION16_BE( 0x80, "eeprom", 0 )
	ROM_LOAD( "eeprom-superchs.bin", 0x0000, 0x0080, CRC(230f0753) SHA1(4c692b35083da71ed866b233c7c9b152a914c95c) )
ROM_END

ROM_START( superchsj )
	ROM_REGION( 0x100000, "maincpu", 0 )	/* 1024K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "d46-28+.ic27", 0x00000, 0x40000, CRC(5c33784f) SHA1(cb3b3bae4fe8f83809c1f437635b3efc1fb4206a) ) /* Actually labeled D46 28* */
	ROM_LOAD32_BYTE( "d46-27+.ic25", 0x00001, 0x40000, CRC(e81125b8) SHA1(a5c9731eb255217861cda0dfad1ee5003f087b81) ) /* Actually labeled D46 27* */
	ROM_LOAD32_BYTE( "d46-26+.ic23", 0x00002, 0x40000, CRC(2aaba1b0) SHA1(13ceaa678bd671c5c88cac35e8a021a180728a69) ) /* Actually labeled D46 26* */
	ROM_LOAD32_BYTE( "d46-25+.ic21", 0x00003, 0x40000, CRC(4241e97a) SHA1(e3e361080e3ebc098805310d41b3afe7f14ff8b4) ) /* Actually labeled D46 25* */

	ROM_REGION( 0x140000, "audiocpu", 0 )	/* Sound cpu */
	ROM_LOAD16_BYTE( "d46-30.ic8", 0x100000, 0x20000, CRC(88f8a421) SHA1(4fd0885d398b1b0e127d7462926d1630a635e305) )
	ROM_LOAD16_BYTE( "d46-29.ic7", 0x100001, 0x20000, CRC(04501fa5) SHA1(dfbafc34df8ab0fcaefb5ca4c3143977020b7e58) )

	ROM_REGION( 0x40000, "sub", 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "d46-24.ic127", 0x00000, 0x20000, CRC(a006baa1) SHA1(e691ddab6cb79444bd6c3fc870e0dff3051d8cf9) )
	ROM_LOAD16_BYTE( "d46-23.ic112", 0x00001, 0x20000, CRC(9a69dbd0) SHA1(13eca492f1db834c599656750864e7003514f3d4) )

	ROM_REGION( 0x200000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "d46-05.ic87", 0x00000, 0x100000, CRC(150d0e4c) SHA1(9240b32900be733b8f44868ed5d64f5f1aaadb47) )	/* SCR 16x16 tiles */
	ROM_LOAD16_BYTE( "d46-06.ic88", 0x00001, 0x100000, CRC(321308be) SHA1(17e724cce39b1331650c1f08d693d057dcd43a3f) )

	ROM_REGION( 0x800000, "gfx2", 0 )
	ROM_LOAD32_BYTE( "d46-01.ic64", 0x000003, 0x200000, CRC(5c2ae92d) SHA1(bee2caed4729a27fa0569d952d6d12170c2aa2a8) )	/* OBJ 16x16 tiles: each rom has 1 bitplane */
	ROM_LOAD32_BYTE( "d46-02.ic65", 0x000002, 0x200000, CRC(a83ca82e) SHA1(03759be87a8d62c0044e8a44e90c47308e32d3e5) )
	ROM_LOAD32_BYTE( "d46-03.ic66", 0x000001, 0x200000, CRC(e0e9cbfd) SHA1(b7deb2c58320af9d1b4273ad2758ce927d2e279c) )
	ROM_LOAD32_BYTE( "d46-04.ic67", 0x000000, 0x200000, CRC(832769a9) SHA1(136ead19edeee90b5be91a6e2f434193dc670fd8) )

	ROM_REGION16_LE( 0x80000, "user1", 0 )
	ROM_LOAD16_WORD( "d46-07.ic34", 0x00000, 0x80000, CRC(c3b8b093) SHA1(f34364248ca7fdaaa1a0f8f6f795f9b4bc935fb9) )	/* STY, used to create big sprites on the fly */

	ROM_REGION16_BE( 0x1000000, "ensoniq.0" , ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "d46-10.ic2", 0xc00000, 0x200000, CRC(306256be) SHA1(e6e5d4a4c0b98470f2aff2e94624dd19af73ec5d) )
	ROM_LOAD16_BYTE( "d46-09.ic4", 0x000000, 0x200000, CRC(0acb8bc7) SHA1(62d66925f0eee4cee282c4e0972e08d12acf331c) )
	ROM_RELOAD     (               0x400000, 0x200000 )
	ROM_LOAD16_BYTE( "d46-08.ic5", 0x800000, 0x200000, CRC(4677e820) SHA1(d6427844b08438e45af4c671589a270e46e6dead) )

	ROM_REGION16_BE( 0x80, "eeprom", 0 )
	ROM_LOAD( "eeprom-superchs.bin", 0x0000, 0x0080, CRC(230f0753) SHA1(4c692b35083da71ed866b233c7c9b152a914c95c) )
ROM_END

READ32_MEMBER(superchs_state::main_cycle_r)
{
	if (cpu_get_pc(&space.device())==0x702)
		device_spin_until_interrupt(&space.device());

	return m_ram[0];
}

READ16_MEMBER(superchs_state::sub_cycle_r)
{
	if (cpu_get_pc(&space.device())==0x454)
		device_spin_until_interrupt(&space.device());

	return m_ram[2]&0xffff;
}

static DRIVER_INIT( superchs )
{
	/* Speedup handlers */
	superchs_state *state = machine.driver_data<superchs_state>();
	machine.device("maincpu")->memory().space(AS_PROGRAM)->install_read_handler(0x100000, 0x100003, read32_delegate(FUNC(superchs_state::main_cycle_r),state));
	machine.device("sub")->memory().space(AS_PROGRAM)->install_read_handler(0x80000a, 0x80000b, read16_delegate(FUNC(superchs_state::sub_cycle_r),state));
}

GAMEL( 1992, superchs,         0, superchs, superchs, superchs, ROT0, "Taito America Corporation", "Super Chase - Criminal Termination (US)", 0, layout_superchs )
GAMEL( 1992, superchsj, superchs, superchs, superchs, superchs, ROT0, "Taito Corporation", "Super Chase - Criminal Termination (Japan)", 0, layout_superchs )
