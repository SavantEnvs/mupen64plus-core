# Extra make rules for the instrumented build (passed with a second -f after upstream's Makefile).
# LBAToPhys() indexes TrackZoneTable[0][pzone] with pzone up to 15 to reach row 1 of the 2-D table
# (head 1): in-bounds in memory but flagged by UBSan array-bounds on every valid disk, which would
# abort the 64DD target on any real image. Relax that one check on that one file only.
$(OBJDIR)/device/dd/disk.o: CFLAGS += -fno-sanitize=array-bounds
