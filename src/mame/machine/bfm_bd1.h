#pragma once
#ifndef BFM_BD1_H
#define BFM_BD1_H

//Based on the datasheet, the makimum oscillation rate for one character is 31.25 KHz, so we set our character clock to that.
#define MCFG_BFMBD1_ADD(_tag,_val) \
		MCFG_DEVICE_ADD(_tag, BFM_BD1,312500)\
		MCFG_BD1_PORT(_val) \

#define MCFG_BD1_PORT(_val) \
	bfm_bd1_t::static_set_value(*device, _val); \

#define MCFG_BFMBD1_REMOVE(_tag) \
    MCFG_DEVICE_REMOVE(_tag)

class bfm_bd1_t : public device_t
{
public:
	typedef delegate<void (bool state)> line_cb;
	bfm_bd1_t(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);


	// inline configuration helpers
	static void static_set_value(device_t &device, int val);

    int write_char(int data);
    virtual void update_display();
	UINT8	m_port_val;
	void blank(int data);
	void shift_data(int data);
	void setdata(int segdata, int data);
    UINT32 set_display(UINT16 segin);

protected:
    static const UINT8 AT_NORMAL  = 0x00;
    static const UINT8 AT_FLASH   = 0x01;
    static const UINT8 AT_FLASHED = 0x80;   // set when character should be blinked off

	emu_timer *m_timer;

	int m_cursor_pos;
	int	m_window_start;		// display window start pos 0-15
	int	m_window_end;		// display window end   pos 0-15
	int	m_window_size;		// window  size
	int	m_shift_count;
	int	m_shift_data;
	int m_pcursor_pos;
	int m_blank_flag;
	int m_flash_flag;
	int m_scroll_active;
	int m_display_mode;
	int m_flash_rate;
	int m_flash_control;
    UINT8 m_cursor;
    UINT16 m_chars[16];
    UINT16 m_outputs[16];
    UINT8 m_attrs[16];
	UINT16 m_user_data; 			// user defined character data (16 bit)
	UINT16 m_user_def;			// user defined character state

	virtual void device_start();
	virtual void device_reset();
	virtual void device_post_load();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

};

extern const device_type BFM_BD1;
#endif

