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
