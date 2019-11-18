// license:BSD-3-Clause
// copyright-holders:Robbbert
/**********************************************************************************

  PINBALL
  Hankin
  Based on Bally BY35
  Sound based on Atari System 1


ToDo:
- High score isn't saved or remembered
- Sound needs to be verified with original
- Mechanical

***********************************************************************************/

#include "emu.h"
#include "machine/genpin.h"

#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "machine/timer.h"
#include "sound/dac.h"
#include "sound/volt_reg.h"
#include "speaker.h"

#include "hankin.lh"

class hankin_state : public genpin_class
{
public:
	hankin_state(const machine_config &mconfig, device_type type, const char *tag)
		: genpin_class(mconfig, type, tag)
		, m_p_prom(*this, "prom")
		, m_maincpu(*this, "maincpu")
		, m_audiocpu(*this, "audiocpu")
		, m_ic10(*this, "ic10")
		, m_ic11(*this, "ic11")
		, m_ic2(*this, "ic2")
		, m_dac(*this, "dac")
		, m_io_test(*this, "TEST")
		, m_io_x(*this, { "X0", "X1", "X2", "X3", "X4", "DSW0", "DSW1", "DSW2" })
		, m_display(*this, "digit%u%u", 0U, 0U)
	{ }

	DECLARE_INPUT_CHANGED_MEMBER(self_test);
	void hankin(machine_config &config);

private:
	DECLARE_WRITE_LINE_MEMBER(ic10_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(ic10_cb2_w);
	DECLARE_WRITE_LINE_MEMBER(ic11_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(ic11_cb2_w);
	DECLARE_WRITE_LINE_MEMBER(ic2_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(ic2_cb2_w);
	DECLARE_WRITE8_MEMBER(ic10_a_w);
	DECLARE_WRITE8_MEMBER(ic10_b_w);
	DECLARE_WRITE8_MEMBER(ic11_a_w);
	DECLARE_WRITE8_MEMBER(ic2_b_w);
	DECLARE_WRITE8_MEMBER(ic2_a_w);
	DECLARE_READ8_MEMBER(ic11_b_r);
	DECLARE_READ8_MEMBER(ic2_a_r);
	TIMER_DEVICE_CALLBACK_MEMBER(timer_s);
	TIMER_DEVICE_CALLBACK_MEMBER(timer_x);

	virtual void machine_start() override;
	virtual void machine_reset() override;

	void hankin_map(address_map &map);
	void hankin_sub_map(address_map &map);

	bool m_timer_x;
	bool m_timer_sb;
	uint8_t m_timer_s[3];
	uint8_t m_vol;
	uint8_t m_ic2a;
	uint8_t m_ic2b;
	uint8_t m_ic10a;
	uint8_t m_ic10b;
	uint8_t m_ic11a;
	bool m_ic11_ca2;
	bool m_ic10_cb2;
	bool m_ic2_ca2;
	bool m_ic2_cb2;
	uint8_t m_counter;
	uint8_t m_digit;
	uint8_t m_segment[5];

	required_region_ptr<uint8_t> m_p_prom;
	required_device<m6802_cpu_device> m_maincpu;
	required_device<m6802_cpu_device> m_audiocpu;
	required_device<pia6821_device> m_ic10;
	required_device<pia6821_device> m_ic11;
	required_device<pia6821_device> m_ic2;
	required_device<dac_4bit_r2r_device> m_dac;
	required_ioport m_io_test;
	required_ioport_array<8> m_io_x;
	output_finder<5, 6> m_display;
};


void hankin_state::hankin_map(address_map &map)
{
	map.global_mask(0x1fff);
	map(0x0000, 0x007f).ram(); // internal to the cpu
	map(0x0088, 0x008b).rw(m_ic11, FUNC(pia6821_device::read), FUNC(pia6821_device::write));
	map(0x0090, 0x0093).rw(m_ic10, FUNC(pia6821_device::read), FUNC(pia6821_device::write));
	map(0x0200, 0x02ff).ram().share("nvram"); // 5101L 4-bit static ram
	map(0x1000, 0x1fff).rom().region("maincpu", 0);
}

void hankin_state::hankin_sub_map(address_map &map)
{
	map.global_mask(0x1fff);
	map(0x0000, 0x007f).ram(); // internal to the cpu
	map(0x0080, 0x0083).rw(m_ic2, FUNC(pia6821_device::read), FUNC(pia6821_device::write));
	map(0x1000, 0x17ff).rom().mirror(0x800).region("audiocpu", 0);
}

static INPUT_PORTS_START( hankin )
	PORT_START("TEST")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Self Test") PORT_IMPULSE(1) PORT_CHANGED_MEMBER(DEVICE_SELF, hankin_state, self_test, 0)

	PORT_START("DSW0")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR(Coinage))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_2C ))
	PORT_DIPSETTING(    0x06, "5 coins 4 credits")
	PORT_DIPSETTING(    0x07, "5 coins 2 credits")
	PORT_DIPNAME( 0x08, 0x08, "Award")
	PORT_DIPSETTING(    0x00, "Extra Ball")
	PORT_DIPSETTING(    0x08, "Free Game")
	PORT_DIPNAME( 0x10, 0x10, "Match")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x60, 0x40, "Credits for exceeding high score")
	PORT_DIPSETTING(    0x00, "0")
	PORT_DIPSETTING(    0x20, "1")
	PORT_DIPSETTING(    0x40, "2")
	PORT_DIPSETTING(    0x60, "3")
	PORT_DIPNAME( 0x80, 0x80, "Game Over Tune")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x80, DEF_STR( On ))

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x00, "Coin Alarm")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x01, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x00, "S10 (game specific)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x02, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, "Background Sound")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x04, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x00, "S12 (game Specific)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x08, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x00, "S13 (game specific)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x00, "S14 (game specific)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x20, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, "S15 (game specific)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x00, "Balls")
	PORT_DIPSETTING(    0x00, "3")
	PORT_DIPSETTING(    0x80, "5")

	PORT_START("DSW2")
	PORT_DIPNAME( 0x03, 0x02, "Maximum Credits")
	PORT_DIPSETTING(    0x00, "5")
	PORT_DIPSETTING(    0x01, "10")
	PORT_DIPSETTING(    0x02, "15")
	PORT_DIPSETTING(    0x03, "20")
	PORT_DIPNAME( 0x04, 0x00, "S19")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x04, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x00, "S20")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x08, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x00, "Remember Bonus Multiplier")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, "Free Game Sound")
	PORT_DIPSETTING(    0x00, "Special Tune")
	PORT_DIPSETTING(    0x20, "Knocker")
	PORT_DIPNAME( 0x40, 0x00, "Coin Counter reset") // see manual
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x00, "Time out in test mode")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x80, DEF_STR( On ))

	// Switches are numbered 8-1,16-9,24-17
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_SLASH)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_COLON)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_Z)

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_MINUS)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE2 ) PORT_NAME("Coin Door")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_TILT1 ) PORT_NAME("Slam Tilt")

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_K)

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_I)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_O)

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Outhole") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_M)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_COMMA)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_CODE(KEYCODE_STOP)
INPUT_PORTS_END

