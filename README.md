### Build

Setup vcpkg inside the build folder
```
cd path/to/clone
mkdir build
cd build
git clone --single-branch https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.bat
```

Now you can open the project with CLion and everything should be fine.
**Or**, you can generate the project with cmake presets `Debug` or `RelWithDebInfo` (_both require Ninja_)
```
cd path/to/clone
cmake --preset=RelWithDebInfo
```

And build with
```
cd path/to/clone
cmake --build --preset=RelWithDebInfo
```

To run the program, provide the paths to `projects/editor` and `projects/development` folders as the command line arguments.
(I prefer absolute paths to prevent any _current working directory_ differences between compilers) 

GLTF assets are not added to git (except `projects/editor/models`). I may prepare a GDrive link for the ones I'm using but actually any model is fine,
just edit the `projects/development/assets.json`.

---

### TODO

Github
- Screenshots

Editor
- Camera
- Mouse input
- Visualizing geometry attributes
- An outline shader to indicate selection
- Object picking with mouse
- Transform gizmos

Rendering
- GLTF materials
- Deferred rendering
- Instanced rendering
    - Particles
    - Foliage
- Z-up right-handed coordinate system

Core
- Entity Component System (or some equilevent)
- Logging
- Profiling (flame chart)
- Compute shaders
- Project restructure