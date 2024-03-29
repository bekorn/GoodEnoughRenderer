![ScreenShot](https://drive.google.com/uc?export=view&id=1P4RQFhDjnaFaes-Lgcotvu9DJII12MQI)

### Features

- GLTF model, material, scene loading
- JPEG, PNG, HDRI loading
  - Supports 1:6, 6:1 cubemap layouts
- GLSL shader loading via json configuration
  - Interface inspection
  - Dynamic reloading

- Physically Based Rendering
  - Single-scattering isotropic BRDF: Cook-Torrance
    - F: Schlick, D: Trowbridge-Reitz GGX, G: Schlick GGX
  - Image Based Lighting
    - Convert a 2:1 HDRI to an Environment Map
  - Point lights

- General metrics and custom GPU time queries
- Material editor
- Scene Tree


See [vcpkg.json](./vcpkg.json) for the dependencies.

----


### How to

<details><summary>Build</summary>

Prefer cloning the repository with `--single-branch` to reduce size (some branches have assets).

Setup vcpkg inside the build folder
```
cd path/to/clone
mkdir build
cd build
git clone --single-branch https://github.com/Microsoft/vcpkg.git
```

Now you can open the project with CLion and everything should be fine.
**Or**, you can generate the project with cmake presets `Debug` or `RelWithDebInfo` (_both require Ninja_)
```
cd path/to/clone
cmake --preset=Debug
```
> **Note**
I only use the Debug profile and run with the debugger because I disabled many features so that it is _almost_ the same performance.
Main advantages of Debug profile are: code is not optimized away (line by line debugging), asserts are caught by the debugger (easily investigable).

> **Warning**
Currently only x64 architecture is supported (because its the only one I have access to).
To build with MSVC, run the commands in a `x64 Native Tools Command Prompt`.

And build with
```
cd path/to/clone
cmake --build --preset=Debug
```

</details>


<details><summary>Run</summary>

To run the Laboratory app, provide the paths to the `editor_assets` and `apps/laboratory/assets` folders as the command line arguments.
(I prefer absolute paths to prevent any _current working directory_ differences between compilers).

GLTF assets are excluded from git because of their size (except `editor_assets/models`). To load your own, edit the `apps/laboratory/assets/assets.json`.
HDRIs and Envmaps are also excluded for the same reason. You can load an HDRI and use the Envmap Baker in the editor.

You can download the assets I used from [here](https://drive.google.com/drive/folders/1LU5GHVmfd7sNY9ECnvMjL3NwSL50dnun).

</details>

----


### What to read next

I did not work in an organized manner before (besides my Notion).
However, it's getting more difficult with each feature so I made some changes recently:
- Each feature will have its own issue and branch
- Each goal/version will have its own project that contains the necessary features to accomplish it

Goals are planned as little projects on their own, with challenging requirements that are not possible with the current version.
Here is the [active goal](https://github.com/bekorn/GoodEnoughRenderer/issues/2).
