"""heygo_link - the phone side of the doc 07 protocol, version 0.2, in plain Python.

This module has no BLE code. phone_sim.py uses it over BLE, and
tests/test_phone_sim.py checks it byte for byte against the C code of the firmware.
"""

import struct

PROTO_MAJOR, PROTO_MINOR = 0, 2
HEADER = 4
ADPCM_BLOCK = 164
BLOCK_SAMPLES = 320
SAMPLE_RATE = 16000

NUS_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   # Phone to device.
NUS_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   # Device to phone.
NAME_PREFIX = "HeygoPP-"

HELLO, HELLO_ACK, PHONE_STATUS, DEVICE_STATUS, ERROR = 0x01, 0x02, 0x03, 0x04, 0x0F
CONTEXT = 0x10
AGENT_START, AGENT_END, AGENT_CANCEL, AGENT_STATE = 0x20, 0x21, 0x22, 0x23
AUDIO_UP, AUDIO_DOWN, PLAY_START, PLAY_END = 0x30, 0x31, 0x32, 0x33
LIVE_STATS, RUN_REPORT, SHOW_PAGE, TURN_SHAPE = 0x60, 0x61, 0x62, 0x63
PAGE_REPORT = 3

TYPE_NAMES = {
    HELLO: "HELLO", HELLO_ACK: "HELLO_ACK", PHONE_STATUS: "PHONE_STATUS",
    DEVICE_STATUS: "DEVICE_STATUS", ERROR: "ERROR", CONTEXT: "CONTEXT",
    AGENT_START: "AGENT_START", AGENT_END: "AGENT_END", AGENT_CANCEL: "AGENT_CANCEL",
    AGENT_STATE: "AGENT_STATE", AUDIO_UP: "AUDIO_UP", AUDIO_DOWN: "AUDIO_DOWN",
    PLAY_START: "PLAY_START", PLAY_END: "PLAY_END",
    LIVE_STATS: "LIVE_STATS", RUN_REPORT: "RUN_REPORT", SHOW_PAGE: "SHOW_PAGE", TURN_SHAPE: "TURN_SHAPE",
}

ACCEPTED, RECOGNIZING, THINKING, ANSWERING, DONE, FAILED = 1, 2, 3, 4, 5, 6
FAIL_RECOGNITION, FAIL_TIMEOUT, FAIL_GATEWAY = 1, 2, 3
CODEC_ADPCM = 0
CHANNEL_AGENT = 0

# Increment 2 data, doc 07 section 6. The phone sends these values when it cannot
# calculate a field.
U8_UNKNOWN, U16_UNKNOWN = 0xFF, 0xFFFF
RUN_UNKNOWN, RUN_SKIING, RUN_STOPPED, RUN_ENDED = 0, 1, 2, 3
PROBLEM_MAX, TIP_MAX = 36, 60   # UTF-8 bytes: 12 and 20 Chinese characters.
# Increment 3: TURN_SHAPE, doc 07 section 6.
SRC_MEASURED, SRC_SCHEMATIC = 0, 1
SIDE_TOE, SIDE_HEEL = 0, 1
TURN_POINTS_MAX, TURNS_MAX, EDGE_MAX = 40, 12, 90


# ---------------------------------------------------------------- packets
def packet(ptype, xid=0, seq=0, payload=b"", flags=0):
    return bytes([ptype, flags, xid, seq & 0xFF]) + payload


def parse(data):
    """Return (type, flags, xid, seq, payload). Raise ValueError for a short packet."""
    if len(data) < HEADER:
        raise ValueError(f"packet shorter than the header: {len(data)} bytes")
    return data[0], data[1], data[2], data[3], bytes(data[4:])


def hello_ack(utc, app="phone_sim"):
    return packet(HELLO_ACK, payload=struct.pack("<BBI", PROTO_MAJOR, PROTO_MINOR, utc) + app.encode()[:32])


def phone_status(utc, online=True, gateway=True, background=False):
    return packet(PHONE_STATUS, payload=struct.pack("<IBBB", utc, online, gateway, background))


def agent_state(xid, state, failure=0, text=""):
    return packet(AGENT_STATE, xid, payload=bytes([state, failure]) + text.encode())


def play_start(xid, channel=CHANNEL_AGENT, codec=CODEC_ADPCM):
    return packet(PLAY_START, xid, payload=bytes([channel, codec]))


def audio_down(xid, seq, block):
    if len(block) != ADPCM_BLOCK:
        raise ValueError("an ADPCM block has 164 bytes")
    return packet(AUDIO_DOWN, xid, seq, block)


def play_end(xid):
    return packet(PLAY_END, xid)


def live_stats(run_state, run_no, speed_dkmh, drop_m, distance_m, turns, time_s):
    """One LIVE_STATS packet. Speed in 0.1 km/h. Use U8_UNKNOWN or U16_UNKNOWN for a missing field."""
    return packet(LIVE_STATS, payload=struct.pack("<BBHHHHH", run_state, run_no, speed_dkmh, drop_m,
                                                  distance_m, turns, time_s))


def run_report(run_no, distance_m, turns, iq, problem="", tip=""):
    """One RUN_REPORT packet. iq is 0 to 100, or U8_UNKNOWN."""
    p, t = problem.encode(), tip.encode()
    if len(p) > PROBLEM_MAX or len(t) > TIP_MAX:
        raise ValueError("problem holds at most 36 bytes, tip at most 60 bytes")
    if iq > 100 and iq != U8_UNKNOWN:
        raise ValueError("iq is 0 to 100, or U8_UNKNOWN")
    return packet(RUN_REPORT, payload=struct.pack("<BHHBB", run_no, distance_m, turns, iq, len(p)) + p + t)


