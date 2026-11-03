# Assinatura digital PACE Eden (wraptool) para Pro Tools Comercial no Windows
# Uso: .\scripts\sign_aax_windows.ps1 -AaxPath "dist\Windows_x64\KingLion Box.aaxplugin" -WcGuid "SEU-WCGUID" -CertThumbprint "SEU-THUMBPRINT"

param(
    [string]$AaxPath = "dist\Windows_x64\KingLion Box.aaxplugin",
    [string]$PaceAccount = "wyattsounds",
    [Parameter(Mandatory=$true)][string]$WcGuid,
    [Parameter(Mandatory=$true)][string]$CertThumbprint
)

Write-Host "Assinando '$AaxPath' com PACE Eden wraptool para Pro Tools..."
wraptool sign --verbose `
    --account $PaceAccount `
    --wcguid $WcGuid `
    --signtool "signtool" `
    --signid $CertThumbprint `
    --in "$AaxPath" `
    --out "$AaxPath"

Write-Host "Concluido! Copie '$AaxPath' para: C:\Program Files\Common Files\Avid\Audio\Plug-Ins\"
