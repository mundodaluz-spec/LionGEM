; ============================================================================
; Instalador Grafico Oficial Windows x64 (Inno Setup 6)
; Produto: KingLion Box 1.0 — Tube Voice Channel
; Empresa: WYATT Sounds
; ============================================================================

#define MyAppName "KingLion Box"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "WYATT Sounds"
#define MyAppURL "https://wyattsounds.com"

[Setup]
AppId={{8F4C92E1-5B7D-4A39-9E24-71C8D50A1F10}
AppName={#MyAppName} {#MyAppVersion}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion} — {#MyAppPublisher}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DefaultDirName={autopf64}\{#MyAppPublisher}\{#MyAppName}
DefaultGroupName={#MyAppPublisher}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir=..\..\dist\Windows_Installer
OutputBaseFilename=KingLion_Box_1.0_WYATT_Sounds_Windows_x64_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
UninstallDisplayName={#MyAppName} {#MyAppVersion} ({#MyAppPublisher})

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Instalacao Completa (Todos os formatos de Plugin + Standalone)"
Name: "custom"; Description: "Instalacao Personalizada"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3 64-bit (Reaper, Cubase, Studio One, Ableton, FL Studio)"; Types: full custom
Name: "standalone"; Description: "Aplicativo Standalone (.exe)"; Types: full custom
#ifdef HAS_AAX
Name: "aax"; Description: "Plugin AAX 64-bit (Avid Pro Tools)"; Types: full custom
#endif

[Files]
; VST3 -> C:\Program Files\Common Files\VST3\
Source: "..\..\staging\VST3\*"; DestDir: "{commoncf64}\VST3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs

; Aplicativo Standalone -> C:\Program Files\WYATT Sounds\KingLion Box\
Source: "..\..\staging\Standalone\*"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion recursesubdirs createallsubdirs

#ifdef HAS_AAX
; Avid Pro Tools AAX -> C:\Program Files\Common Files\Avid\Audio\Plug-Ins\
Source: "..\..\staging\AAX\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins"; Components: aax; Flags: ignoreversion recursesubdirs createallsubdirs
#endif

[Icons]
Name: "{autoprograms}\{#MyAppPublisher}\{#MyAppName}"; Filename: "{app}\KingLion Box.exe"; Components: standalone
Name: "{autoprograms}\{#MyAppPublisher}\Desinstalar {#MyAppName}"; Filename: "{uninstallexe}"
