New-Variable -Name "ExtPath"	-Value ".\ExtLib\"

# OpenXR Runtime
Invoke-Webrequest -Uri https://github.com/KhronosGroup/OpenXR-SDK/releases/download/release-1.0.11/openxr_loader_windows-1.0.11.zip -OutFile $ExtPath\openxr_loader.zip
Expand-Archive -Path $ExtPath\openxr_loader.zip -DestinationPath $ExtPath\tmp
Copy-Item -Recurse -Path $ExtPath\tmp\openxr_loader_windows\include\openxr -Destination $ExtPath\include\
Copy-Item -Recurse -Path $ExtPath\tmp\openxr_loader_windows\x64\lib -Destination $ExtPath\lib
Copy-Item -Recurse -Path $ExtPath\tmp\openxr_loader_windows\x64\bin -Destination $ExtPath\dll
