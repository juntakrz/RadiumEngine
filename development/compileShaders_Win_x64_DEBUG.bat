@echo off

%VULKAN_SDK%/Bin/glslc -g "../content_src/shaders/vs_default.vert" -o "../content/shaders/vs_default.spv"
%VULKAN_SDK%/Bin/glslc -g "../content_src/shaders/fs_default.frag" -o "../content/shaders/fs_default.spv"