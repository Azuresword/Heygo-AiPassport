[简体中文](README.zh_CN.md) · **English**

# HeygoAI and AI Passport: requirements workspace

Version: 0.8. Date: 2026-10-17. Status: Draft for review.
This folder holds the product definition, the architecture, and the Scrum backlog.
It also holds the validation plan, the decision record, the Sprint 0 plan, the BLE protocol draft, and the four-hour Sprint roadmap.
The firmware code is in the `Firmware` folder of the repository, not in this folder.
No evidence shows a working HeygoAI integration.

## Read in this order

| Document | Purpose |
| --- | --- |
| [Product requirements](01-product-requirements.md) | Goal, priorities, 21 requirements, and acceptance targets. |
| [System architecture](02-architecture.md) | Components, pages, buttons, the state diagram, page layouts, the Coach audio flow, and device limits. |
| [Scrum backlog](03-scrum-backlog.md) | User stories, priorities, Sprint order, and the Definition of Done. |
| [Validation and decisions](04-validation-and-decisions.md) | Test cases, open questions, the decision log, and risks. |
| [Decision amendments](05-decision-amendments.md) | The reasons behind D-08 to D-29. |
| [Sprint 0 plan](06-sprint-0-plan.md) | Four hours of host-only work for the first Coach question. Done. |
| [BLE protocol draft](07-ble-protocol-draft.md) | The device side of the BLE link, with the live data message. |
| [Four-hour Sprint roadmap](08-sprint-roadmap.md) | The goal, tasks, inputs, and cut line of Sprints 0 to 10. |

## First version

Two increments form the first version:

- **Increment 1, the Coach voice chain.** An OK hold on any page opens the Coach page. Another OK hold asks the AI coach a question, and the skier hears the answer.
- **Increment 2, live data and reports.** During a run, the live data page shows the speed, vertical drop, distance, turn count, and run time. After each run, the device shows the report of that run: distance, turn count, Heygo IQ, the core problem, and a short tip.

The phone stays in a pocket. The skier keeps the gloves on. The phone calculates all data. The device only shows it.

D-17, D-18, and D-22 set the priorities:

- **P0, Increment 1:** device connection, the voice question, the spoken answer, and the Agent exchange. P0 also covers page navigation, gloves and Chinese text, and recovery.
- **P1, Increment 2:** data sync, live data on the live data page, and the Report page.
- **P1, Increment 3:** the Turns page (D-30).
- **P1, Increment 4:** phone popups: a fall alert, a voice message, a problem report, or an achievement (D-31).
- **P1, after Increment 2:** metric charts, the offline report cache, and human intercom.
- **P2, later candidates:** practice goals, quests, a reflection journal, session comparison, and report sharing.
- **Deferred:** privacy and authorization, and power management.

## Status and evidence

Sprint 0 ended. The Sprint 1 device code is in the repository. Device tests are NOT RUN. The team closed no story.
Section 4.1 of the [backlog](03-scrum-backlog.md) gives the progress of each story.

The user accepted D-08 to D-15 and D-17 to D-31. The phone application team accepted protocol 0.2 (D-16). D-29 changes the meaning of two protocol fields, so that team must accept it again. The team must also accept the three turn messages of D-30 and the two popup messages of D-31.
A phone application already exists. Another team develops it.
No person accepted a Scrum role or a delivery date.

AI Passport hardware facts come from the local firmware baseline.
HeygoAI interfaces, the review of the BLE protocol, and the phone platform remain open.
Commit `970bb8e` holds version 0.3, which still described reports, turns, and intercom in detail.

## Terms

| Term | Meaning in this project |
| --- | --- |
| Agent | The AI service that answers the question. |
| ASR | Automatic speech recognition. It turns speech into text. |
| TTS | Text to speech. It turns text into speech. |
| PTT | Push to talk. The user holds a button to speak to other people. |
| BSP | Board support package. It gives the public hardware interface. |
| MVP | The smallest complete product. The P0 requirements define it. |
| P0, P1, P2 | Priority levels. P0 is the first version. P1 is the next increment. P2 holds later candidates. |
| Deferred | A requirement that stays in the record but sits in no planned increment. |
| Sprint | A fixed Scrum time box that produces a usable increment. |
| Definition of Done | The quality rules that an increment must meet. |
| API | Application programming interface. It defines how software parts exchange data. |
| ADC | Analog-to-digital converter. |
| PCM | Pulse-code modulation. It stores sound as numeric samples. |
| IMA ADPCM | A simple audio compression format. It stores each sample in 4 bits. |
| RAM | The memory that software uses while it runs. |
| PSRAM | Extra external RAM. This board has none. |
| Flash | Memory that keeps firmware and data without power. |
| IMU | Inertial measurement unit. It measures acceleration and rotation. |
| GNSS | Satellite navigation. A receiver estimates position from satellite signals. |
| BLE | Bluetooth Low Energy. |
| GATT | The BLE rule set for services and characteristics. |
| ATT MTU | The largest BLE attribute packet that both sides accept. |

## Maintenance

Keep each English document and its Chinese pair consistent.
Keep the REQ, US, and TC numbers stable.
Update the backlog and the decision log when the scope changes.

Use the installed `asd-ste100` skill for each update.
Run the structure check on the English documents.
Check the terms, the meaning, and the Chinese text by hand.

```bash
STE_SKILL="${STE_SKILL:-$HOME/.claude/skills/asd-ste100}"
python3 "${STE_SKILL}/scripts/ste-lint.py" \
  README.md 01-product-requirements.md 02-architecture.md \
  03-scrum-backlog.md 04-validation-and-decisions.md \
  05-decision-amendments.md 06-sprint-0-plan.md \
  07-ble-protocol-draft.md 08-sprint-roadmap.md
```

Run the command inside this folder. Set `STE_SKILL` when the skill sits elsewhere.
The tool cannot prove that Chinese text meets the standard. It cannot prove that words match the official STE dictionary.