INPUT_CHANGED_MEMBER( hankin_state::self_test )
{
	m_ic11->ca1_w(newval);
}

WRITE8_MEMBER( hankin_state::ic10_a_w )
{
	m_ic10a = data;

	if (!m_ic11_ca2)
	{
		if (BIT(data, 2))
			m_digit = 5;
		else if (BIT(data, 3))
			m_digit = 4;
		else if (BIT(data, 4))
			m_digit = 3;
		else if (BIT(data, 5))
			m_digit = 2;
		else if (BIT(data, 6))
			m_digit = 1;
		else if (BIT(data, 7))
			m_digit = 0;

		// This machine has a 10-segment display, however the only
		// use is to place the '1' digit in the centre segments.
		if (BIT(data, 0) && (m_counter > 8))
		{
			static constexpr uint8_t patterns[16] = { 0x3f,0x80,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0,0,0,0,0,0 }; // MC14543 with '1' adjusted
			for (unsigned i = 0U; i < 5U; ++i)
				m_display[i][m_digit] = bitswap<10>(uint16_t(patterns[m_segment[i]]), 7, 7, 6, 6, 5, 4, 3, 2, 1, 0);
		}
	}
}

WRITE8_MEMBER( hankin_state::ic10_b_w )
{
	m_ic10b = data;

	if (!m_ic10_cb2)
	{
		switch (data & 15)
		{
			case 0x0: // knocker
				m_samples->start(0, 6);
				break;
			case 0x6: // outhole
				m_samples->start(0, 5);
				break;
			case 0x8:
			case 0x9:
			case 0xa: // bumpers
				m_samples->start(0, 0);
				break;
			case 0xb:
			case 0xd: // slings
				m_samples->start(0, 7);
				break;
		}
	}
	// also sound data
}

WRITE_LINE_MEMBER( hankin_state::ic10_ca2_w )
{
	output().set_value("led0", !state);
	// also sound strobe
	m_ic2->ca1_w(state);
}

