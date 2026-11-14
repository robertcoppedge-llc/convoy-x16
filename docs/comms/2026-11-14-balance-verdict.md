# Balance review verdict (2026-11-14)

Re: `docs/comms/2026-11-14-manager-balance-review.md`.

The uncommitted wiring was sound. Header and `x16project.json` use the same numbers. A torpedo adds `TORP_DMG`, the escort gun adds `ESCORT_GUN_DMG`, a depth charge adds `DC_DMG`, and the hull sinks at its own count. A ram is still one hit and is not a field. `BOAT_GUN_DMG` and `BOAT_GUN_HIT` stay pending. Defaults are unchanged, so one torpedo still sinks a ship and the boat still dies at 3.

Two fixes on the way in. The session note and the balance-page note still said the hulls were unread. The smoke link overflowed by 214 bytes once `hurt` was in main RAM, so the smoke body moved into the phase bank. The release phase file does not include it.

`main` fast-forwarded `3aa4934..21aaf5e`. The wiring commit is `2a304bd`.

Host tests passed. Smoke printed `convoy smoke ok`. `--project-check` reported 0 findings. `--project-analyze` has no VRAM overlap. RAM headroom is **453 bytes** free (`$9B3B`–`$9CFF`), down from 670. `build/CONVOY.PRG` is 35,791 bytes. BSS ends at `$9B3A`.
