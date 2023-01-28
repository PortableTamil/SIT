@echo off
Rem change the code page to UTF-8. Also, you need to use Lucida console fonts
call :%~1 "%~2" %~3 %~4 %~5 %~6 %~7 %~8 %~9
cd /d %~dp0
goto :eof

:PAp 
Rem PortableApps "appname" "Args"
cd /d "..\%~1"
tasklist /FI "IMAGENAME eq %~1.exe" | find /i "%~1.exe"
IF ERRORLEVEL 1 start /min %~1.exe %~2 %~3 %~4 %~5 %~6 %~7 %~8
goto :eof

:PAB
Rem PortableAppsBitnessbased "appname"
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set Arch="" || set Arch=64
cd /d "..\%~1"
tasklist /FI "IMAGENAME eq %~1%Arch%.exe" | find /i "%~1%Arch%.exe"
IF ERRORLEVEL 1 start /min %~1%Arch%.exe %~2 %~3 %~4 %~5 %~6 %~7 %~8
goto :eof

:SAp
Rem SystemApps "Location" "appname" "Args"
cd /d "%~1"
tasklist /FI "IMAGENAME eq %~2.exe" | find /i "%~2.exe"
IF ERRORLEVEL 1 start /min %~2.exe %~3 %~4 %~5 %~6 %~7 %~8
goto :eof

:PAO
Rem PortableAppOnly "appname" arguments discarded.
cd /d "..\%~1"
tasklist /FI "IMAGENAME eq %~1.exe" | find /i "%~1.exe"
IF ERRORLEVEL 1 start /min %~1.exe
goto :eof

:getNStatus
set ipaddr=%2
set state=Down
for /f "tokens=5,6,7" %%a in ('ping -n 1 %ipaddr%') do (
	if "x%%b"=="xunreachable." goto :endloop
	if "x%%a"=="xReceived" if "x%%c"=="x1,"  set state=Up
)
:endloop
set "%~1=%state%"
goto :eof

:getTTime returnVar [TTime]
if not "%~1"=="" set "%~1=%Office%"
if "%~2"=="" ( set "Office=Working" ) else ( set "Office=%~2" )
set DOW=%date:~0,3%
set /a Hour=%time:~0,2%
if %Hour% lss 9 ( set "Office=Leave" )
if %Hour% geq 17 ( set "Office=Leave" )
if "%DOW%" == "Sun" ( set "Office=Leave" )
if "%DOW%" == "Sat" ( set "Office=Leave" )
goto :eof

:getDoY DoY DoLY TrYear TrDay DoTLY
for /F "usebackq tokens=1,2 delims==" %%i in (`wmic os get LocalDateTime /VALUE 2^>NUL`) do if '.%%i.'=='.LocalDateTime.' set ldt=%%j
rem Set ldt=20200204222340.360000+330
Set /a Year=%ldt:~0,4%, Month=1%ldt:~4,2% %% 100, Day=1%ldt:~6,2% %% 100,Hour=%ldt:~8,2%, Minute=%ldt:~10,2%, Seconds=1%ldt:~12,2% %% 100
Set /a YMod4=%Year% %% 4
Set /a LYear = 0
if %YMod4% equ 0 set /a LYear = 1
rem Define accumulated days of the year for various months
Set /a acm[1]=0, acm[2]=31, acm[3]=59, acm[4]=90, acm[5]=120, acm[6]=151, acm[7]=181, acm[8]=212, acm[9]=243, acm[10]=273, acm[11]=304, acm[12]=334
rem To Access array indexed variable extensions are enabled 
setlocal EnableExtensions EnableDelayedExpansion
set /a DoY= !acm[%Month%]! + %Day%
if %Month% gtr 2 set /A DoY+=%LYear%
endlocal & Set "Doy=%DoY%"
Rem Extensionts are disabled and variable value is takenout 
Rem Compute Day of year and Day of Leap year
set /a DoLY = %DoY% + %YMod4%*365
Rem convert to Thiruvalluvar day of year and year.
Set /a TrYear = %Year%+31
Set /a TrDay = %DoY% - 14
if %LYear% equ 1 set /A Trday = %TrDay% - 1
if %Month% equ 1 if %TrDay% lss 1 ( set /A TrYear = %TrYear% - 1, TrDay = %TrDay% + 365 + %LYear%)
Rem conversion over 
Set /a TYMod4=%TrYear% %% 4
set /a DoTLY = %TrDaY% + %TYMod4%*365
echo %Tryear% - %TrDay%
endlocal & Set "%~1=%DoY%" & Set "%~2=%DoLY%" & Set "%~3=%TrYear%" & Set "%~4=%TrDay%"  & Set "%~5=%DoTLY%" 
goto :eof

