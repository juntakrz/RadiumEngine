DEVELOPMENT TOOLS
=================


1. "compileShaders_Win_x64_DEBUG"
Runs automatically when using a debug build of Radium Engine, compiles all GLSL shaders into SPIR-V format with the debug flag.

2. "glTF_toKTX2"
Converts glTF-standard files to an appropriately compressed KTX2 format. Can be launched in texture directory of glTF model.
NVidia Texture Tools must be present in system PATH.

3. "cubemapPSDtoKTX2"
Converts PSD with 6 appropriately named layers ("front", "back", "left", "right", "up", "down") to KTX2 cubemap using NVidia Texture Tools.
NVidia Texture Tools must be present in system PATH.