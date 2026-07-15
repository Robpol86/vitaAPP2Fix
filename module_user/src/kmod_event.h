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
 * @brief Start and stop the kernel module event handling thread.
 ******************************************************************************/

#ifndef KMOD_EVENT_H
#define KMOD_EVENT_H

#include "vqmbt.h"

typedef void (*KmeCallback)(void);
typedef void (*KmeCallbackEvent)(const VqmbtEvent* event);

void kmod_event_start(KmeCallback on_start, KmeCallback on_event_dropped, KmeCallbackEvent on_event);
void kmod_event_stop(void);

#endif  // KMOD_EVENT_H
