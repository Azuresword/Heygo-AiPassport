<p class="lang"><strong>English</strong> · <a href="scrum.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Scrum</p>

# Delivery with Scrum

This project uses Scrum with a 4-hour sprint. Each sprint has one goal. Evidence, not opinion, shows if the team completed the work.

This chapter describes the Scrum flow as the project used it. Each step gives its source in the repository.

<div class="tiles">
<div class="tile accent"><div class="k">Sprints</div><div class="v">18</div></div>
<div class="tile"><div class="k">Each sprint</div><div class="v">4<small>h</small></div></div>
<div class="tile"><div class="k">User stories</div><div class="v">25</div></div>
<div class="tile"><div class="k">Requirements</div><div class="v">25</div></div>
<div class="tile"><div class="k">Decisions</div><div class="v">34</div></div>
</div>

## 1. Overview

<figure>
<img src="images/scrum-en.svg" alt="The Scrum flow of this project">
<figcaption>Figure 13. The Scrum flow. The top row is the delivery path. The bottom row is the change loop. Sources: <code>需求文件夹/03-scrum-backlog.md</code> and <code>08-sprint-roadmap.md</code>.</figcaption>
</figure>

The flow has two paths:

1. **Delivery path.** The product goal sets the backlog. Items from the backlog go into sprints. Each sprint delivers one increment and one evidence report.
2. **Change loop.** Feedback from the user becomes a decision (D-xx). Then the team changes the requirement documents and orders the backlog again.

## 2. Product goal and backlog

### Product goal

> During a run, the skier sees live data on the device. During a rest, the skier asks the AI coach a question by voice and hears the answer. The phone stays in a pocket, and the gloves stay on.

Source: `03-scrum-backlog.md`, section 1

### Four levels of trace

You can trace each function from the requirement to the sprint. Four documents keep the four kinds of ID:

| Level | ID | Document | Example: page rail |
|---|---|---|---|
| Requirement | REQ-xx | `01-product-requirements` | REQ-24 shows the position of the user in the page loop |
| User story | US-xx | `03-scrum-backlog` | US-24 page rail, 2 to 3 points |
| Test case | TC-xx | `04-validation-and-decisions` | TC-24: 40 page changes, the thumb stops in 200 ms each time |
| Sprint | 0 to 17 | `08-sprint-roadmap` and `03` | Sprint 16: simulator, firmware, and acceptance |

The result: when the team reviews an increment, it can find the related requirement and the test case at once.

### Use of story points

- Story points compare uncertainty and effort. They are not hours.
- The team has no velocity data. Thus, the plan does not assume a velocity.
- After the P0 scope became smaller in version 0.4, five P0 stories show "to estimate again".

Source: `03-scrum-backlog.md`, sections 2 and 4

## 3. The 4-hour sprint

### Fixed structure

<div class="figure-panel">
<div class="timebox"><div class="f15">Plan 15</div><div class="f200 dev">Develop 200 min</div><div class="f15">Review 15</div><div class="f10">Retro</div></div>
</div>

| Event | Time | Output |
|---|---|---|
| Sprint planning | 15 min | The team agrees the sprint goal and the tasks. It examines the prerequisites. |
| Development | 200 min | A usable increment that passes the checks of the sprint. |
| Sprint review | 15 min | The team shows the increment and reports the evidence in a fixed format. |
| Retrospective | 10 min | The team records one improvement for the next sprint. |

### Four rules

1. If a prerequisite is not met, the sprint does not start. The team does the next sprint that has its prerequisites.
2. If development takes too long, the team drops tasks at the cut line. Dropped tasks go back to the backlog.
3. If the team does not reach the sprint goal, the product owner decides the next step. The team does not extend the sprint.
4. At the end of each sprint, `./tools/validate.sh` must exit with code 0.

Source: `08-sprint-roadmap.md`, section 1

### Example: Sprint 0

| Order | Task | Time box | Check |
|---|---|---|---|
| 1 | T-1 mode and audio state machine | 60 min | One host test for each transition |
| 2 | T-2 packet layer for P0 messages | 75 min | Host tests for packing, unpacking, `xid`, and error codes |
| 3 | T-3 IMA ADPCM codec | 60 min | Host tests for block size, round trip, lost blocks, and full-scale input |
| 4 | T-4 gate and build | 45 min | Gate and build pass. The report gives the static RAM. |

