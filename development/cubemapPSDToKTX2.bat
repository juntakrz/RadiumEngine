@echo off

REM colour textures
REM BC1 15		sRGB RGB format

echo Radium Engine's PSD cubemap to KTX cubemap baker script
echo -------------------------------------------------------
echo Will convert files named _cubemap.psd using nvtt_export
echo These must have layers named similarly to template_cubemap.psd
echo *
echo *

setlocal enabledelayedexpansion

for %%f in ("*.psd") do (
	set file=%%f
	if not "!file!"=="!file:_cubemap.psd=!" (
		REM normal map, convert to BC5 format with linear colourspace
		echo Compressing %%f as a cubemap using BC1 format
		nvtt_export --zcmp 12 -f 15 -o %%~nf.ktx2 %%f
	) 
)

endlocal