:ClrWelMsg
chcp 65001
REG ADD HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System /v legalnoticecaption /d "" /f
REG ADD HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System /v legalnoticetext /d "" /f
REG ADD HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\LogonUI /v LastLoggedOnDisplayName /d "தமிழ் நேரம்" /f
REG ADD HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\LogonUI\SessionData\1 /v LoggedOnDisplayName /d "தமிழ் நேரம்" /f
rem REG ADD HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\LogonUI\SessionData\2 /v LoggedOnDisplayName /d "தமிழ் நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Identity /v ADUserDisplayName /d "தமிழ் நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Lync\anish.t@ni.com  /v UserDisplayName /d "தமிழ் நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Identity\Identities\76ad4112-eb66-4791-8292-b33c552ac017_ADAL /v FirstName /d "தமிழ்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Identity\Identities\76ad4112-eb66-4791-8292-b33c552ac017_ADAL /v LastName /d "நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Identity\Identities\76ad4112-eb66-4791-8292-b33c552ac017_ADAL /v FriendlyName /d "தமிழ் நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Identity\Identities\76ad4112-eb66-4791-8292-b33c552ac017_ADAL /v Initials /d "தமிழ்நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Identity\Identities\anish.t@ni.com_AD /v FriendlyName /d "தமிழ் நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Identity\Identities\anish.t@ni.com_AD /v FirstName /d "தமிழ்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Identity\Identities\anish.t@ni.com_AD /v LastName /d "நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Identity\Identities\anish.t@ni.com_AD /v Initials /d "தமிழ்நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Licensing\OlsToken\Identity\O365ProPlusRetail /v firstName /d "தமிழ்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\Office\16.0\Common\Licensing\OlsToken\Identity\O365ProPlusRetail /v lastName /d "நேரம்" /f
REG ADD HKEY_CURRENT_USER\Software\Microsoft\OneDrive\Accounts\Business1 /v UserName /d "தமிழ் நேரம்" /f
REG ADD HKEY_USERS\S-1-5-21-592541541-3038525560-1254390659-97021\Software\Microsoft\Office\16.0\Common\ServicesManagerCache\Identities\76ad4112-eb66-4791-8292-b33c552ac017_ADAL\O365_SHAREPOINT_76ad4112-eb66-4791-8292-b33c552ac017_2051 /v ConnectionUserDisplayName /d "தமிழ் நேரம்" /f
REG ADD HKEY_USERS\S-1-5-21-592541541-3038525560-1254390659-97021\Software\Microsoft\Office\16.0\Common\ServicesManagerCache\Identities\76ad4112-eb66-4791-8292-b33c552ac017_ADAL\O365_SHAREPOINT_76ad4112-eb66-4791-8292-b33c552ac017_4194307 /v ConnectionUserDisplayName /d "தமிழ் நேரம்" /f
REG ADD HKEY_USERS\S-1-5-21-592541541-3038525560-1254390659-97021\Software\Microsoft\Office\16.0\Common\ServicesManagerCache\Identities\76ad4112-eb66-4791-8292-b33c552ac017_ADAL\O365_SHAREPOINTGROUP_76ad4112-eb66-4791-8292-b33c552ac017_16384 /v ConnectionUserDisplayName /d "தமிழ் நேரம்" /f
goto :eof

:NetStart
netsh wlan set hostednetwork mode=allow ssid=%computername% key=%username%
netsh wlan start hostednetwork
goto :eof

:NetStop
netsh wlan stop hostednetwork
mkdir தமிழ் நேரம்.{ED7BA470-8E54-465E-825C-99712043E01C} for godmode
goto :eof

:Fibonacci
set "fst=0"
set "fib=1"
set "limit=100"
echo %fst%
echo %fib%

call:myFibo fib,%fst%,%limit%
echo.The next Fibonacci number greater or equal %limit% is %fib%.
echo.&goto:eof

:myFibo  -- calc the next Fibonacci number greater or equal to a limit
::       -- %~1: return variable reference and current number
set /a "CurNum=%~1"
set /a "PreNum=%~2"
set /a "Limit=%~3"
set /a "NexNum=CurNum + PreNum"
echo %NexNum%
if /i %NexNum% LSS %Limit% call:myFibo NexNum,%CurNum%,%Limit%
(IF "%~1" NEQ "" SET "%~1=%NexNum%"
)
goto:eof