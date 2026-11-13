# ROM database: a second `CRC=` line in one section makes the CRC index cyclic, `ini_search_by_crc` never returns

- Reproduce (hangs; `timeout 20` exits 124): `timeout 20 /mayhem/fuzz_romdb-standalone mayhem/fuzz_romdb/known-findings/duplicate-crc-key-cycle/duplicate-crc.ini`
- Cause: in `romdatabase_open` (`src/main/rom.c:683-692`) every valid `CRC=` property pushes the section's
  node onto `crc_lists[crc1 >> 24]` (`search->next_crc = crc_lists[index]`). A second `CRC=` line in the same
  section whose `crc1` has the same top byte pushes the node onto its own list, so `next_crc` points at itself.
  The `while (search != NULL)` loop in `ini_search_by_crc` (`rom.c:928-936`) then spins forever for any lookup
  in that bucket that does not stop on a second match, e.g. a ROM whose CRC1 has the same top byte.
- Impact: a `mupen64plus.ini` with a duplicated `CRC=` key hangs `open_rom` / `CoreDoCommand(M64CMD_ROM_GET_SETTINGS)`.
- Fix: ignore (or replace) a repeated `CRC=` property for a section that is already indexed.
- Effect on the target: a hit costs a whole `timeout:` (30 s) of the fuzzing run, which is why `fuzz-romdb`
  runs at tens of exec/s. The harness does not guard it (a hang is a finding, not something to skip).
