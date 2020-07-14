Invoke-Webrequest -Uri https://github.com/KhronosGroup/OpenXR-SDK/releases/download/release-1.0.9/openxr_loader_windows-1.0.9.zip -OutFile .\openxr_loader.zip
Expand-Archive -Path .\openxr_loader.zip -DestinationPath .\
