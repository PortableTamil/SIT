REM பாதுகாப்பாகநீக்கு.bat மூலம் இடத்தை குறைக்கவும்
Rem பிறகு
Rem இத்த கோப்பை மெய்நிகர் கணினிக்கு அருகில் நகல் எடுத்து இயக்கவும்.

cd %~dp0
cd "C:\Program Files\Oracle\VirtualBox"
VBoxManage.exe modifymedium "%~dp0%~n0.vdi" --compact
pause

