@echo off

cd "content_src/shaders/"
for %%f in (*) do %VULKAN_SDK%/Bin/glslc -g %%f -o "../../content/shaders/%%~nf.spv"