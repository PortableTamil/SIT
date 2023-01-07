@echo on
cd /d %~dp0
cd ..

REG ADD HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run /v திருக்குறள் /t REG_SZ /d "%%பயன்பாடுகள்%%\திரைக்கதைகள்\திருக்குறள்.bat" /f

Rem update Environment variable %%பயன்பாடுகள்%% with %cd%
setx பயன்பாடுகள் %cd% /m

pause
