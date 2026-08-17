#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Generate a dense Intel HEX file filled with erase bytes."""

import argparse
from intelhex import IntelHex

def parse_regions(spec):
    regions = []
    for part in spec.split(","):
        addr_s, size_s = part.split(":")
        regions.append((int(addr_s, 0), int(size_s, 0)))
    return regions

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--regions", required=True,
                   help="Comma-separated absolute addr:size hex pairs, e.g. 0xaa000:0x60000")
    p.add_argument("--fill", type=lambda x: int(x, 0), default=0xFF)
    p.add_argument("--output", required=True)
    args = p.parse_args()

    ih = IntelHex()
    for addr, size in parse_regions(args.regions):
        ih.puts(addr, bytes([args.fill & 0xFF]) * size)

    ih.write_hex_file(args.output, write_start_addr=False, byte_count=32)

if __name__ == "__main__":
    main()