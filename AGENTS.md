# Agent rules for this X16 repo

Read these before any change, and follow them. They apply to Cursor and Grok Bot agents alike:

- `C:\source\System\global-agents\AGENTS.md` holds the global rules: working modes, docs/comms and the X16 "start here" section.
- `..\x16-lib-poc\docs\X16-PROJECT-START.md` covers the skeleton, build scripts (section 3), memory and the program cap (section 5), art and sound (section 6), tests and smoke (section 8), and the Cursor checklist (section 11).
- `..\x16-lib-poc\docs\X16-TOOLS-STANDARD.md` covers the tool project files: `x16project.json`, `assets/` and `.x16/exports.json`.
- Everything in `docs/comms/`, especially `open-notes.md`.

The rules agents most often miss:
1. `build-win.cmd` is the one build, and the build runs the pixel-cli and sound-cli exports. No generator scripts write into `boot/`.
2. The tool files are the source. Build from Robert's edits in Pixel X16, Sound X16 and Project X16, and never overwrite or regenerate them. If an edit looks wrong, note it in `open-notes.md` and ask.
3. x16-lib belongs to Project X16 Manager. To ask for a lib change, leave a note in `docs/comms`.

This file only points to the rules. Edit the docs above, not this file.
