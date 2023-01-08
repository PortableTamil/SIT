REM பாதுகாப்பாகநீக்கு.bat மூலம் இடத்தை குறைக்கவும்
Rem பிறகு
Rem இத்த கோப்பை மெய்நிகர் கணினிக்கு அருகில் நகல் எடுக்கவும்.
Rem நகலின் பெயரை "*.VBOX" இன் பெயரில் மாற்றவும்.
Rem எடுத்து காட்டு கோப்புகள்: வின்10.bat மற்றும் வின்10.vbox 
Rem இப்போழுது வின்10.bat கோப்புபை இயக்கவும்.

cd %~dp0
cd "C:\Program Files\Oracle\VirtualBox"
VBoxManage.exe modifymedium "%~dp0%~n0.vdi" --compact
pause

