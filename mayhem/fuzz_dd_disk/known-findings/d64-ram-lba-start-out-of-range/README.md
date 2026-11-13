# D64 loader: ram_lba_start beyond MAX_LBA indexes tables with vzone -1

- Reproduce: `/mayhem/fuzz_dd_disk-standalone mayhem/fuzz_dd_disk/known-findings/d64-ram-lba-start-out-of-range/ram-lba-start-0x15d3.ndd`
- Cause: in the D64 branch of `scan_and_expand_disk_format` (`src/device/dd/disk.c:551`) the system data's
  `ram_lba_start`/`ram_lba_end` are passed to `LBAToByteA(disk_type, SYSTEM_LBAS + ram_lba_start, ...)` unchecked.
  For an LBA above `MAX_LBA` `LBAToVZoneA` returns `-1`, which `LBAToByteA` then uses as an index into
  `DiskTypeZones` / `VZoneLBATable` (line 142), reading far outside the tables (ASan: SEGV).
- Impact: a crafted `.d64` image crashes the emulator at load time.
- Fix: reject `rom_lba_end`/`ram_lba_start`/`ram_lba_end` greater than `MAX_LBA - SYSTEM_LBAS` before sizing,
  and make `LBAToByteA` return `0xFFFFFFFF` when `LBAToVZoneA` returns `-1`.
