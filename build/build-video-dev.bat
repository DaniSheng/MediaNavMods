del /F /S /Q .\upgrade.lgu
del /F /S /Q .\unpacked\*
rem xcopy .\orig\* .\unpacked\* /E
xcopy .\patch\* .\unpacked\* /E /Y
import-patcher ".\orig\upgrade\Storage Card\System" ".\unpacked\upgrade\Storage Card\System"
rmdir /Q /S ".\unpacked\upgrade\Storage Card4"
del /F /S /Q .\unpacked\upgrade\booter_standalone.bin
dir2lgu -p m2 "Video-dev" ./unpacked ./upgrade.lgu
rmdir /Q /S ".\unpacked\upgrade"