**Cut line:** if T-1 and T-2 together take more than 150 minutes, drop T-3. Always do T-4.

**Why T-3 can go:** protocol document 07 has a raw PCM fallback. Without ADPCM, the team can still test the link.

**Result:** commit `a917aaa` delivered all four tasks. The team did not use the cut line. The definition of done closes no story in Sprint 0. Each P0 story still needs a device test and the real phone app.

Source: `06-sprint-0-plan.md`, sections 2, 4, and 7

## 4. Definition of done and evidence

### Definition of done

Each increment slice must meet these checks, where they apply:

- The user completes the task on a physical AI Passport.
- The path uses the real phone app, if the slice needs it.
- Host tests, repository checks, and the firmware build all pass.
- Device tests pass under recorded conditions.
- Chinese text, key actions, mode changes, and cancel behavior pass their tests.
- Latency is within the limit of the slice.
- The English and Chinese documents describe the same behavior.
- The team records each remaining gap.

**Important limit:** a simulator or a stub cannot replace a device test or a test with the real service.

Source: `03-scrum-backlog.md`, section 9

### Evidence format

Each sprint review uses this format:

```text
Sprint: N
Sprint Goal: met / not met
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Device tests: PASS / FAIL / PARTIAL / NOT RUN (real phone app or phone_sim)
Unverified: each item that nobody verified
```

Source: `08-sprint-roadmap.md`, section 5

The format keeps "build passed", "host tests passed", and "device tests passed" in three different lines. The Sprint 0 plan names this risk: a person reads a host-only result as a product result, and the status report becomes wrong. The evidence format is the response to that risk (`06-sprint-0-plan.md`, section 6).

### Test conditions

| Condition | Content |
|---|---|
| N1 indoor reference link | One AI Passport, one phone, and the real phone app. The team measures round trip, jitter, and loss. It uses the 95th percentile. |
| N2 faults | The team tests each fault alone. The faults are a 30 s link break, the app in the background, and a permission failure. Two more are an MTU below 172 and a 5-minute clock offset. |
| U1 usability | Five adults do one task script with bare hands and with gloves. Nobody helps them during the attempts. |

Source: `04-validation-and-decisions.md`, section 2

## 5. Code sprints and acceptance sprints

Each story has two sprints. One sprint makes the code. Another sprint does the acceptance.

| Story | Code sprint | Acceptance sprint |
|---|---|---|
| US-06 voice capture, US-07 voice playback | 0–1 | 6 |
| US-08 key-driven agent talk | 1 | 8 |
| US-15 gloves and Chinese text | 1 | 9 |
| US-02 data sync | 2 | 7 |
| US-21 live data page | 3 | 7, 9 |
| US-03 report for each run | 4 | 7 |
| US-05 turns | 11 | 12 |
| US-22 phone popups | 13 | 14 |

Source: `03-scrum-backlog.md`, section 4

The reasons for this split:

1. The board was not available at first. A code sprint needs only the host, so it does not wait for hardware.
2. An acceptance sprint has its own prerequisites: a board, a phone app that supports protocol 0.2, and five testers. If one is missing, the acceptance sprint does not start.
3. The team records code and acceptance separately, so the two statuses do not mix. "Code complete" and "acceptance passed" are different statements.

## 6. How a change enters a sprint

### Four-step change flow

1. Record the feedback and the affected requirements.
2. The product owner orders the backlog again.
3. The developers update the estimates.
4. Record each accepted change in the decision log.

Source: `03-scrum-backlog.md`, section 10

All 34 decisions entered the project through this flow. Later, other decisions replaced 6 of them. One decision, D-29 (four pages in a loop), replaced 5. Document `05-decision-amendments` gives the reasons.

### Example: the order of D-33 and D-34

<figure>
<img src="images/change-en.svg" alt="Timeline of D-33 and D-34">
<figcaption>Figure 14. How a decision becomes firmware. An empty circle is a change after a user review. Source: <code>git log</code>.</figcaption>
</figure>

Both decisions changed the documents first, then the code, and then the screen images:

