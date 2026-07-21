# Contributing to BedrockF3

Thanks for your interest in contributing. This document covers the
ground rules for working on the project. By participating you agree to
follow it.

## Reporting bugs

Open an issue using the **Bug report** template and include:

- Your Windows version (10 or 11, build number from `winver`)
- Your LeviLamina version (visible at the top of the LeviLamina console)
- Your Bedrock version (the exact patch number from the bottom-left of
  the title screen)
- The mod version you are running
- A minimal reproduction: what you did, what you expected, what happened
- The `logs/` directory produced by LeviLamina, if relevant

## Suggesting features

Open an issue using the **Feature request** template. Describe the
feature and the use case. A pull request that implements the feature is
welcome but please open the issue first so the design can be discussed.

## Code changes

1. **Fork the repository** and create a feature branch off `main`:
   ```bash
   git checkout -b feature/short-description
   ```
2. **Keep changes focused.** One logical change per pull request. Mixed
   changes make review harder and history messier.
3. **Match the existing style.** The project uses LLVM-style formatting
   enforced by `.clang-format`. The project is C++20 with C++23 features
   opt-in via the `_HAS_CXX23=1` macro.
4. **Build locally** before opening the PR:
   ```bash
   xmake f -p windows -a x64 -m release
   xmake
   ```
5. **Test in-game.** Hook offsets change between Bedrock versions. If
   you touch anything in `src/f3_debug/hooks/` or the rendering path,
   please confirm the mod still draws the overlay and the F3 toggle still
   works on a release build of Bedrock.
6. **Open a pull request** against `main`. Fill in the PR template. Link
   the relevant issue with `Closes #N` or `Refs #N`.
7. **Wait for CI.** The first thing a reviewer will check is whether
   the build still passes.

## Coding conventions

- C++20 baseline, with `_HAS_CXX23=1` to opt into C++23 features where
  they make the code clearer (`std::format`, `std::expected`,
  `std::move_only_function`, etc.).
- Prefer `std::` standard library types and functions over custom
  utilities or older C-style idioms.
- No raw `new` or `delete`. Use smart pointers and RAII.
- All public functions that return error information should return
  `std::expected<T, std::string>` rather than throwing.
- All getters should be `[[nodiscard]]`.
- Namespaces are lower_snake_case, types are UpperCamelCase, functions
  and variables are lower_snake_case. Constants are UPPER_SNAKE_CASE.
- Include order: paired header for the .cpp, then LeviLamina, then
  Bedrock SDK, then standard library, with a blank line between groups.

## License

By contributing, you agree that your contributions will be licensed
under the MIT License. See [LICENSE](LICENSE) for the full text.