WRITE_LINE_MEMBER( hankin_state::ic10_cb2_w )
{
	// solenoid strobe
	m_ic10_cb2 = state;
}

WRITE8_MEMBER( hankin_state::ic11_a_w )
{
	m_ic11a = data;

	if (!m_ic11_ca2)
	{
		m_counter++;

		if (m_counter==1)
			m_segment[0] = data>>4;
		else
		if (m_counter==3)
			m_segment[1] = data>>4;
		else
		if (m_counter==5)
			m_segment[2] = data>>4;
		else
		if (m_counter==7)
			m_segment[3] = data>>4;
		else
		if (m_counter==9)
			m_segment[4] = data>>4;
	}
}

READ8_MEMBER( hankin_state::ic11_b_r )
{
	uint8_t data = 0;

	for (unsigned i = 0U; i < 8U; ++i)
	{
		if (BIT(m_ic11a, i))
			data |= m_io_x[i]->read();
	}

	return data;
}

WRITE_LINE_MEMBER( hankin_state::ic11_ca2_w )
{
	m_ic11_ca2 = state;
	if (!state)
		m_counter = 0;
}

// lamp strobe
WRITE_LINE_MEMBER( hankin_state::ic11_cb2_w )
{
}

// zero-cross detection
TIMER_DEVICE_CALLBACK_MEMBER( hankin_state::timer_x )
{
	m_timer_x ^= 1;
	m_ic11->cb1_w(m_timer_x);
}

// Sound
// 555 osc at 47kHz
// Then optional divide by 2 controlled by CA2
// Then presettable 74LS161 binary divider controlled by PB0-3
// Then a pair of 7493 to generate 5 address lines, enabled by CB2
// The address lines are merged with PA4-7 to form a lookup on the prom
// Output of prom goes to a 4-bit DAC
// Volume is controlled by PB4-7
// Variables:
// m_timer_s[0] inc each timer cycle, bit 0 = 47k, bit 1 = 23.5k
// m_timer_s[1] count in 74LS161
// m_timer_s[2] count in 7493s
// m_timer_sb   wanted output of m_timer_s[0]
TIMER_DEVICE_CALLBACK_MEMBER( hankin_state::timer_s )
{
	m_timer_s[0]++;
	bool cs = (m_ic2_ca2) ? BIT(m_timer_s[0], 0) : BIT(m_timer_s[0], 1); // divide by 2 stage
	if (cs != m_timer_sb)
	{
		m_timer_sb = cs;
		m_timer_s[1]++;
		if (m_timer_s[1] > 15)
		{
			m_timer_s[1] = m_ic2b & 15; // set to preset value
			if (!m_ic2_cb2)
			{
				m_timer_s[2]++;
				offs_t offs = (m_timer_s[2] & 31) | (m_ic2a << 5);
				m_dac->write(m_p_prom[offs]);
			}
			else
				m_timer_s[2] = 0;
		}
	}
}

void hankin_state::machine_start()
{
	m_display.resolve();
}

void hankin_state::machine_reset()
{
	m_vol = 0;
	m_dac->set_output_gain(0, 0);
}

// PA0-3 = sound data from main cpu
READ8_MEMBER( hankin_state::ic2_a_r )
{
	return m_ic10b;
}

// PA4-7 = sound data to prom
WRITE8_MEMBER( hankin_state::ic2_a_w )
{
	m_ic2a = data >> 4;
	offs_t offs = (m_timer_s[2] & 31) | (m_ic2a << 5);
	m_dac->write(m_p_prom[offs]);
}

// PB0-3 = preset on 74LS161
// PB4-7 = volume
WRITE8_MEMBER( hankin_state::ic2_b_w )
{
	m_ic2b = data;

	data >>= 4;

	if (data != m_vol)
	{
		m_vol = data;
		float vol = m_vol/16.666+0.1;
		m_dac->set_output_gain(0, vol);
	}
}

// low to divide 555 by 2
WRITE_LINE_MEMBER( hankin_state::ic2_ca2_w )
{
	m_ic2_ca2 = state;
}

// low to enable 7493 dividers
WRITE_LINE_MEMBER( hankin_state::ic2_cb2_w )
{
	m_ic2_cb2 = state;
}

