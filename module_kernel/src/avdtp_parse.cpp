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
 * @brief TIER 2: decode an AVDTP signaling PDU and print the negotiated codec.
 *
 * This is protocol-only; it does not touch the kernel. Feed it the AVDTP
 * signaling payload (everything AFTER the L2CAP header) and it will log the
 * signal type and, for GET_CAPABILITIES responses / SET_CONFIGURATION commands,
 * the Media Codec capability -- SBC parameters, AAC, ATRAC, or vendor.
 *
 * FRAMING, so you know how much to strip depending on where you hook:
 *
 *   HCI ACL packet you'd see at the lowest RX point:
 *     [ACL header 4B] [L2CAP: len 2B LE][CID 2B LE] [AVDTP signaling PDU ...]
 *
 *   So: if you hook the ACL/L2CAP RX path, skip 4 (ACL) + 4 (L2CAP) = 8 bytes,
 *   then hand the rest here. If you hook an internal AVDTP handler that already
 *   receives the reassembled signaling PDU, pass its buffer directly.
 *
 * AVDTP signaling PDU layout (single-packet; fragmentation ignored for
 * brevity -- codec caps almost always fit one packet):
 *   byte0: [Transaction Label :4][Packet Type :2][Message Type :2]
 *   byte1: [RFA :2][Signal Identifier :6]
 *   byte2+: signal-specific payload
 *
 * Media Codec capability element (Service Category 0x07):
 *   [Category=0x07][LOSC][MediaType<<4][CodecType][codec-specific bytes...]
 *   SBC codec-specific (4 bytes):
 *     b0: [sampling_freq :4][channel_mode :4]
 *     b1: [block_length :4][subbands :2][alloc_method :2]
 *     b2: min_bitpool
 *     b3: max_bitpool
 ******************************************************************************/

#include "avdtp_parse.h"

#include "log.h"

// AVDTP signal identifiers.
static const char* avdtp_signal_name(unsigned int sig) {
    switch (sig) {
        case 0x01:
            return "DISCOVER";
        case 0x02:
            return "GET_CAPABILITIES";
        case 0x03:
            return "SET_CONFIGURATION";
        case 0x04:
            return "GET_CONFIGURATION";
        case 0x05:
            return "RECONFIGURE";
        case 0x06:
            return "OPEN";
        case 0x07:
            return "START";
        case 0x08:
            return "CLOSE";
        case 0x09:
            return "SUSPEND";
        case 0x0A:
            return "ABORT";
        case 0x0C:
            return "GET_ALL_CAPABILITIES";
        default:
            return "?";
    }
}

static const char* avdtp_msgtype_name(unsigned int mt) {
    switch (mt) {
        case 0x0:
            return "CMD";
        case 0x2:
            return "ACCEPT";
        case 0x3:
            return "REJECT";
        default:
            return "?";
    }
}

static const char* sbc_freq(unsigned int bit) {
    switch (bit) {
        case 0x8:
            return "16000";
        case 0x4:
            return "32000";
        case 0x2:
            return "44100";
        case 0x1:
            return "48000";
        default:
            return "multi/none";
    }
}

static const char* sbc_chan(unsigned int bit) {
    switch (bit) {
        case 0x8:
            return "mono";
        case 0x4:
            return "dual";
        case 0x2:
            return "stereo";
        case 0x1:
            return "joint-stereo";
        default:
            return "multi/none";
    }
}

// Decode one Media Codec capability body (already positioned past cat+losc).
static void decode_media_codec(const unsigned char* p, unsigned int losc) {
    if (losc < 2) {
        LOG_DEBUG(0, "  MEDIA_CODEC: truncated (losc=%u)", losc);
        return;
    }
    unsigned int media_type = (p[0] >> 4) & 0x0F;
    unsigned int codec_type = p[1];
    const char* codec = "unknown";
    switch (codec_type) {
        case 0x00:
            codec = "SBC";
            break;
        case 0x01:
            codec = "MPEG-1/2-Audio";
            break;
        case 0x02:
            codec = "MPEG-2/4-AAC";
            break;
        case 0x04:
            codec = "ATRAC";
            break;
        case 0xFF:
            codec = "non-A2DP(vendor,e.g.aptX/LDAC)";
            break;
        default:
            break;
    }
    LOG_DEBUG(0, "  MEDIA_CODEC media_type=%u codec=0x%02X (%s)", media_type, codec_type, codec);

    if (codec_type == 0x00 && losc >= 6) {  // SBC
        unsigned int freq = (p[2] >> 4) & 0x0F;
        unsigned int chan = p[2] & 0x0F;
        unsigned int block = (p[3] >> 4) & 0x0F;
        unsigned int sub = (p[3] >> 2) & 0x03;
        unsigned int alloc = p[3] & 0x03;
        unsigned int minbp = p[4];
        unsigned int maxbp = p[5];
        LOG_DEBUG(0, "    SBC freq=0x%X(%s) chan=0x%X(%s) block=0x%X sub=%s alloc=%s bitpool=%u..%u", freq,
                  sbc_freq(freq), chan, sbc_chan(chan), block, (sub == 0x2 ? "4" : "8"),
                  (alloc == 0x2 ? "SNR" : "Loudness"), minbp, maxbp);
    } else {
        // Dump the raw codec-specific bytes for AAC/vendor so you can eyeball diffs.
        char hex[3 * 16 + 1];
        unsigned int n = losc - 2;
        if (n > 16) n = 16;
        for (unsigned int i = 0; i < n; i++) {
            const char* d = "0123456789ABCDEF";
            hex[i * 3 + 0] = d[(p[2 + i] >> 4) & 0xF];
            hex[i * 3 + 1] = d[p[2 + i] & 0xF];
            hex[i * 3 + 2] = ' ';
        }
        hex[n * 3] = '\0';
        LOG_DEBUG(0, "    codec-specific: %s", hex);
    }
}

