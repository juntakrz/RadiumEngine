@echo off

REM colour textures
REM BC1 15		sRGB RGB format
REM BC7 23		sRGB RGBA format

REM normal maps
REM BC5 21		linear normal map format

echo Radium Engine glTF texture baker script
echo ---------------------------------------
echo Converting supported image files into KTX2 format using BC1/BC5/BC7 compression
echo Requires glTF-standard naming of files and nvtt_export added to system path
echo *
echo *

setlocal enabledelayedexpansion

for %%f in (*.bmp *.png *.jpg *.jpeg) do (
	set file=%%~nf
	if not "!file!"=="!file:_normal=!" (
		REM normal map, convert to BC5 format with linear colourspace
		echo Compressing %%f as a normal map using BC5 format
		nvtt_export --zcmp 12 -f 21 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:_metallicRoughness=!" (
		REM physical map, convert to BC1 format with sRGB colourspace
		echo Compressing %%f as a physical map using BC1 format
		nvtt_export --zcmp 12 -f 15 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:_occlusion=!" (
		REM occlusion map, convert to BC1 format with sRGB colourspace
		echo Compressing %%f as an occlusion map using BC1 format
		nvtt_export --zcmp 12 -f 15 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:_emissive=!" (
		REM emissive map, convert to BC1 format with sRGB colourspace
		echo Compressing %%f as an emissive map using BC1 format
		nvtt_export --zcmp 12 -f 15 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:_baseColor=!" (
		REM colour map to BC1 with sRGB colourspace
		echo Compressing %%f as a colour map using BC7 format
		nvtt_export --zcmp 12 -f 23 -o %%~nf.ktx2 %%f
	) else if not "!file!"=="!file:=!" (
		REM all other to BC1 with sRGB colourspace
		echo Compressing %%f as an extra map using BC1 format
		nvtt_export --zcmp 12 -f 15 -o %%~nf.ktx2 %%f
	) else (
		echo File %%f isn't of a properly named format.
	)
)

endlocal