void hankin_state::hankin(machine_config &config)
{
	/* basic machine hardware */
	M6802(config, m_maincpu, 3276800);
	m_maincpu->set_addrmap(AS_PROGRAM, &hankin_state::hankin_map);

	M6802(config, m_audiocpu, 3276800); // guess, xtal value not shown
	m_audiocpu->set_addrmap(AS_PROGRAM, &hankin_state::hankin_sub_map);

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);

	/* Video */
	config.set_default_layout(layout_hankin);

	/* Sound */
	genpin_audio(config);

	SPEAKER(config, "speaker").front_center();
	DAC_4BIT_R2R(config, m_dac, 0).add_route(ALL_OUTPUTS, "speaker", 0.5); // unknown DAC
	voltage_regulator_device &vref(VOLTAGE_REGULATOR(config, "vref"));
	vref.add_route(0, "dac", 1.0, DAC_VREF_POS_INPUT);
	vref.add_route(0, "dac", -1.0, DAC_VREF_NEG_INPUT);

	/* Devices */
	PIA6821(config, m_ic10, 0);
	//m_ic10->readpa_handler().set(FUNC(hankin_state::ic10_a_r));
	m_ic10->writepa_handler().set(FUNC(hankin_state::ic10_a_w));
	//m_ic10->readpb_handler().set(FUNC(hankin_state::ic10_b_r));
	m_ic10->writepb_handler().set(FUNC(hankin_state::ic10_b_w));
	m_ic10->ca2_handler().set(FUNC(hankin_state::ic10_ca2_w));
	m_ic10->cb2_handler().set(FUNC(hankin_state::ic10_cb2_w));
	m_ic10->irqa_handler().set_inputline("maincpu", M6802_IRQ_LINE);
	m_ic10->irqb_handler().set_inputline("maincpu", M6802_IRQ_LINE);

	PIA6821(config, m_ic11, 0);
	//m_ic11->readpa_handler().set(FUNC(hankin_state::ic11_a_r));
	m_ic11->writepa_handler().set(FUNC(hankin_state::ic11_a_w));
	m_ic11->readpb_handler().set(FUNC(hankin_state::ic11_b_r));
	//m_ic11->writepb_handler().set(FUNC(hankin_state::ic11_b_w));
	m_ic11->ca2_handler().set(FUNC(hankin_state::ic11_ca2_w));
	m_ic11->cb2_handler().set(FUNC(hankin_state::ic11_cb2_w));
	m_ic11->irqa_handler().set_inputline("maincpu", M6802_IRQ_LINE);
	m_ic11->irqb_handler().set_inputline("maincpu", M6802_IRQ_LINE);

	PIA6821(config, m_ic2, 0);
	m_ic2->readpa_handler().set(FUNC(hankin_state::ic2_a_r));
	m_ic2->writepa_handler().set(FUNC(hankin_state::ic2_a_w));
	//m_ic2->readpb_handler().set(FUNC(hankin_state::ic2_b_r));
	m_ic2->writepb_handler().set(FUNC(hankin_state::ic2_b_w));
	m_ic2->ca2_handler().set(FUNC(hankin_state::ic2_ca2_w));
	m_ic2->cb2_handler().set(FUNC(hankin_state::ic2_cb2_w));
	m_ic2->irqa_handler().set_inputline("audiocpu", M6802_IRQ_LINE);
	m_ic2->irqb_handler().set_inputline("audiocpu", M6802_IRQ_LINE);

	TIMER(config, "timer_x").configure_periodic(FUNC(hankin_state::timer_x), attotime::from_hz(120)); // mains freq*2
	TIMER(config, "timer_s").configure_periodic(FUNC(hankin_state::timer_s), attotime::from_hz(94000)); // 555 on sound board*2
}

/*--------------------------------
/ FJ Holden
/-------------------------------*/
ROM_START(fjholden)
	ROM_REGION(0x1000, "maincpu", 0)
	ROM_LOAD("fj_ic2.mpu",   0x0000, 0x0800, CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052))
	ROM_LOAD("fj_ic3.mpu",   0x0800, 0x0800, CRC(ceaeb7d3) SHA1(9e479b985f8500983e71d6ff33ee94160e99650d))

	ROM_REGION(0x0800, "audiocpu", 0)
	ROM_LOAD("fj_ic14.snd",  0x0000, 0x0800, CRC(34fe3587) SHA1(132714675a23c101ceb5a4d544818650ae5ccea2))

	ROM_REGION(0x0200, "prom", 0)
	ROM_LOAD("fj_ic3.snd",   0x0000, 0x0200, CRC(09d3f020) SHA1(274be0b94d341ee43357011691da82e83a7c4a00))
ROM_END

