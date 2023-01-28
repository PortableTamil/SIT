@rem use either 
@rem .bat 
@rem cmd /c ShortcutsSearchAndReplace64.exe args
@rem START /WAIT ShortcutsSearchAndReplace64.exe args

ShortcutsSearchAndReplace64.exe Op=Search Search="*" IncUserStartup IncCommonStartup IncUserDesktop IncCommonDesktop InsideDirName

ShortcutsSearchAndReplace64.exe Op=Search Search="*view*" IncUserStartup IncCommonStartup IncUserDesktop IncCommonDesktop InsideDirName

ShortcutsSearchAndReplace64.exe Op=SearchDead SearchPostOp=Resolve IncUserStartup IncCommonStartup IncUserDesktop IncCommonDesktop InsideDirName

ShortcutsSearchAndReplace64.exe Op=Search Search="*" InsideDirName InsideIconDirName IncDir="C:\ProgramData\Microsoft\Windows\Start Menu\Programs\"
ShortcutsSearchAndReplace64.exe Op=Search Search="*" InsideDirName IncDir="C:\ProgramData\Microsoft\Windows\Start Menu\Programs\"
ShortcutsSearchAndReplace64.exe Op=Search Search="*" InsideIconDirName IncDir="C:\ProgramData\Microsoft\Windows\Start Menu\Programs\"

ShortcutsSearchAndReplace64.exe Op=Replace Search="SomethingThatDoesntExists" Replace="SomethingThatDoesntExists" IncUserStartup IncCommonStartup IncUserDesktop IncCommonDesktop InsideDirName
ShortcutsSearchAndReplace64.exe Op=Replace Search="SomethingThatDoesntExists" Replace="SomethingThatDoesntExists" InsideDirName InsideIconDirName IncDir="C:\ProgramData\Microsoft\Windows\Start Menu\Programs\"
