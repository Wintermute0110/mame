// license:BSD-3-Clause
// copyright-holders:Nigel Barnes
/***************************************************************************

    Advanced Battery-Backed RAM

***************************************************************************/

#ifndef MAME_BUS_ELECTRON_CART_ABR_H
#define MAME_BUS_ELECTRON_CART_ABR_H

#pragma once

#include "slot.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> electron_abr_device

class electron_abr_device : public device_t,
								public device_electron_cart_interface
{
public:
	// construction/destruction
	electron_abr_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// device-level overrides
	virtual void device_start() override;

	// electron_cart_interface overrides
	virtual uint8_t read(offs_t offset, int infc, int infd, int romqa, int oe, int oe2) override;
	virtual void write(offs_t offset, uint8_t data, int infc, int infd, int romqa, int oe, int oe2) override;

private:
	bool m_bank_locked[2];
};

// device type definition
DECLARE_DEVICE_TYPE(ELECTRON_ABR, electron_abr_device)

#endif // MAME_BUS_ELECTRON_CART_ABR_H
