#pragma once

#ifndef __K005289_H__
#define __K005289_H__

#include "devlegcy.h"

WRITE8_DEVICE_HANDLER( k005289_control_A_w );
WRITE8_DEVICE_HANDLER( k005289_control_B_w );
WRITE8_DEVICE_HANDLER( k005289_pitch_A_w );
WRITE8_DEVICE_HANDLER( k005289_pitch_B_w );
WRITE8_DEVICE_HANDLER( k005289_keylatch_A_w );
WRITE8_DEVICE_HANDLER( k005289_keylatch_B_w );

DECLARE_LEGACY_SOUND_DEVICE(K005289, k005289);

#endif /* __K005289_H__ */
