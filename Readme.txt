﻿Urho3D - cross-platform rendering and game engine
-------------------------------------------------

http://urho3d.googlecode.com

Licensed under the MIT license, see License.txt for details.


Credits
-------

Urho3D development, contributions and bugfixes by:
- Lasse Öörni (loorni@gmail.com)
- Colin Barrett
- Alex Fuller
- Jason Kinzer
- Miika Santala
- Wei Tjong Yao
- Magic.Lixin
- primitivewaste
- skaiware

Urho3D is greatly inspired by OGRE (http://www.ogre3d.org) and Horde3D
(http://www.horde3d.org). Additional inspiration & research used:
- Rectangle packing by Jukka Jylänki (clb)
  http://clb.demon.fi/projects/rectangle-bin-packing
- Tangent generation from Terathon
  http://www.terathon.com/code/tangent.html
- Fast, Minimum Storage Ray/Triangle Intersection by Möller & Trumbore
  http://www.graphics.cornell.edu/pubs/1997/MT97.pdf
- Linear-Speed Vertex Cache Optimisation by Tom Forsyth
  http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html
- Software rasterization of triangles based on Chris Hecker's
  Perspective Texture Mapping series in the Game Developer magazine
  http://chrishecker.com/Miscellaneous_Technical_Articles
- Networked Physics by Glenn Fiedler
  http://gafferongames.com/game-physics/networked-physics/
- Euler Angle Formulas by David Eberly
  http://www.geometrictools.com/Documentation/EulerAngles.pdf
- Red Black Trees by Julienne Walker
  http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
- Comparison of several sorting algorithms by Juha Nieminen
  http://warp.povusers.org/SortComparison/
- NVIDIA FXAA II for Consoles by Timothy Lottes
  http://timothylottes.blogspot.com/2011/04/nvidia-fxaa-ii-for-console.html

Urho3D uses the following third-party libraries:
- AngelScript 2.26.1 WIP (http://www.angelcode.com/angelscript/)
- Bullet 2.80 (http://www.bulletphysics.org/)
- FreeType 2.3.12 (http://www.freetype.org/)
- GLee 5.4 (http://elf-stone.com/)
- kNet (https://github.com/juj/kNet)
- libcpuid 0.2.0 (http://libcpuid.sourceforge.net/)
- MojoShader (http://icculus.org/mojoshader/)
- Open Asset Import Library 3.0 (http://assimp.sourceforge.net/)
- pugixml 1.0 (http://pugixml.org/)
- SDL 2.0 (http://www.libsdl.org/)
- StanHull (http://codesuppository.blogspot.com/2006/03/
  john-ratcliffs-code-suppository-blog.html)
- stb_image 1.29 (http://nothings.org/)
- stb_vorbis 0.99996 (http://nothings.org/)
- jo_jpeg 1.52 (http://www.jonolick.com/uploads/7/9/2/1/7921194/jo_jpeg.cpp)

DXT / ETC1 / PVRTC decompression code based on the Squish library and the Oolong
Engine.

Jack and mushroom models from the realXtend project. (http://www.realxtend.org)

Ninja model and terrain / smoke / flare / status bar textures from OGRE.

Skybox cubemap from http://www.codemonsters.de.

BlueHighway font from Larabie Fonts.

Anonymous Pro font by Mark Simonson.

NinjaSnowWar sounds by Veli-Pekka Tätilä.


Documentation
-------------

Urho3D classes have been sparsely documented using Doxygen notation. To
generate documentation into the "Docs" subdirectory, open the Doxyfile in the
root directory with doxywizard and click "Run doxygen" from the "Run" tab.
Get Doxygen from http://www.doxygen.org


Building prerequisites
----------------------

Although all required third-party libraries are included as source code, there
are system-level dependencies that must be satisfied before Urho3D can be built
successfully:

- For Windows, the June 2010 DirectX SDK needs to be installed and its include 
  and library directories set as Visual Studio global directories (Tools -> 
  Options -> Projects and Solutions -> VC++ Directories in VS2008.)

- For Linux, the following development packages need to be installed:
  libx11-dev, libxrandr-dev, libasound2-dev. Also install the package
  libgl1-mesa-dev if your GPU driver does not include OpenGL headers & libs.

- For Mac OS X, the Xcode developer tools package should include everything
  necessary.

- For Android, the Android SDK and Android NDK need to be installed.

To run Urho3D, the minimum system requirements are:

- Windows: CPU with SSE instructions support, Windows XP or newer, DirectX 9.0c,
  GPU with Shader Model 2 support (Shader Model 3 recommended.)

- Linux & Mac OS X: CPU with SSE instructions support, GPU with OpenGL 2.0
  support, EXT_framebuffer_object and EXT_packed_depth_stencil extensions.

- Android: OS version 2.2 or newer, OpenGL ES 2.0 capable GPU.

- iOS: OpenGL ES 2.0 capable GPU.

SSE requirement can be eliminated by commenting out lines that enable it from
the root CMakeLists.txt.


Desktop build process
---------------------

On desktop systems Urho3D uses CMake (http://www.cmake.org) to build. The
process has two steps:

1) Run CMake in the root directory with your preferred toolchain specified to
generate the build files. You can use the batch files or shell scripts provided:
cmake_vs2008.bat, cmake_vs2010.bat or cmake_vs2012.bat on Windows, and 
cmake_gcc.sh on Linux and Mac OS X.

2) For Visual Studio, open Urho3D.sln and build the configuration(s) you like.
For gcc, execute make (by default, cmake_gcc.sh specifies to make a
RelWithDebInfo build.)

On Windows, using other compilers than Visual Studio is not officially
supported. MinGW may work (cmake -G "MinGW Makefiles"), but may lack required
DirectX headers. They can be copied to a MinGW installation from the following 
package: http://www.libsdl.org/extras/win32/common/directx-devel.tar.gz

When using Xcode on Mac OS X, select the i386 architecture before building 
(CMake should already select this when generating the Xcode project). 
Compiling Urho3D as 64-bit is not supported.

After the build is complete, the programs can be run from the Bin directory.

To run from the Visual Studio debugger, set the Urho3D project as the startup
project, set it as a startup project and enter its relative path and filename
into Properties -> Debugging -> Command: ..\Bin\Urho3D.exe. Additionally,
entering -w into Debugging -> Command Arguments is highly recommended. This
enables startup in windowed mode: without it running into an exception or
breakpoint will be obnoxious as the mouse cursor will likely be hidden.

To actually make Urho3D.exe do something useful, it must be supplied with the
name of the script file it should load and run. You can try for example the
following arguments: Scripts/NinjaSnowWar.as -w

To make the Urho3D examples start faster on Windows & Direct3D9 mode, run
CompileAllShaders.bat from the Bin directory first.


Android build process
---------------------

First copy Bin/Data and Bin/CoreData directories to the Android/assets directory
(you can use the provided batch file CopyData.bat.) Next, execute the following
commands in the Android directory:

- android update project -p . (only needed on the first time)
- ndk-build
- ant debug

Note that ndk-build builds Urho3D twice, once without hardware floating point
instructions, and once with them. After the commands finish successfully, the
APK should have been generated to the Android/bin directory, from where it can
be installed on a device or an emulator.

For a release build, use the "ant release" command instead of "ant debug" and
follow the Android SDK instructions on how to sign your APK properly.

By default the Android package for Urho3D is com.googlecode.urho3d. For a real
application you must replace this with your own package name. Unfortunately the
name has to be replaced in several files:

- Android/AndroidManifest.xml
- Android/src/com/googlecode/urho3d/SDLActivity.java (rename directories also)
- ThirdParty/SDL/include/SDL_config_android.h, look for the NATIVE_FUNCTION
  macro


iOS build process
-----------------

Run cmake_ios.sh. This generates an Xcode project named Urho3D.xcodeproj.

Open the Xcode project and check the properties for the Urho3D project (topmost
in the Project Navigator.) In Architectures -> Base SDK, choose your iOS SDK
(CMake would automatically select latest iOS when generating the Xcode project).
In Code Signing, enter your developer identity as necessary.

The Urho3D target will actually build the application bundle and copy resources
from Bin/Data and Bin/CoreData directories. Edit its build scheme to choose
debug or release mode.


History
-------

V1.0    - Original release.

V1.1    - Object and scene model refactoring.
        - Automatic serialization of scene objects via attributes.
        - Added OpenGL and cross-platform support.
        - Switched to kNet library for networking.

V1.11   - Bugfixes and performance optimizations.
        - Added GraphicsTest example from V1.0 (now called TestSceneOld.)
        - Added fallback mode, which is used if multiple render targets or
          hardware shadow maps are not available.

V1.12   - Manipulator gizmo and multi-editing in the editor.
        - Switched to forward rendering exclusively, which is optimized to do
          more work in the vertex shader.
        - Zone system refactoring. Objects check the zone they belong to for
          per-zone light masking, ambient light and fog settings.
        - Scripting API fixes and improvements.

V1.13   - Task-based multithreading.
        - Vertex lighting option.
        - Forward and light pre-pass rendering pipelines.

V1.14   - Object (partial scene) load/save.
        - Post-processing.
        - Switched to pugixml library, scene load/save optimizations.
        - Bugfixes to rendertexture views and component attributes.

V1.15   - New deferred rendering pipeline.
        - Unicode support.
        - Live resource reloading in the editor (Windows only so far.)
        - More accurate frame timing.
        - Bugfixes to physics jittering and FBO performance issue on Linux.

V1.16   - Switched to Bullet physics library.
        - More physics constraint types.
        - Rendering and networking performance optimizations.
        - Use Squish library to implement software DXT decompression when not
          supported in hardware.

V1.2    - Android and iOS support.
        - Decal rendering.
        - Terrain rendering.
        - Joystick input support.
        - Use SDL library for windowing and input on all platforms.
        - KTX and PVR image loading (for ETC1 & PVRTC compressed textures.)
        - Removed need for shader preprocessing; reorganized shaders to be more
          friendly to base custom shaders on.
        - Inbuilt geometry shapes in the editor.
        
V1.21   - Bugfixes and code cleanup.
        - External window support (experimental.)
        - UI elements refactored to use attributes for serialization.
        - Animation state editing and animation trigger events.
        - Scene update time scale can be modified.
        - Improved the delayed method call system.

V1.22   - Configurable render path replaces hardcoded forward/prepass/deferred
          modes. Render path system also used for postprocessing now.
        - Threaded task priorities; long-running tasks (more than one frame) can
          coexist with the time-critical tasks.
        - Possibility to use also RGB normal maps.
        - CustomGeometry component, which allows geometry to be defined
          similarly to OpenGL immediate mode.
        - Elapsed time shader parameter for material animation.
        - Cubic environment mapping example shaders.
        - Separate physics collision start & end events.
        - Visual Studio 2012, Eclipse & Xcode build support.
        - Many bugfixes, including iOS 6 orientation & shadow mapping bugfixes,
          skinning on some Android devices.