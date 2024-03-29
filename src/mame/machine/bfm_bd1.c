/**********************************************************************

    Bellfruit BD1 VFD module interface and emulation by J.Wallace

    TODO: Implement flashing (our only datasheet has that section
    completely illegible)
**********************************************************************/

#include "emu.h"
#include "bfm_bd1.h"

const device_type BFM_BD1 = &device_creator<bfm_bd1_t>;


/*
   BD1 14 segment charset lookup table, according to datasheet (we rewire this later)

        2
    ---------
   |\   |3  /|
 0 | \6 |  /7| 1
   |  \ | /  |
    -F-- --E-
   |  / | \  |
 D | /4 |A \B| 5
   |/   |   \|
    ---------  C
        9

        8 is flashing

  */

static const UINT16 BD1charset[]=
{           // FEDC BA98 7654 3210
	0xA626, // 1010 0110 0010 0110 @.
	0xE027, // 1110 0000 0010 0111 A.
	0x462E, // 0100 0110 0010 1110 B.
	0x2205, // 0010 0010 0000 0101 C.
	0x062E, // 0000 0110 0010 1110 D.
	0xA205, // 1010 0010 0000 0101 E.
	0xA005, // 1010 0000 0000 0101 F.
	0x6225, // 0110 0010 0010 0101 G.
	0xE023, // 1110 0000 0010 0011 H.
	0x060C, // 0000 0110 0000 1100 I.
	0x2222, // 0010 0010 0010 0010 J.
	0xA881, // 1010 1000 1000 0001 K.
	0x2201, // 0010 0010 0000 0001 L.
	0x20E3, // 0010 0000 1110 0011 M.
	0x2863, // 0010 1000 0110 0011 N.
	0x2227, // 0010 0010 0010 0111 O.
	0xE007, // 1110 0000 0000 0111 P.
	0x2A27, // 0010 1010 0010 0111 Q.
	0xE807, // 1110 1000 0000 0111 R.
	0xC225, // 1100 0010 0010 0101 S.
	0x040C, // 0000 0100 0000 1100 T.
	0x2223, // 0010 0010 0010 0011 U.
	0x2091, // 0010 0000 1001 0001 V.
	0x2833, // 0010 1000 0011 0011 W.
	0x08D0, // 0000 1000 1101 0000 X.
	0x04C0, // 0000 0100 1100 0000 Y.
	0x0294, // 0000 0010 1001 0100 Z.
	0x2205, // 0010 0010 0000 0101 [.
	0x0840, // 0000 1000 0100 0000 \.
	0x0226, // 0000 0010 0010 0110 ].
	0x0810, // 0000 1000 0001 0000 ^.
	0x0200, // 0000 0010 0000 0000 _
	0x0000, // 0000 0000 0000 0000
	0xC290, // 1100 0010 1001 0000 POUND.
	0x0009, // 0000 0000 0000 1001 ".
	0xC62A, // 1100 0110 0010 1010 #.
	0xC62D, // 1100 0110 0010 1101 $.
	0x0100, // 0000 0001 0000 0000 flash character
	0x0000, // 0000 0000 0000 0000 not defined
	0x0040, // 0000 0000 1000 0000 '.
	0x0880, // 0000 1000 1000 0000 (.
	0x0050, // 0000 0000 0101 0000 ).
	0xCCD8, // 1100 1100 1101 1000 *.
	0xC408, // 1100 0100 0000 1000 +.
	0x1000, // 0001 0000 0000 0000 .
	0xC000, // 1100 0000 0000 0000 -.
	0x1000, // 0001 0000 0000 0000 ..
	0x0090, // 0000 0000 1001 0000 /.
	0x22B7, // 0010 0010 1011 0111 0.
	0x0408, // 0000 0100 0000 1000 1.
	0xE206, // 1110 0010 0000 0110 2.
	0x4226, // 0100 0010 0010 0110 3.
	0xC023, // 1100 0000 0010 0011 4.
	0xC225, // 1100 0010 0010 0101 5.
	0xE225, // 1110 0010 0010 0101 6.
	0x0026, // 0000 0000 0010 0110 7.
	0xE227, // 1110 0010 0010 0111 8.
	0xC227, // 1100 0010 0010 0111 9.
	0xFFFF, // 0000 0000 0000 0000 user defined, can be replaced by main program
	0x0000, // 0000 0000 0000 0000 dummy
	0x0290, // 0000 0010 1001 0000 <.
	0xC200, // 1100 0010 0000 0000 =.
	0x0A40, // 0000 1010 0100 0000 >.
	0x4406, // 0100 0100 0000 0110 ?
};

