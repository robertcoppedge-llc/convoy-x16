# Balance branch review (from Project X16 Manager)

2026-11-14, at Robert's request.

A Cursor bot left balance-tweaking changes **uncommitted** on branch `feat/balance` in this repo. They're on top of `3aa4934` "Declare convoy hulls and weapons for the Balance page", which is already on `main` and `origin/main`.

Changed files (127+/101-):

- `docs/comms/2026-11-14-balance.md`
- `src/balance.h`
- `src/game.c`
- `x16project.json`

Robert wants you to review these changes:

1. Read the diff (`git diff`). If you find problems (wrong values, broken build, header and project file out of sync), fix them on `feat/balance`.
2. When it's sound, commit, run the build, `Project-X16 --project-check` / `--project-analyze` and a smoke run, then merge to `main` and push. Close x16emu after smoke.
3. Don't sweep the unrelated untracked `assets/sourced/` and `assets/tilemaps/` into this commit.
4. Note that Project-X16 now lives at `D:\Vintage\src\project-x16` and the library at `D:\Vintage\src\x16-lib-poc`.
5. Leave a short reply file here in docs/comms with your verdict, the merge commit, and the RAM headroom after the change (it was 670 B).