/*--------------------------------
/ Howzat!
/-------------------------------*/
ROM_START(howzat)
	ROM_REGION(0x1000, "maincpu", 0)
	ROM_LOAD("hz_ic2.mpu",   0x0000, 0x0800, CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052))
	ROM_LOAD("hz_ic3.mpu",   0x0800, 0x0800, CRC(d13df4bc) SHA1(27a70260698d3eaa7cf7a56edc5dd9a4af3f4103))

	ROM_REGION(0x0800, "audiocpu", 0)
	ROM_LOAD("hz_ic14.snd",  0x0000, 0x0800, CRC(0e3fdb59) SHA1(cae3c85b2c32a0889785f770ece66b959bcf21e1))

	ROM_REGION(0x0200, "prom", 0)
	ROM_LOAD("hz_ic3.snd",   0x0000, 0x0200, CRC(dfc57606) SHA1(638853c8e46bf461f2ecde02b8b2aa68c2d414b8))
ROM_END

/*--------------------------------
/ Orbit 1
/-------------------------------*/
ROM_START(orbit1)
	ROM_REGION(0x1000, "maincpu", 0)
	ROM_LOAD("o1_ic2.mpu",   0x0000, 0x0800, CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052))
	ROM_LOAD("o1_ic3.mpu",   0x0800, 0x0800, CRC(fe7b61be) SHA1(c086b0433bb9ab3f2139c705d4372beb1656b27f))

	ROM_REGION(0x0800, "audiocpu", 0)
	ROM_LOAD("o1_ic14.snd",  0x0000, 0x0800, CRC(323bfbd5) SHA1(2e89aa4fcd33f9bfeea5c310ffb0a5be45fb70a9))

	ROM_REGION(0x0200, "prom", 0)
	ROM_LOAD("o1_ic3.snd",   0x0000, 0x0200, CRC(dfc57606) SHA1(638853c8e46bf461f2ecde02b8b2aa68c2d414b8))
ROM_END

/*--------------------------------
/ Shark
/-------------------------------*/
ROM_START(shark)
	ROM_REGION(0x1000, "maincpu", 0)
	ROM_LOAD("shk_ic2.mpu",  0x0000, 0x0800, CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052))
	ROM_LOAD("shk_ic3.mpu",  0x0800, 0x0800, CRC(c3ef936c) SHA1(14668496d162a77e03c1142bef2956d5b76afc99))

	ROM_REGION(0x0800, "audiocpu", 0)
	ROM_LOAD("shk_ic14.snd", 0x0000, 0x0800, CRC(8f8b0e48) SHA1(72d94aa9b32c603b1ca681b0ab3bf8ddbf5c9afe))

	ROM_REGION(0x0200, "prom", 0)
	ROM_LOAD("shk_ic3.snd",  0x0000, 0x0200, CRC(dfc57606) SHA1(638853c8e46bf461f2ecde02b8b2aa68c2d414b8))
ROM_END

/*--------------------------------
/ The Empire Strike Back
/-------------------------------*/
ROM_START(empsback)
	ROM_REGION(0x1000, "maincpu", 0)
	ROM_LOAD("sw_ic2.mpu",   0x0000, 0x0800, CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052))
	ROM_LOAD("sw_ic3.mpu",   0x0800, 0x0800, CRC(837ffe32) SHA1(9affc5d9345ce15394553d3204e5234cc6348d2e))

	ROM_REGION(0x0800, "audiocpu", 0)
	ROM_LOAD("sw_ic14.snd",  0x0000, 0x0800, CRC(c1eeb53b) SHA1(7a800dd0a8ae392e14639e1819198d4215cc2251))

	ROM_REGION(0x0200, "prom", 0)
	ROM_LOAD("sw_ic3.snd",   0x0000, 0x0200, CRC(db214f65) SHA1(1a499cf2059a5c0d860d5a4251a89a5735937ef8))
ROM_END


GAME(1978,  fjholden, 0, hankin, hankin, hankin_state, empty_init, ROT0, "Hankin", "FJ Holden",              MACHINE_MECHANICAL | MACHINE_NOT_WORKING )
GAME(1978,  orbit1,   0, hankin, hankin, hankin_state, empty_init, ROT0, "Hankin", "Orbit 1",                MACHINE_MECHANICAL | MACHINE_NOT_WORKING )
GAME(1980,  shark,    0, hankin, hankin, hankin_state, empty_init, ROT0, "Hankin", "Shark",                  MACHINE_MECHANICAL | MACHINE_NOT_WORKING )
GAME(1980,  howzat,   0, hankin, hankin, hankin_state, empty_init, ROT0, "Hankin", "Howzat!",                MACHINE_MECHANICAL | MACHINE_NOT_WORKING )
GAME(1981,  empsback, 0, hankin, hankin, hankin_state, empty_init, ROT0, "Hankin", "The Empire Strike Back", MACHINE_MECHANICAL | MACHINE_NOT_WORKING )