bfm_bd1_t::bfm_bd1_t(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, BFM_BD1, "BFM BD1 VFD controller", tag, owner, clock),
	m_port_val(0)
{
}

void bfm_bd1_t::static_set_value(device_t &device, int val)
{
	bfm_bd1_t &bd1 = downcast<bfm_bd1_t &>(device);
	bd1.m_port_val = val;
}

void bfm_bd1_t::device_start()
{
	m_timer=timer_alloc(0);

    save_item(NAME(m_cursor));
    save_item(NAME(m_cursor_pos));
	save_item(NAME(m_window_start));		// display window start pos 0-15
	save_item(NAME(m_window_end));		// display window end   pos 0-15
	save_item(NAME(m_window_size));		// window  size
	save_item(NAME(m_shift_count));
	save_item(NAME(m_shift_data));
	save_item(NAME(m_pcursor_pos));
	save_item(NAME(m_blank_flag));
	save_item(NAME(m_flash_flag));
	save_item(NAME(m_scroll_active));
	save_item(NAME(m_display_mode));
	save_item(NAME(m_flash_rate));
	save_item(NAME(m_flash_control));
    save_item(NAME(m_chars));
    save_item(NAME(m_attrs));
	save_item(NAME(m_user_data));			// user defined character data (16 bit)
	save_item(NAME(m_user_def));			// user defined character state

	device_reset();
}

void bfm_bd1_t::device_reset()
{
	m_cursor = 0;
    m_cursor_pos = 0;
	m_window_start = 0;
	m_window_end = 0;
	m_window_size = 0;
	m_shift_count = 0;
	m_shift_data = 0;
	m_pcursor_pos = 0;
	m_blank_flag = 0;
	m_flash_flag = 0;
	m_scroll_active = 0;
	m_display_mode = 0;
	m_flash_rate = 0;
	m_flash_control = 0;
	m_user_data = 0;
	m_user_def = 0;

    memset(m_chars, 0, sizeof(m_chars));
    memset(m_attrs, 0, sizeof(m_attrs));
	m_timer->adjust(attotime::from_hz(clock()), 0);
}

void bfm_bd1_t::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	update_display();
	m_timer->adjust(attotime::from_hz(clock()), 0);
}

UINT32 bfm_bd1_t::set_display(UINT16 segin)
{
	UINT32 segout=0;
	if ( segin & 0x0004 )	segout |=  0x0001;
	else    	            segout &= ~0x0001;
	if ( segin & 0x0002 )	segout |=  0x0002;
	else        	        segout &= ~0x0002;
	if ( segin & 0x0020 )	segout |=  0x0004;
	else            	    segout &= ~0x0004;
	if ( segin & 0x0200 )	segout |=  0x0008;
	else                	segout &= ~0x0008;
	if ( segin & 0x2000 )	segout |=  0x0010;
	else                    segout &= ~0x0010;
	if ( segin & 0x0001 )	segout |=  0x0020;
	else                    segout &= ~0x0020;
	if ( segin & 0x8000 )	segout |=  0x0040;
	else                    segout &= ~0x0040;
	if ( segin & 0x4000 )	segout |=  0x0080;
	else                	segout &= ~0x0080;
	if ( segin & 0x0008 )	segout |=  0x0100;
	else        		    segout &= ~0x0100;
	if ( segin & 0x0400 )	segout |=  0x0200;
	else                    segout &= ~0x0200;
	if ( segin & 0x0010 )	segout |=  0x0400;
	else                    segout &= ~0x0400;
	if ( segin & 0x0040 )	segout |=  0x0800;
	else                    segout &= ~0x0800;
	if ( segin & 0x0080 )	segout |=  0x1000;
	else                    segout &= ~0x1000;
	if ( segin & 0x0800 )	segout |=  0x2000;
	else                    segout &= ~0x2000;
	if ( segin & 0x1000 )	segout |=  0x4000;
	else                    segout &= ~0x4000;

	return segout;
}

void bfm_bd1_t::device_post_load()
{
	for (int i =0; i<16; i++)
	{
		output_set_indexed_value("vfd", (m_port_val*16) + i, m_outputs[i]);
	}
}

