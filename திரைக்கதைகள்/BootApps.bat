@echo on
cd /d %~dp0
call Boot.bat ClrWelMsg
call Boot.bat getNStatus Link google.com
call Boot.bat getTTime Office
call Boot.bat getDoY DoY DoLY TrYear TrDay DoTLY
Set /a Kod= (%DoTLY% %% 1330)+1
Set Kod1=000%Kod%
Set Kod2=%Kod1:~-4%
call :CmnTasks
call :%Office%%Link%
echo %Office%-%Link%-%DoLY%-%Kod2%-%TrDay%-%TrYear%
echo %date%
call Boot.bat Fibonacci
Rem timeout 7
Rem pause
exit

:onetime
call Boot.bat PAp "7-ZipPortable"
call Boot.bat SAp "" "Autoruns"
call Boot.bat PAp "ccPortable"
call Boot.bat PAp "CamStudioPortable"
call Boot.bat PAp "ClamWinPortable"
call Boot.bat PAp "FastCopyPortable"
call Boot.bat PAp "SumatraPDFPortable"
call Boot.bat SAp "" "Autologon"
call Boot.bat PAp "VLCPortable"
call Boot.bat PAp "VirtualMagnifyingGlassPortable"
goto :eof

:CmnTasks
call Boot.bat PAp "ArthaPortable"
call Boot.bat PAp "Azhagi-Plus"
call Boot.bat PAp "Beeftext"
call Boot.bat PAp "Kural"
call Boot.bat PAp "Notepad++Portable"
call Boot.bat PAp "wweb32"
call Boot.bat SAp "..\BGInfo-SIT" BGInfo Thirukkural\Thirukkural-%Kod2%.bgi /NOLICPROMPT /SILENT /timer:0
call Boot.bat SAp "..\ZoomIt-SIT" ZoomIt
call Boot.bat PAB "Everything" "-startup"
goto :eof

:WorkingUp
call Boot.bat SAp "C:\Users\INThiruA\AppData\Local\Microsoft\OneDrive\" OneDrive "/background"
call Boot.bat SAp "C:\Program Files\Microsoft Office\root\Office16" OUTLOOK 
call Boot.bat SAp "C:\Users\INThiruA\AppData\Local\Microsoft\Teams\" Update --processStart "Teams.exe" --process-start-args "--system-initiated"
goto :eof

:LeaveUp
call Boot.bat PAp DesktopTicker
call Boot.bat SAp "C:\Program Files (x86)\AirDroid" AirDroid
call Boot.bat PAp ThunderbirdPortable
call Boot.bat PAp sPortable
call Boot.bat PAp FirefoxPortable
call Boot.bat PAp "PeerBlockPortable"
call Boot.bat SAp "..\..\" தொடங்கு
goto :eof

:LeaveDown
call Boot.bat SAp "..\StrokesPlus-B64Signed" StrokesPlus
call Boot.bat PAp "AIMPPortable"
call Boot.bat PAp "DittoPortable"
call Boot.bat PAp "Mp3Tag"
goto :eof

:WorkingDown
call Boot.bat PAp "DicomPortable"
call Boot.bat PAp "WorkravePortable"
goto :eof
