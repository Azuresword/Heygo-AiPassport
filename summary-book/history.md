<p align="right"><strong>English</strong> · <a href="history.zh_CN.html">简体中文</a></p>

# Project history

## Numbers

| Item | Value | Source |
|---|---|---|
| Commits on main | 55 | `git rev-list main --count` at `c865ba8` |
| Time range | 10-16 18:02 to 10-18 10:48 (+0800) | First commit `f431fe6`, last commit `c865ba8` |
| Authors | Dylan Li made 52 commits. A local account made 3. | `git log --format=%an` |
| Tags | None | `git tag` |

## Six phases

### 1. Requirements (10-16, evening)

`f431fe6` start → `c2dee22` shared scope → `6f5b6af` eight decisions → `970bb8e` BLE draft → `48926ff` smaller P0 → `cfa30fe` protocol values

### 2. Sprints 0 and 1: voice path (10-17, morning)

| Commit | Change |
|---|---|
| `a917aaa` | Add the firmware (406 files). The coach state machine runs on the device. |
| `941f4e7` | Connect voice and BLE. |
| `11b10cc`, `da3e5fa`, `f0cb302` | Fixes and logs. |
| `4ff7f13` | Heygo brand screens. |
| `344c8e5` | Fix audio mute and playback start. |

### 3. Increment 2: run data (10-17, midday to afternoon)

`86bd2fc` plan → `d371c1d` Sprint 2 → `0d0eaac`, `31e511c` Sprints 3 and 4 → `9238eaf` D-28 → `bed4b27`, `bedcf1a`, `c996e14`

### 4. Page loop and Turns page (10-17, early evening)

`13fe56f`, `7d90750`, `c121084` → `49573f9` D-30

### 5. Popups and run start (10-17, night)

| Commit | Change |
|---|---|
| `93b6498` | D-31 phone popups. |
| `816f043` | IQ range 0 to 200. |
| `421b15c` | Turns page rule from the phone app team. |
| `2234275` | Animated popup badges. |
| `5658911` | D-32 run start opens the live data page. |

### 6. Page rail (10-18, morning)

| Commit | Change |
|---|---|
| `eef9c65` to `9b46a71` | Documents 01 to 05 add REQ-24, US-24, Sprint 16, TC-24, and D-33. |
| `30ba3c7`, `258f6fa`, `6e5668f` | The simulator shows the rail. The self-test checks it. |
| `c865ba8` | New simulator preview image. |

The work from requirements to D-33 took approximately 41 hours.
