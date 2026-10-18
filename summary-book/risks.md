<p align="right"><strong>English</strong> · <a href="risks.zh_CN.html">简体中文</a></p>

# Open items and risks

## P0: items that stop delivery

| # | Item | Next step | Source |
|---|---|---|---|
| 1 | Nobody ran a device test yet. | Do the Sprint 5 board smoke test. Then do Sprints 6 to 10. | `需求文件夹/08-sprint-roadmap.md`, sprint table |
| 2 | No phone app version supports protocol 0.2. | Sprints 6 to 8 need it. | Same document |
| 3 | D-20, D-31, and D-32 wait for the phone app team. | Review the latest document 07 with that team. | `需求文件夹/04-validation-and-decisions.md` |
| 4 | The HeygoAI coach capability is not known (Q-01). | Sprint 8 needs it. | `08-sprint-roadmap.md`, Sprint 8 row |

## P1

| # | Item | Source |
|---|---|---|
| 5 | Firmware work for the D-33 page rail has not started (Sprint 16). | `04-validation-and-decisions.md`, D-33 row |
| 6 | The source of measured turn tracks is not known (Q-02). | Same document, Q-02 row |
| 7 | CI possibly never ran. | [Checks and deployment](build-and-deploy.md) |

## Known defects

Source: `Firmware/docs/heygo-changes.md`, known issues.

- The battery label on the screen is always empty. The device sends the battery level to the phone in DEVICE_STATUS.
- A PLAY_START that arrives during a recording is lost. Then the device shows a timeout. D-20 tells the phone not to send it at that time.
- The body label shows four lines. Longer text ends with three dots. The voice plays the full answer.

## Accepted risks

Source: `04-validation-and-decisions.md`, risk table.

| Risk | Response |
|---|---|
| BLE has no encryption. People near the user can hear the audio (D-12). | Accepted. |
| No power management (D-18). | Accepted. |
| The fall popup closes after 10 s. The user possibly cannot push a key (D-31). | The phone controls the escalation. POPUP_END reason 0 tells the phone that nobody pushed a key. |
| Popup voice arrives before the old answer stops (D-31). | `audio_play_cut` stops the speaker. The link accepts AUDIO_DOWN only for the open stream. TC-22 checks this. |
| The phone sees a run start that is not correct (D-32). | The phone makes the decision. On the live data page, a short press of DOWN opens the report page. |
