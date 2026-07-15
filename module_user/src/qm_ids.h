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
 * @brief Quick Menu widget IDs.
 ******************************************************************************/

#ifndef QM_IDS_H
#define QM_IDS_H

#include "vqmbt.h"

// Widget IDs (prefixed because they must be unique across all plugins).
#define QM_ID_SEPARATOR MODULE_NAME "Separator"
#define QM_ID_SECTION_TITLE MODULE_NAME "SectionTitle"
#define QM_ID_PLANE_BUTTONS MODULE_NAME "PlaneButtons"

// Button IDs.
static const char* const QM_ID_BUTTONS[VQMBT_MAX_DEVICES] = {
    MODULE_NAME "Button0", MODULE_NAME "Button1", MODULE_NAME "Button2", MODULE_NAME "Button3",
    MODULE_NAME "Button4", MODULE_NAME "Button5", MODULE_NAME "Button6", MODULE_NAME "Button7",
};
_Static_assert(sizeof(QM_ID_BUTTONS) / sizeof(QM_ID_BUTTONS[0]) == VQMBT_MAX_DEVICES,
               "QM_ID_BUTTONS size != VQMBT_MAX_DEVICES");

#endif  // QM_IDS_H
