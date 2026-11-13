# ConfigExternal: file buffer is not NUL-terminated

- Reproduce: `M64P_FUZZ_RAW_CONFIG=1 /mayhem/fuzz_config-standalone mayhem/fuzz_config/known-findings/config-external-not-terminated/unterminated.cfg`
  (without the variable the harness ends the file with a newline, which avoids the bug so the rest of the parser can be fuzzed)
- Cause: `ConfigExternalOpen` (`src/api/config.c`) reads the file into a `malloc`ed buffer of `size + 1` bytes
  and never terminates it; `ConfigExternalGetParameter` (line ~706) then copies `length + 1` bytes of it and hands
  them to `ini_parse_line` / `strchr`, which read past the allocation (ASan: heap-buffer-overflow in
  `strchr`). It triggers whenever the last line has no trailing newline.
- Impact: out-of-bounds heap read when a frontend queries an externally supplied config file.
- Fix: set `ext_config->file[filelen] = '\0'` in `ConfigExternalOpen`.
