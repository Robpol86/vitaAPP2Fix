/*
This file is part of vitaAPP2Fix.
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
 * @brief Decode AVDTP signaling PDUs to log negotiated audio codecs.
 ******************************************************************************/

#ifndef AVDTP_PARSE_H
#define AVDTP_PARSE_H

/**
 * Parse and log one AVDTP signaling PDU.
 *
 * @param pdu Pointer to the AVDTP signaling PDU (after any L2CAP/ACL header).
 * @param len Length of the PDU in bytes.
 */
void avdtp_parse(const unsigned char* pdu, unsigned int len);

#endif  // AVDTP_PARSE_H
