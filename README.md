# BedrockF3

```
   ___    _____    ___   ____  _   _
  | __ ) |___ /   / _ \ |  _ \| \ | |   The F3 debug screen,
  |  _ \   |_ \  | | | || |_) |  \| |   for Minecraft Bedrock.
  | |_) | ___) | | |_| ||  _ <| |\  |   A native C++ mod for the
  |____/ |____/   \___/ |_| \_\_| \_|   LeviLamina mod loader.
```

Mojang ships the F3 debug screen in Java Edition. Bedrock has never
had it. The community workarounds all run on the official add-on
system, which means they cannot read engine state: no real FPS, no
live frame time, no pitch. BedrockF3 hooks the engine directly through
LeviLamina's event API, so the values update every frame and reflect
the actual game state.

## Quick start

From your Bedrock install directory, the one LeviLauncher manages:

## Install

Drag-and-drop is the path most people will use. Go to the
[Releases page](https://github.com/CormacZ/BedrockF3/releases),
download the latest `BedrockF3-client-windows-x64.zip`, and drop it
into the mods page in LeviLauncher. That is the whole install.

If you prefer the package manager, run this from your Bedrock install
directory (the one LeviLauncher manages):

```
lip install github.com/CormacZ/BedrockF3#client
```

Or, for development, the GitHub Actions `build` workflow produces the
same zip as an artifact on every push to `main` and every pull
request. Open the run you care about and download
`BedrockF3-client-windows-x64.zip` from the artifacts section.

Either way, restart Minecraft after install. Press F3 in-game to
show or hide the panel.

## What it shows

| Line      | Where it comes from                              |
| --------- | ------------------------------------------------ |
| FPS       | 1-second moving average of frame deltas          |
| Frame     | Last frame's wall-clock time, in milliseconds    |
| Uptime    | Since the overlay was loaded                     |
| XYZ       | Sub-block position from the local player          |
| Chunk     | Coordinates plus position inside the chunk        |
| Facing    | Cardinal direction plus yaw, in degrees          |
| Pitch     | Look angle, in degrees                           |
| Dimension | Overworld, Nether, or The End                    |

## Configuration

A `config.json` is written next to the mod DLL on first launch.

| Key              | Default | Notes                                          |
| ---------------- | ------- | ---------------------------------------------- |
| visibleOnStartup | true    | Show the overlay immediately on load           |
| toggleKey        | "F3"    | Rebindable from the in-game options menu       |
| panelX, panelY   | 4, 4    | Position of the panel, in pixels               |
| textScale        | 1.0     | 1.0 matches the Minecraft default              |
| backgroundAlpha  | 0.55    | 0.0 transparent, 1.0 opaque                    |
| padding          | 6       | Pixels between the panel border and the text   |

The F3 toggle is also exposed as a rebindable key in the in-game
options menu, alongside the vanilla bindings.

## Build from source

You need xmake 3.0 or newer, Visual Studio 2022 with the C++ desktop
workload, Git, and a working LeviLamina client install on PATH so
`prelink.exe` and `bedrock_runtime_data` are reachable.

```
git clone https://github.com/CormacZ/BedrockF3
cd BedrockF3
xmake f -p windows -a x64 -m release
xmake
```

The build output lands in `bin/BedrockF3/`, with `manifest.json`
generated automatically from the latest git tag.

To install the build into an existing Bedrock install:

```
xmake install -o "D:\path\to\bedrock-install\mods"
```

## Architecture

```
src/f3_debug/
  F3Debug.h / .cpp    entry point, owns the event listener and the key handle
  events/             subscribes to LeviLamina's AfterUIRenderEvent on the event bus
  input/              registers the F3 key with LeviLamina's KeyRegistry
  overlay/            builds the F3 lines and draws onto the render context
  config/             JSON config load and save
  util/               FpsCounter, Uptime, CardinalDirection
```

The overlay is reached entirely through LeviLamina's public event API.
The mod never patches or hooks the closed-source game binary, so a
Bedrock update only needs a new LeviLamina release, not a mod update.

## Compatibility

| | |
| --- | --- |
| Mod loader | LeviLamina 26.20 or newer |
| Minecraft  | 1.21.x release channel    |
| Platform   | Windows 10 or 11 x64      |

## License

MIT. See [LICENSE](LICENSE) for the full text.

## Acknowledgements

- The [LeviLamina](https://github.com/LiteLDev/LeviLamina) team for the
  mod loader and the client-side event API that makes this possible
  without touching the closed-source game binary.
- [MiracleForest's iInfiniteNightVision](https://github.com/MiracleForest/iInfiniteNightVision)
  for the minimal client-side LeviLamina mod that served as the
  structural reference for the build and packaging setup.

## Contributing

Bug reports and feature requests go on the
[issue tracker](https://github.com/CormacZ/BedrockF3/issues). For code
changes, open a pull request on a feature branch and describe what
you changed and why. See [CONTRIBUTING.md](CONTRIBUTING.md) for the
full guidelines.
