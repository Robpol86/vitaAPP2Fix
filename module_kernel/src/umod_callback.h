/*
This file is part of vitaQmBluetooth.
Copyright © 2026 Robpol86

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

/******************************************************************************
 * @file
 * @brief Start and stop the user module event emitting thread.
 ******************************************************************************/

#ifndef UMOD_CALLBACK_H
#define UMOD_CALLBACK_H

#include "vqmbt.h"

int umod_cb_emit_event(const VqmbtEvent* event);
int umod_cb_start(void);
int umod_cb_stop(void);

#endif  // UMOD_CALLBACK_H
