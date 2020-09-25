New-Variable -Name "ExtPath"	-Value ".\ExtLib\"

#Create folders
New-Item -ItemType Directory -Force -Path $ExtPath\tmp
New-Item -ItemType Directory -Force -Path $ExtPath\include
New-Item -ItemType Directory -Force -Path $ExtPath\lib
New-Item -ItemType Directory -Force -Path $ExtPath\dll

# OpenXR Runtime
New-Item -ItemType Directory -Force -Path $ExtPath\include\openxr

Invoke-Webrequest -Uri https://github.com/KhronosGroup/OpenXR-SDK/releases/download/release-1.0.11/openxr_loader_windows-1.0.11.zip -OutFile $ExtPath\openxr_loader.zip
Expand-Archive -Path $ExtPath\openxr_loader.zip -DestinationPath $ExtPath\tmp
Copy-Item -Path $ExtPath\tmp\openxr_loader_windows\include\openxr\* -Destination $ExtPath\include\openxr
Copy-Item -Path $ExtPath\tmp\openxr_loader_windows\x64\lib\openxr_loader.lib -Destination $ExtPath\lib
Copy-Item -Path $ExtPath\tmp\openxr_loader_windows\x64\bin\openxr_loader.dll -Destination $ExtPath\dll

# freeglut
New-Item -ItemType Directory -Force -Path $ExtPath\include\GL

Invoke-Webrequest -Uri https://www.transmissionzero.co.uk/files/software/development/GLUT/freeglut-MSVC.zip -OutFile $ExtPath\freeglut.zip
Expand-Archive -Path $ExtPath\freeglut.zip -DestinationPath $ExtPath\tmp
Copy-Item -Path $ExtPath\tmp\freeglut\include\GL\* -Destination $ExtPath\include\GL
Copy-Item -Path $ExtPath\tmp\freeglut\lib\x64\freeglut.lib -Destination $ExtPath\lib
Copy-Item -Path $ExtPath\tmp\freeglut\bin\x64\freeglut.dll -Destination $ExtPath\dll

# glew
Invoke-Webrequest -Uri https://nchc.dl.sourceforge.net/project/glew/glew/2.1.0/glew-2.1.0-win32.zip -OutFile $ExtPath\glew.zip
Expand-Archive -Path $ExtPath\glew.zip -DestinationPath $ExtPath\tmp
get-childitem -Path $ExtPath\tmp -filter "glew-2.*" |  rename-item -newname "glew"
Copy-Item -Path $ExtPath\tmp\glew\include\GL\* -Destination $ExtPath\include\GL
Copy-Item -Path $ExtPath\tmp\glew\lib\Release\x64\glew32.lib -Destination $ExtPath\lib\
Copy-Item -Path $ExtPath\tmp\glew\bin\Release\x64\glew32.dll -Destination $ExtPath\dll\
