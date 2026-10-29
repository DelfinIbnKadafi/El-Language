#define AppName "ELVM"
#define AppVersion "0.8.2"

[Setup]
AppId={{A81C25B7-9F41-4C75-B90E-2A0F6B12A001}
AppName={#AppName}
AppVersion={#AppVersion}
DefaultDirName={autopf}\ELVM
DisableDirPage=no
PrivilegesRequired=admin
ChangesEnvironment=yes
OutputDir=output
OutputBaseFilename=elvm-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Dirs]
; Program Files can only be changed by administrators. The library folder is where
; "ellib install" creates folders, so every user must be allowed to modify it.
Name: "{app}\library"; Permissions: users-modify

[Files]
Source: "elvm.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "ellib.exe"; DestDir: "{app}"; Flags: ignoreversion

[UninstallDelete]
; Remove the libraries installed with ellib, Inno only removes folders it created itself
Type: filesandordirs; Name: "{app}\library"

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Check: NeedsAddPath(ExpandConstant('{app}')); Flags: preservestringtype

[Code]
function NeedsAddPath(Param: string): Boolean;
var
  ExistingPath: string;
begin
  if not RegQueryStringValue(
    HKEY_LOCAL_MACHINE,
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
    'Path',
    ExistingPath
  ) then
    ExistingPath := '';

  Result := Pos(';' + Uppercase(Param) + ';',
    ';' + Uppercase(ExistingPath) + ';') = 0;
end;