// Walk the service capability list starting at `p` for `len` bytes.
static void walk_capabilities(const unsigned char* p, unsigned int len) {
    unsigned int i = 0;
    while (i + 2 <= len) {
        unsigned int cat = p[i];
        unsigned int losc = p[i + 1];
        if (i + 2 + losc > len) {
            LOG_DEBUG(0, "  cap cat=0x%02X losc=%u OVERRUNS pdu, stop", cat, losc);
            break;
        }
        const unsigned char* body = &p[i + 2];
        switch (cat) {
            case 0x01:
                LOG_DEBUG(0, "  MEDIA_TRANSPORT");
                break;
            case 0x02:
                LOG_DEBUG(0, "  REPORTING");
                break;
            case 0x03:
                LOG_DEBUG(0, "  RECOVERY");
                break;
            case 0x04: {  // Content protection.
                unsigned int cptype = (losc >= 2) ? (body[0] | (body[1] << 8)) : 0;
                LOG_DEBUG(0, "  CONTENT_PROTECTION cp_type=0x%04X%s", cptype, cptype == 0x0002 ? " (SCMS-T)" : "");
                break;
            }
            case 0x05:
                LOG_DEBUG(0, "  HEADER_COMPRESSION");
                break;
            case 0x06:
                LOG_DEBUG(0, "  MULTIPLEXING");
                break;
            case 0x07:
                decode_media_codec(body, losc);
                break;
            case 0x08:
                LOG_DEBUG(0, "  DELAY_REPORTING");
                break;
            default:
                LOG_DEBUG(0, "  cap cat=0x%02X losc=%u", cat, losc);
                break;
        }
        i += 2 + losc;
    }
}

// Decode the SEP list from a DISCOVER ACCEPT response. Each Stream End Point is
// two bytes:
//   byte0: [ACP SEID :6][in_use :1][rfa :1]
//   byte1: [media_type :4][tsep :1][rfa :3]   (tsep 0 = SRC, 1 = SNK)
// This is the only place the peer's SEIDs are visible from the RX side, and the
// SBC SEP's SEID here is what an injected GET_CONFIGURATION would need to target.
static void decode_discover(const unsigned char* p, unsigned int len) {
    for (unsigned int i = 0; i + 2 <= len; i += 2) {
        unsigned int seid = (p[i] >> 2) & 0x3F;
        unsigned int in_use = (p[i] >> 1) & 0x01;
        unsigned int media_type = (p[i + 1] >> 4) & 0x0F;
        unsigned int tsep = (p[i + 1] >> 3) & 0x01;
        const char* media = media_type == 0 ? "audio" : (media_type == 1 ? "video" : "multimedia");
        LOG_DEBUG(0, "  SEP seid=%u media=%s type=%s in_use=%u", seid, media, tsep ? "SNK" : "SRC", in_use);
    }
}

void avdtp_parse(const unsigned char* pdu, unsigned int len) {
    if (len < 2) return;

    unsigned int label = (pdu[0] >> 4) & 0x0F;
    unsigned int pkttype = (pdu[0] >> 2) & 0x03;
    unsigned int msgtype = pdu[0] & 0x03;
    unsigned int sig = pdu[1] & 0x3F;

    // Only single-packet PDUs are decoded here (pkttype 0 == single).
    LOG_DEBUG(0, "AVDTP label=%u %s %s%s", label, avdtp_signal_name(sig), avdtp_msgtype_name(msgtype),
              pkttype != 0 ? " [fragmented, body skipped]" : "");
    if (pkttype != 0) return;

    // Capability-bearing PDUs we care about:
    //   DISCOVER ACCEPT: SEP list starts at byte 2.
    //   GET_CAPABILITIES / GET_ALL_CAPABILITIES ACCEPT: caps start at byte 2.
    //   SET_CONFIGURATION CMD: byte2=ACP SEID, byte3=INT SEID, caps start at 4.
    //   GET_CONFIGURATION ACCEPT: caps start at byte 2.
    if (sig == 0x01 && msgtype == 0x2) {
        decode_discover(&pdu[2], len - 2);
    } else if ((sig == 0x02 || sig == 0x0C) && msgtype == 0x2) {
        walk_capabilities(&pdu[2], len - 2);
    } else if (sig == 0x03 && msgtype == 0x0) {
        if (len >= 4) {
            LOG_DEBUG(0, "  ACP_SEID=%u INT_SEID=%u", (pdu[2] >> 2) & 0x3F, (pdu[3] >> 2) & 0x3F);
            walk_capabilities(&pdu[4], len - 4);
        }
    } else if (sig == 0x04 && msgtype == 0x2) {
        walk_capabilities(&pdu[2], len - 2);
    }
}
