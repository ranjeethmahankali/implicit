# implicit

I was quite impressed by [nTopology's implicit modeling
software](https://ntopology.com/) and wanted to experiment with
implicit modeling myself.

I started with a simple command line application with a 3d viewer to
visualize the results of the commands being typed. Later I'd like to
try and make plugins to integrate this with other modeling software.

#### OpenCL and OpenGL

The OpenCL code performs raytracing on the implicit geometry created
by the user via the REPL interface. The result of the raytracing is
RGBA color data for each pixel in the viewer. The OpenCL kernel that
performs the raytracing writes this color information to a pixel
buffer on the device. OpenGL then renders the pixel buffer to the
screen.

#### Implicit Kernel ####

This module contains all the basic entity types. All entities inherit
from `entity`. The `simple_entity` struct represents simplest
forms of implicit geometry, defined by a single equation. The
`comp_entity` struct represents geometry that is a result of combining
two other entities via an operation. The operation can be a boolean
operation, or a blending operation etc.

Every `entity` has `render_data_size` function which estimates the
size of the render data for that entity. It also has a `copy_render_data`
function, which generates and copies the render data to the
given destination. This render data is later copied to a device buffer,
whenever a new entity is created / has to be shown in the viewer. This
data is then used by the OpenCL kernel that performs the raytracing.

`ent_ref` is a shared pointer that references an entity. The user is
allowed to freely assign and reassign `ent_ref` values to Lua
variables. The Lua garbage collector will take care of releasing the
reference and cleaning up the heap allocated memory so the user
doesn't have to think about that.

#### Implicit-Shell Application ####

This module has several C functions, which are bound to lua. These
functions depend on the Implicit Kernel to do the user's bidding.
The user can also load external lua scripts using the `load`
function.

## Building the Solution ##

You should first install the following dependencies using vcpkg

```
vcpkg install glm:x64-windows-static
vcpkg install lua:x64-windows-static
vcpkg install boost-gil:x64-windows-static
vcpkg install boost-algorithm:x64-windows-static
vcpkg install glfw3:x64-windows-static
vcpkg install glew:x64-windows-static

vcpkg integrate install
```

The environment variable `VCPKG_PATH` must be set to the root directory of vcpkg.
Then build the project using cmake:

```
cmake -S . -B build/
cmake --build build/ --config Release
```
The binaries should be built to `build/Release/` directory.

## Using the application ##

Once the solution is built, just run the `implicitshell.exe`. You will see
a REPL interface that begins with `>>>`. Because this is a Lua
console, you can do basic lua stuff, create variables, do math etc. If
you want to see all the function bindings from the implicit kernel,
just call `help_all` function.

```
>>> help_all()

***
A list of all available implicit kernel function bindings, names,
return types, descriptions should be printed here.
***
```

If you want to use one of the functions and want to see a list of
expected arguments for that function, use the `help` function. This
function takes a single argument, the name of the function as a
string. You can also type `help("help")` to get this information.

Below is an example of a function's help printed to the console.

```
>>> help("cylinder")

ent_ref cylinder(...)
        Creates a cylinder
        Arguments:
                float xstart: The x coordinate of the start of the cylinder
                float ystart: The y coordinate of the start of the cylinder
                float zstart: The z coordinate of the start of the cylinder
                float xend: The x coordinate of the end of the cylinder
                float yend: The y coordinate of the end of the cylinder
                float zend: The z coordinate of the end of the cylinder
                float radius: The radius of the cylinder
```

#### Viewer ####

You can pan by holding down the left mouse button. You can orbit
around the center of the screen using the right mouse button. You can
use the scroll wheel to zoom in and out.

The geometry is rendered in grayscale. The faint red, green and blue
planes represent the bounds of the build volume. All modeling
operations are expected to be contained inside these bounds. You can
change these bounds using the `setbounds` Lua function.

### Example

This is an example of what can be created with this application with
just a few commands.

```
>>> rad = 1.25
>>> b = box(-rad, -rad, 0, rad, rad, 4)
>>> c = cylinder(0, 0, 0, 0, 0, 4, rad)
>>> bl = smoothblend(b, c, 0, 0, 0, 0, 0, 4)
>>> lat = bintersect(bl, gyroid(16, .2))
>>> part = smoothblend(bl, lat, 0, 0, -1, 0, 0, 5)
```

![Example](example.png)

## P.S.

I decided to pause the work on this repo and start working on [implicitv2](https://github.com/themachine0094/implicitv2) instead. In v2, instead of doing my own raytracing and building my own datastructures from scratch, I want to make use of [OpenVDB](https://www.openvdb.org/) where it makes sense.
