![ScreenShot](https://drive.google.com/uc?export=view&id=1P4RQFhDjnaFaes-Lgcotvu9DJII12MQI)


### How to Build

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
cmake --preset=Debug
```
> I only use the Debug profile and run with the debugger because I disabled many features so that it is _almost_ the same performance.
Main advantages of Debug profile are: code is not optimized away (line by line debugging), asserts are caught by the debugger (easily investigable).


And build with
```
cd path/to/clone
cmake --build --preset=RelWithDebInfo
```

To run the program, provide paths to the `projects/editor` and `projects/development` folders as the command line arguments.
(I prefer absolute paths to prevent any _current working directory_ differences between compilers) 

GLTF assets are excluded from git because of their size (except `projects/editor/models`). To load your own, edit the `projects/development/assets.json`.
HDRIs and Envmaps are also excluded (‼[^-1]) for the same reason. You can load an HDRI and use the Envmap Baker in the editor.

[^-1]: fix(bekorn): this causes editor to crash if no envmap is loaded


### What to read next

I did not work in an organized manner before (besides my Notion).
However, it's getting more difficult with each feature so I made some changes recently:
- Each feature will have its own issue and branch
- Each goal/version will have its own project that contains the necessary features to accomplish it

Goals are planned as little projects on their own, with challenging requirements that are not possible with the current version.
Here is the [active goal](https://github.com/bekorn/GoodEnoughRenderer/issues/2).
