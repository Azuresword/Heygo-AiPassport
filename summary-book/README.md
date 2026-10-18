<p align="right"><strong>English</strong> · <a href="README.zh_CN.html">简体中文</a></p>

# Heygo AI Passport project summary

> Snapshot: 2026-10-18, `main` commit `c865ba8`.
> Each statement shows its source: a file path and line, or a git commit.

## Summary

AI Passport is a small handheld device for skiers.
During a rest, the user can see the data of the last run and ask the AI coach.
The user does not take out the phone or remove the gloves.
The code for all four increments is complete, and the host tests pass.
**Nobody ran a device test yet.**

## Status

| Increment | Function | Status |
|---|---|---|
| 1 Coach voice | Long press OK opens the Coach page. Hold OK to ask a question. The device plays the answer. | Code complete. Device test not run. |
| 2 Run data and report | Speed, drop, distance, turns, and time. Heygo IQ (0 to 200) for each run. The device opens the live data page at run start (D-32). | Code complete. Device test not run. |
| 3 Turns | The line of each run, 8 turns on each page. | Code complete. Device test not run. |
| 4 Phone popups (D-31) | Fall alert, voice notice, issue report, or achievement report. A 10 s countdown. | Code complete. Host tests pass. Device test not run. |
| D-33 Page rail | A rail on the right side shows the position in the page loop. | The simulator shows the rail. Firmware work has not started. |

Sources: `README.md`, section "What the device does". `需求文件夹/04-validation-and-decisions.md`, D-33 row.

## Top three items

1. **Use a development board.** Sprint 5 is the first sprint that needs a board. All later integration sprints need it (`需求文件夹/08-sprint-roadmap.md`).
2. **Get the phone app team to confirm D-20, D-31, and D-32.** These three decisions wait for that team.
3. **Get the HeygoAI coach capability (Q-01).** Sprint 8 needs it. There is no answer yet.

For more items, see [Open items and risks](risks.md).

## Links

- Simulator: <https://azuresword.github.io/Heygo-AiPassport/>
- Repository: <https://github.com/Azuresword/Heygo-AiPassport>
- Book source: `summary-book/`. Run `summary-book/build.sh` to make `summary/` again.
