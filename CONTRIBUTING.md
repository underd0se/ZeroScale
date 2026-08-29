# Contributing to ZeroScale C-TUI

We're thrilled that you're interested in contributing to ZeroScale! This document provides a set of guidelines for contributing to ZeroScale C-TUI.

ZeroScale is licensed under GPLv3 and is based on [TAILMON](https://github.com/ViktorJp/TAILMON) by Viktor Jaep.

****How Can I Contribute?****
Please make your modifications to the modular C source files in `src/`, compile and test locally using `make` or `./build-all.sh`, and submit a pull request against the `beta` branch.

------

**Reporting Bugs**
* Use a clear and descriptive title for the issue to identify the problem.
* Describe the exact steps which reproduce the problem in as much detail as possible.
* Provide specific examples, screenshots, or logs to demonstrate the issue.

**Suggesting Enhancements**
* Use a clear and descriptive title for the issue.
* Provide a step-by-step description of the suggested enhancement in as much detail as possible.

**Your First Code Contribution**
* Fork the repository.
* Create a new branch in your fork (`git checkout -b feature/my-new-feature`).
* Make your changes inside `src/` (or `include/`).
* Test compilation locally:
  * For local native build: `make`
  * For cross-compiling Asus router binaries (ARMv7 & ARM64): `./build-all.sh`
* Commit your changes using conventional commits (`git commit -m 'feat: add some feature'`).
* Push to your branch (`git push origin feature/my-new-feature`).
* Create a new Pull Request against `beta`.