void bfm_bd1_t::update_display()
{
	m_outputs[m_cursor] = set_display(m_chars[m_cursor]);;
	output_set_indexed_value("vfd", (m_port_val*16) + m_cursor, m_outputs[m_cursor]);

	m_cursor++;
	if (m_cursor >15)
	{
		m_cursor=0;
	}
}
///////////////////////////////////////////////////////////////////////////
void bfm_bd1_t::blank(int data)
{
	switch ( data & 0x04 )
	{
		case 0x00:	// blank all
		{
			memset(m_chars, 0, sizeof(m_chars));
			memset(m_attrs, 0, sizeof(m_attrs));
		}
		break;
		case 0x01:	// blank inside window
		if ( m_window_size > 0 )
		{
			memset(m_chars+m_window_start,0,m_window_size);
			memset(m_attrs+m_window_start,0,m_window_size);
		}
		break;
		case 0x02:	// blank outside window
		if ( m_window_size > 0 )
		{
			if ( m_window_start > 0 )
			{
				for (int i = 0; i < m_window_start; i++)
				{
					memset(m_chars+i,0,i);
					memset(m_attrs+i,0,i);
				}
			}

			if (m_window_end < 15 )
			{
				for (int i = m_window_end; i < 15- m_window_end ; i++)
				{
					memset(m_chars+i,0,i);
					memset(m_attrs+i,0,i);
				}
			}
		}
		break;

		case 0x03:	// blank entire display
		{
			memset(m_chars, 0, sizeof(m_chars));
			memset(m_attrs, 0, sizeof(m_attrs));
		}
		break;
	}
}
int bfm_bd1_t::write_char(int data)
{
	int change = 0;
	if ( m_user_def )
	{
		m_user_def--;

		m_user_data <<= 8;
		m_user_data |= data;

		if ( m_user_def )
		{
			return 0;
		}

		setdata( m_user_data, data);
		change ++;
	}
	else
	{

		if(data < 0x80)//characters
		{

			if (m_blank_flag || m_flash_flag)
			{
				if (m_blank_flag)
				{
					//m_display_blanking = data & 0x0F;
					blank( data & 0x04 );
					m_blank_flag = 0;
				}
				if (m_flash_flag)
				{
					//not setting yet
					m_flash_flag = 0;
				}
			}
			else
			{
				if (data > 0x3F)
				{
				//  logerror("Undefined character %x \n", data);
				}

				setdata(BD1charset[(data & 0x3F)], data);
			}
		}
		else
		{
			switch ( data & 0xF0 )
			{
				case 0x80:	// 0x80 - 0x8F Set display blanking
				if (data ==0x84)// futaba setup
				{
					m_blank_flag = 1;
				}
				else
				{
					blank(data&0x04);//use the blanking data
				}
				break;

				case 0x90:	// 0x90 - 0x9F Set cursor pos
				m_cursor_pos = data & 0x0F;
				m_scroll_active = 0;
				if ( m_display_mode == 2 )
				{
					if ( m_cursor_pos >= m_window_end) m_scroll_active = 1;
				}
				break;

				case 0xA0:	// 0xA0 - 0xAF Set display mode
				m_display_mode = data &0x03;
				break;

				case 0xB0:	// 0xB0 - 0xBF Clear display area
				switch ( data & 0x03 )
				{
					case 0x00:	// clr nothing
					break;

					case 0x01:	// clr inside window
					if ( m_window_size > 0 )
					{
						memset(m_chars+m_window_start,0,m_window_size);
						memset(m_attrs+m_window_start,0,m_window_size);
					}

					break;

					case 0x02:	// clr outside window
					if ( m_window_size > 0 )
					{
						if ( m_window_start > 0 )
						{
							for (int i = 0; i < m_window_start; i++)
							{
								memset(m_chars+i,0,i);
								memset(m_attrs+i,0,i);
							}
						}

						if (m_window_end < 15 )
						{
							for (int i = m_window_end; i < 15- m_window_end ; i++)
							{
								memset(m_chars+i,0,i);
								memset(m_attrs+i,0,i);
							}
						}
					}
					case 0x03:	// clr entire display
					{
						memset(m_chars, 0, sizeof(m_chars));
						memset(m_attrs, 0, sizeof(m_attrs));
					}
				}
				break;

				case 0xC0:	// 0xC0 - 0xCF Set flash rate
				m_flash_rate = data & 0x0F;
				break;

				case 0xD0:	// 0xD0 - 0xDF Set Flash control
				m_flash_control = data & 0x03;
				break;

				case 0xE0:	// 0xE0 - 0xEF Set window start pos
				m_window_start = data &0x0F;
				m_window_size  = (m_window_end - m_window_start)+1;
				break;

				case 0xF0:	// 0xF0 - 0xFF Set window end pos
				m_window_end   = data &0x0F;
				m_window_size  = (m_window_end - m_window_start)+1;
				m_scroll_active = 0;
				if ( m_display_mode == 2 )
				{
					if ( m_cursor_pos >= m_window_end)
					{
						m_scroll_active = 1;
						m_cursor_pos    = m_window_end;
					}
				}
				break;
			}
		}
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////

void bfm_bd1_t::setdata(int segdata, int data)
{
	int move = 0;
	int change =0;
	switch ( data )
	{
		case 0x25:	// flash
		if(m_chars[m_pcursor_pos] & (1<<8))
		{
			move++;
		}
		else
		{
			m_chars[m_pcursor_pos] |= (1<<8);
		}
		break;

		case 0x26:  // undefined
		break;

		case 0x2C:  // semicolon
		case 0x2E:  // decimal point

		if( m_chars[m_pcursor_pos] & (1<<12))
		{
			move++;
		}
		else
		{
			m_chars[m_pcursor_pos] |= (1<<12);
		}
		break;

		case 0x3B:	// dummy char
		move++;
		break;

		case 0x3A:
		m_user_def = 2;
		break;

		default:
		move++;
		change++;
	}

	if ( move )
	{

		int mode = m_display_mode;

		m_pcursor_pos = m_cursor_pos;

		if ( m_window_size <= 0 || (m_window_size > 16))
		{ // if no window selected default to equivalent rotate mode
				if ( mode == 2 )      mode = 0;
				else if ( mode == 3 ) mode = 1;
		}

		switch ( mode )
		{
		case 0: // rotate left

		m_cursor_pos &= 0x0F;

		if ( change )
		{
			m_chars[m_cursor_pos] = segdata;
		}
		m_cursor_pos++;
		if ( m_cursor_pos >= 16 ) m_cursor_pos = 0;
		break;


		case 1: // Rotate right

		m_cursor_pos &= 0x0F;

		if ( change )
		{
			m_chars[m_cursor_pos] = segdata;
		}
		m_cursor_pos--;
		if ( m_cursor_pos < 0  ) m_cursor_pos = 15;
		break;

		case 2: // Scroll left

		if ( m_cursor_pos < m_window_end )
		{
			m_scroll_active = 0;
			if ( change )
			{
				m_chars[m_cursor_pos] = segdata;
			}
			m_cursor_pos++;
		}
		else
		{
			if ( move )
			{
				if  ( m_scroll_active )
				{
					int i = m_window_start;
					while ( i < m_window_end )
					{
						m_chars[i] = m_chars[i+1];
						i++;
					}
				}
				else   m_scroll_active = 1;
			}

			if ( change )
			{
				m_chars[m_window_end] = segdata;
			}
			else
			{
				m_chars[m_window_end] = 0;
			}
		}
		break;

		case 3: // Scroll right

			if ( m_cursor_pos > m_window_start )
			{
				if ( change )
				{
					m_chars[m_cursor_pos] = segdata;
				}
				m_cursor_pos--;
				if ( m_cursor_pos > 15  ) m_cursor_pos = 0;
			}
			else
			{
				if ( move )
				{
					if  ( m_scroll_active )
					{
						int i = m_window_end;

						while ( i > m_window_start )
						{
							m_chars[i] = m_chars[i-1];
							i--;
						}
					}
					else   m_scroll_active = 1;
				}
				if ( change )
				{
					 m_chars[m_window_start] = segdata;
				 }
				else
				{
					 m_chars[m_window_start] = 0;
				}
			}
			break;
		}
	}
}

void bfm_bd1_t::shift_data(int data)
{
	m_shift_data <<= 1;

	if ( !data ) m_shift_data |= 1;

	if ( ++m_shift_count >= 8 )
	{
		write_char(m_shift_data);
		m_shift_count = 0;
		m_shift_data  = 0;
	}
}