1. **Documents first.** The team updated documents 01 to 05 in English and Chinese pairs.
2. **Code next.** For D-33, the simulator came first (10:47), and the firmware came after the user review (11:49). For D-34, the flow graph, firmware, and simulator came in one set of commits (11:22).
3. **User reviews at any step.** D-33 had two review rounds. The first round (11:39) removed the arrows, before the firmware. The second round (12:05) made all stops one color, after the firmware. It changed only `page_rail.c` (`ea4815a`).
4. **Screen images last.** `host_render` drew the 25 screens again, so the images in the documents agree with the firmware.

| Decision | Documents | Firmware | Duration |
|---|---|---|---|
| D-33 page rail | 10:45 | 11:49 | 64 min, with one review round |
| D-34 link state | 11:08 | 11:22 | 14 min |

Each duration is the time between the first and the last commit. It is not a measured work time.

## 7. Engineering methods that support Scrum

Scrum sets the rhythm. These engineering methods make a 4-hour sprint possible:

<div class="cards">
<div class="card"><span class="tag">One source</span><h4>One flow graph</h4><p><code>coach.flow.json</code> makes the firmware transition table and the browser simulator. One change updates both.</p></div>
<div class="card"><span class="tag">Review</span><h4>Online simulator</h4><p>GitHub Pages publishes the simulator. To review an increment, open the page and push the keys. No board is necessary.</p></div>
<div class="card"><span class="tag">Done</span><h4>Automatic gate</h4><p><code>validate.sh</code> runs host tests, glyph checks, screen checks, the simulator self-test, and the firmware build. It does the automatic part of the definition of done.</p></div>
<div class="card"><span class="tag">Increment</span><h4>One branch for each increment</h4><p><code>feat/report-live-data</code>, <code>feat/turn-view</code>, <code>feat/app-popups</code>, and <code>feat/run-start</code>. Each branch goes into <code>main</code> when its increment is complete.</p></div>
<div class="card"><span class="tag">Documents</span><h4>English and Chinese, with asd-ste100</h4><p>D-06 requires a Chinese copy of each English document. <code>ste-lint.py</code> checks the English text. The phone app team reads the same protocol document.</p></div>
<div class="card"><span class="tag">Tests</span><h4>Plain C host tests</h4><p>The state machine, protocol, and ADPCM are plain C. 33 host tests check the logic without hardware.</p></div>
</div>

### The simulator grows with the sprints

Each sprint that changes the screens has a task "update the flow simulator". Thus, the number of self-test checks increases with each increment:

{% include "./_partials/selftest-en.html" %}

Sources: sprint results in `08-sprint-roadmap.md`, sections 3 and 4. `Firmware/docs/heygo-changes.md`, sections 12 and 13. Measurements for this book at `c865ba8` and `284ab00`.

## 8. Roles

| Role | Person in the documents | Actual situation |
|---|---|---|
| Product owner | Not assigned | The user made all scope decisions, D-17 included |
| Scrum Master | Not assigned | — |
| Developers | Not assigned | One firmware developer. The phone app team works in parallel. |

Source: `03-scrum-backlog.md`, sections 2 and 3

## 9. Lessons: what worked and what to improve

### What worked

- **A 4-hour time box with a cut line.** If time runs short, the team drops a task. It does not extend the sprint. Sprint 0 and Sprint 2 each had a cut line. Both results say "the team did not use the cut line".
- **Separate evidence lines.** Build, host, device, and unverified items have different lines. Nobody can read a host result as a product result.
- **A decision log.** Each of the 34 decisions has an ID, a status, and a reason. The log also shows which decision replaced which.
- **Simulator before firmware.** The user first reviews the appearance in the browser. The first D-33 change (no arrows) came before the firmware, so the firmware did not need rework for that round.

### What to improve

| Gap | Evidence | Recommendation |
|---|---|---|
| No record of the actual time | The results of Sprints 0, 2, 3, 4, 11, 13, and 15 say that nobody recorded the actual time | Record the start and the end of each sprint. Collect velocity data. |
| No written retrospective items | The documents contain no retrospective record | Write one improvement after each retrospective. Add it to the next sprint plan. |
| Roadmap status not current | `08-sprint-roadmap` still shows "not started" for Sprints 5 to 15 | Update the status column after each sprint review. |
| No assigned roles | Section 3 of `03` shows "not assigned" for all three roles | Name a product owner. Then one person owns the order and the acceptance. |
| Story points to estimate again | Five P0 stories show "to estimate again" | Estimate them in the next planning event. |
