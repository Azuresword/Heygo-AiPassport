[简体中文](README.zh_CN.md) · **English**

# HeygoAI and AI Passport: requirements workspace

Version: 0.7. Date: 2026-10-17. Status: Draft for review.
This folder holds the product definition, the architecture, and the Scrum backlog.
It also holds the validation plan, the decision record, the Sprint 0 plan, and the BLE protocol draft.
The firmware code is in the `Firmware` folder of the repository, not in this folder.
No evidence shows a working HeygoAI integration.

## Read in this order

| Document | Purpose |
| --- | --- |
| [Product requirements](01-product-requirements.md) | Goal, priorities, 20 requirements, and acceptance targets. |
| [System architecture](02-architecture.md) | Components, modes, buttons, the Coach audio flow, and device limits. |
| [Scrum backlog](03-scrum-backlog.md) | User stories, priorities, Sprint order, and the Definition of Done. |
| [Validation and decisions](04-validation-and-decisions.md) | Test cases, open questions, the decision log, and risks. |
| [Decision amendments](05-decision-amendments.md) | The reasons behind D-08 to D-20. |
| [Sprint 0 plan](06-sprint-0-plan.md) | Four hours of host-only work for the first Coach question. Done. |
| [BLE protocol draft](07-ble-protocol-draft.md) | The device side of the BLE link. |

## First version

The skier stops for a rest break.
The skier holds OK and asks the AI coach a question.
The skier hears the answer.
The phone stays in a pocket. The skier keeps the gloves on.

D-17 and D-18 set the priorities:

- **P0, the first version:** device connection, the voice question, the spoken answer, and the Agent exchange. P0 also covers the modes, gloves and Chinese text, and recovery.
- **P1, the next increment:** reports, metric charts, the offline report cache, and human intercom.
- **P2, later candidates:** practice goals, quests, a reflection journal, session comparison, and report sharing.
- **Deferred:** turn visualization, privacy and authorization, and power management.

## Status and evidence

Sprint 0 ended. The Sprint 1 device code is in the repository. Device tests are NOT RUN. The team closed no story.
Section 4.1 of the [backlog](03-scrum-backlog.md) gives the progress of each story.

The user accepted D-08 to D-15 and D-17 to D-20. D-16 waits for the phone application team.
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
  07-ble-protocol-draft.md
```

Run the command inside this folder. Set `STE_SKILL` when the skill sits elsewhere.
The tool cannot prove that Chinese text meets the standard. It cannot prove that words match the official STE dictionary.