def show_page(page=PAGE_REPORT):
    """Ask the device to show a page. Only the Report page (3) is valid."""
    if page != PAGE_REPORT:
        raise ValueError("SHOW_PAGE accepts only the Report page (3)")
    return packet(SHOW_PAGE, payload=bytes([page]))


def turn_shape(run_no, source, points, turns):
    """One TURN_SHAPE packet.

    points: (x, y) pairs from 0 to 255. x is the side axis, y grows downhill.
    turns: (first, last, side, speed_kmh, edge_deg, radius_m). Use U8_UNKNOWN for a missing value.
    """
    if not 2 <= len(points) <= TURN_POINTS_MAX or len(turns) > TURNS_MAX:
        raise ValueError("2 to 40 points, and at most 12 turns")
    if source not in (SRC_MEASURED, SRC_SCHEMATIC):
        raise ValueError("source is 0 (measured) or 1 (schematic)")
    for first, last, side, _speed, edge, _radius in turns:
        if not first < last < len(points):
            raise ValueError("a turn needs first < last < point count")
        if side not in (SIDE_TOE, SIDE_HEEL, U8_UNKNOWN) or (edge > EDGE_MAX and edge != U8_UNKNOWN):
            raise ValueError("bad side or edge angle")
    body = bytes([run_no, source, len(points), len(turns)])
    body += b"".join(bytes([x, y]) for x, y in points)
    body += b"".join(bytes(t) for t in turns)
    return packet(TURN_SHAPE, payload=body)


def demo_turn_shape(run_no, n_turns=8, source=SRC_MEASURED):
    """A run of n_turns linked turns for tests and demos: an S line down the slope.
    The side swing is a third of the run length, as on a real slope.

    Each turn uses 3 segments. Even turns swing to the toe side, odd turns to the heel side.
    The metrics follow a fixed pattern, so the same run always gives the same packet.
    """
    import math
    n_turns = max(1, min(n_turns, TURNS_MAX))
    n_points = 3 * n_turns + 1
    points = []
    for k in range(n_points):
        phase = math.pi * k / 3
        points.append((128 + round(50 * math.sin(phase)), 255 * k // (n_points - 1)))
    turns = []
    for i in range(n_turns):
        turns.append((3 * i, 3 * i + 3, SIDE_TOE if i % 2 == 0 else SIDE_HEEL,
                      45 + (i * 7) % 15, 38 + (i * 11) % 25, 9 + (i * 5) % 8))
    return points, turns


def read_hello(payload):
    major, minor, mask, mtu = struct.unpack_from("<BBBH", payload)
    return {"major": major, "minor": minor, "codec_mask": mask, "mtu": mtu,
            "fw": payload[5:].decode(errors="replace")}


def read_device_status(payload):
    battery, mode, page, heap = struct.unpack_from("<BBBH", payload)
    return {"battery": battery, "mode": mode, "page": page, "free_kib": heap}


# ---------------------------------------------------------------- IMA ADPCM, doc 07 section 7
INDEX_TABLE = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]
STEP_TABLE = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
]


class ImaState:
    def __init__(self):
        self.predictor = 0
        self.index = 0

    def apply(self, nibble):
        step = STEP_TABLE[self.index]
        diff = step >> 3
        if nibble & 4:
            diff += step
        if nibble & 2:
            diff += step >> 1
        if nibble & 1:
            diff += step >> 2
        pred = self.predictor - diff if nibble & 8 else self.predictor + diff
        self.predictor = max(-32768, min(32767, pred))
        self.index = max(0, min(88, self.index + INDEX_TABLE[nibble & 0x0F]))
        return self.predictor

    def encode_sample(self, sample):
        step = STEP_TABLE[self.index]
        diff = sample - self.predictor
        nibble = 0
        if diff < 0:
            nibble, diff = 8, -diff
        if diff >= step:
            nibble |= 4
            diff -= step
        step >>= 1
        if diff >= step:
            nibble |= 2
            diff -= step
        step >>= 1
        if diff >= step:
            nibble |= 1
        self.apply(nibble)
        return nibble


def encode_block(state, samples):
    """Encode up to 320 samples. The rest of the block gets samples of value 0."""
    if len(samples) > BLOCK_SAMPLES:
        raise ValueError("a block holds at most 320 samples")
    out = bytearray(ADPCM_BLOCK)
    struct.pack_into("<hBB", out, 0, state.predictor, state.index, 0)
    for k in range(BLOCK_SAMPLES):
        nibble = state.encode_sample(samples[k] if k < len(samples) else 0)
        out[4 + k // 2] |= (nibble << 4) if k & 1 else nibble
    return bytes(out)


def decode_block(block):
    if len(block) != ADPCM_BLOCK or block[2] > 88:
        raise ValueError("bad ADPCM block")
    state = ImaState()
    state.predictor, state.index = struct.unpack_from("<hB", block, 0)
    pcm = []
    for k in range(BLOCK_SAMPLES):
        byte = block[4 + k // 2]
        pcm.append(state.apply(byte >> 4 if k & 1 else byte & 0x0F))
    return pcm


def encode_stream(samples):
    """Split PCM samples into ADPCM blocks. The last block is filled with silence."""
    state = ImaState()
    return [encode_block(state, samples[i:i + BLOCK_SAMPLES]) for i in range(0, len(samples), BLOCK_SAMPLES)]
