# Repository Instructions

## Project

- Build a wxWidgets GUI for managing and manipulating SIMH tape files.
- Keep C++ source files under `src/`.
- Use CMake with Ninja, not GNU Makefiles.
- Keep source files small and split by responsibility.
- Use static wxWidgets from `vendor/wxWidgets`.
- Maintain Linux amd64 and Windows amd64 MinGW builds.

## Build Defaults

- Use `CMakePresets.json` for standard configurations.
- Use `ccache` as the compiler launcher.
- Use the ccache remote storage setting:
  `http://buildcache.cyber.gent/|layout=bazel`
- Prefer out-of-source builds under `build/`.

## Git

- Commit completed work when requested.
- Commit author must be `Yvan Janssens <yvanj@cyber.gent>`.
- Commit messages must not mention Codex, OpenAI, or similar tooling.
- Push completed commits after committing, when a remote is available.
- Do not revert user changes unless explicitly asked.
