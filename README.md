### Build

Setup vcpkg inside the build folder
```
cd path/to/clone
mkdir build
cd build
git clone --single-branch https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.bat
```

Now you can open the project with CLion and everything should be fine. **Or**:

You can generate the project with cmake presets `Debug` or `RelWithDebInfo` (_both require Ninja_)
```
cd path/to/clone
cmake --preset=RelWithDebInfo
```

And build with
```
cd path/to/clone
cmake --build --preset=RelWithDebInfo
```

To run the program, provide the path to the `projects/development` folder as the first and only argument so the assets are loaded.

GLTF assets are not added to git. I may prepare a GDrive link for the ones I'm using but actually any model is fine,
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