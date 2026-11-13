# ROM header Name read past its end

- Reproduce: `/mayhem/fuzz_rom-standalone mayhem/fuzz_rom/known-findings/rom-name-not-terminated/header-0x20-0x3f-nonzero.z64`
- Cause: `src/main/rom.c:235` logs `DebugMessage(M64MSG_INFO, "Name: %s", ROM_HEADER.Name)`. `Name` is a
  20-byte field copied straight from the ROM with no terminator, so when the header bytes after it (0x20-0x3f)
  are all non-zero, `%s` runs off the end of the global `ROM_HEADER` (ASan: global-buffer-overflow in `vsnprintf`).
- Impact: out-of-bounds read of a few bytes of adjacent globals while loading an untrusted ROM; the text can
  reach the frontend's debug callback (information leak, or crash if the next page is unmapped).
- Fix: `DebugMessage(M64MSG_INFO, "Name: %.20s", ROM_HEADER.Name);` (the copy at line 181 already uses a fixed 20).
