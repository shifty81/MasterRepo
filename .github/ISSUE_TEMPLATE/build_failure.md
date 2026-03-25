---
name: Build Failure / Hang Report
about: Report a build failure, hang, or crash with log output
title: "[Build] "
labels: bug, build
assignees: ''
---

## Description

<!-- Briefly describe the problem. What were you doing when the build failed or hung? -->

## Build Script

<!-- Which script were you running? (e.g., build_all.sh, build.sh, ci_pipeline.sh, bootstrap.sh) -->

- Script: `Scripts/Tools/build_all.sh`
- Command: `bash Scripts/Tools/build_all.sh`

## Environment

- **OS:** <!-- e.g., Windows 11, Windows 10, Linux -->
- **Shell:** <!-- e.g., Git Bash, MSYS2, PowerShell, WSL -->
- **Compiler:** <!-- e.g., MSVC 2022, MinGW, Clang -->
- **CMake version:** <!-- `cmake --version` -->

## Log Output

<!-- Paste the relevant log output below, or attach the log file.
     Log files are saved to: Logs/Build/
     
     You can also auto-generate an issue with:
       bash Scripts/Tools/submit_issue.sh --log Logs/Build/<your-log>.log
-->

<details>
<summary>Build Log (click to expand)</summary>

```
PASTE LOG HERE
```

</details>

## Hang Report (if applicable)

<!-- If the build hung, a watchdog report may have been generated at:
     Logs/Build/hang_report_<timestamp>.md
     
     You can submit it with:
       bash Scripts/Tools/submit_issue.sh --from-hang-report Logs/Build/hang_report_<timestamp>.md
-->

<details>
<summary>Hang Report (click to expand)</summary>

```
PASTE HANG REPORT HERE (if applicable)
```

</details>

## Steps to Reproduce

1. Clone the repository
2. Run `bash Scripts/Tools/build_all.sh`
3. <!-- What happens? -->

## Expected Behavior

<!-- What should happen? -->

## Additional Context

<!-- Any other information that might help debug the issue. -->
