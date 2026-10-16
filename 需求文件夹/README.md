[简体中文](README.zh_CN.md) · **English**

# HeygoAI and AI Passport: requirements workspace

Version: 0.1. Date: 2026-10-16. Status: Draft for review.
This folder contains the product definition, proposed architecture, Scrum backlog, and validation plan.
It contains no firmware implementation.
No evidence establishes a working HeygoAI integration.

## Read in this order

| Document | Purpose |
| --- | --- |
| [Product requirements](01-product-requirements.md) | User journey, MVP scope, 20 requirements, and acceptance targets. |
| [System architecture](02-architecture.md) | Components, buttons, report data, turn geometry, voice flows, and device limits. |
| [Scrum backlog](03-scrum-backlog.md) | User stories, four proposed Sprints, roles, events, and completion criteria. |
| [Validation and decisions](04-validation-and-decisions.md) | 20 test cases, assumptions, open questions, risks, and decision records. |

## Core experience

The skier reviews a report during a rest break.
The skier selects a metric or a turn.
The skier uses a button to ask the Agent a question.
The Agent gives a spoken answer.
The skier can then speak with a human coach or companion through a separate Team mode.

Reports, voice input, voice output, Agent interaction, human intercom, charts, and turn visualization form the proposed MVP.
Practice goals, quests, reflection notes, historical comparisons, and sharing remain later candidates.

## Status and evidence

The user requested the core features and Scrum development.
The architecture, priorities, four-Sprint forecast, timings, and capacity limits remain proposals.
No person accepted a role or a delivery date.

AI Passport hardware facts come from the existing local source repository.
The architecture lists those sources and distinguishes local changes from released capabilities.
HeygoAI interfaces, real ski data, the phone platform, and the transport remain open decisions.

## Terms

| Term | Meaning in this project |
| --- | --- |
| Agent | The AI service that answers a question using the selected report context. |
| ASR | Automatic speech recognition that converts speech into text. |
| TTS | Text-to-speech synthesis that produces spoken output. |
| PTT | Push-to-talk. The user holds a button to request permission to speak. That permission allows a short speech interval. |
| Turn | A segment of ski motion with declared timing, direction, and data source. |
| Schematic | An illustrative curve. It is not a measured ski track. |
| BSP | Board support package. It provides the public hardware interfaces. |
| MVP | The smallest complete product scope defined by the P0 requirements. |
| Sprint | A fixed Scrum interval that produces a usable product increment. |
| Definition of Done | The quality conditions that an increment must satisfy. |
| Lease | Temporary permission for one participant to speak in a room. |
| Idempotent request | A repeated request that does not repeat its completed effect. |
| API | Application programming interface. It defines how software components exchange requests and data. |
| ADC | Analog-to-digital converter. It converts a voltage into a digital value. |
| PCM | Pulse-code modulation. It represents sound as digital samples. |
| RAM | Memory that software uses while it runs. |
| PSRAM | Pseudo-static RAM. It is a type of additional working memory. |
| Flash | Memory that retains firmware and data without power. |
| IMU | Inertial measurement unit. It measures acceleration and rotation. |
| GNSS | Global navigation satellite system. A receiver uses satellite signals to estimate position. |
| BLE | Bluetooth Low Energy. It is a wireless communication technology. |
| TLS | Transport Layer Security. It protects data during a network connection. |
| UX | User experience. It describes how users interact with the product. |
| Data provenance | Information about the data source and processing history. |

## Document maintenance

Keep each English document aligned with its Chinese counterpart.
Keep REQ, US, and TC identifiers stable when the wording changes.
Record scope changes in the backlog and the decision log.
Do not silently remove a requested core feature.

Use the installed `asd-ste100` skill for each update.
Run its English structural checks.
Review terminology manually.
Keep every rule enabled.
Review each finding before changing the text.

Check that each rewrite preserves the meaning.
Check Chinese text manually.

```bash
python3 ~/.codex/skills/asd-ste100/scripts/ste-lint.py \
  README.md 01-product-requirements.md 02-architecture.md \
  03-scrum-backlog.md 04-validation-and-decisions.md
```

Run that command from this folder.
The arguments include only the English documents.
The automated tool cannot establish Chinese compliance or full official STE dictionary compliance.
