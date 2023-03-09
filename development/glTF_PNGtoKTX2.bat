@echo off

REM colour textures
REM BC1 15		sRGB RGB format
REM BC7 23		sRGB RGBA format

REM normal maps
REM BC5 21		linear normal map format

echo Radium Engine glTF texture baker script
echo ---------------------------------------
echo Converting .PNG files into KTX2 format using BC1/BC5/BC7 compression
echo Requires glTF-standard naming of files and nvtt_export added to system path
echo *
echo *

setlocal enabledelayedexpansion

for %%f in ("*.png") do (
	set file=%%f
	if not "!file!"=="!file:_normal.png=!" (
		REM normal map, convert to BC5 format with linear colourspace
		echo Compressing %%f as a normal map using BC5 format
		nvtt_export --zcmp 12 -f 21 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:_metallicRoughness.png=!" (
		REM physical map, convert to BC1 format with sRGB colourspace
		echo Compressing %%f as a physical map using BC1 format
		nvtt_export --zcmp 12 -f 15 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:_occlusion.png=!" (
		REM occlusion map, convert to BC1 format with sRGB colourspace
		echo Compressing %%f as an occlusion map using BC1 format
		nvtt_export --zcmp 12 -f 15 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:_emissive.png=!" (
		REM emissive map, convert to BC1 format with sRGB colourspace
		echo Compressing %%f as an emissive map using BC1 format
		nvtt_export --zcmp 12 -f 15 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:_baseColor.png=!" (
		REM colour map to BC1 with sRGB colourspace
		echo Compressing %%f as a colour map using BC7 format
		nvtt_export --zcmp 12 -f 23 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:.png=!" (
		REM all other to BC1 with sRGB colourspace
		echo Compressing %%f as an extra map using BC1 format
		nvtt_export --zcmp 12 -f 15 -o %%~nf.ktx2 %%f
	) else (
		echo File %%f isn't of a properly named PNG format.
	)
)

endlocal