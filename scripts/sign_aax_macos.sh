#!/usr/bin/env bash
# Assinatura digital PACE Eden (wraptool) para Pro Tools Comercial no macOS
# Uso: ./scripts/sign_aax_macos.sh "/caminho/para/KingLion Box.aaxplugin" "SEU_WCID_PACE" "APPLE_DEV_ID_APP"

set -euo pipefail

AAX_BUNDLE="${1:-dist/macOS_Universal/KingLion Box.aaxplugin}"
PACE_WCID="${2:-SEU_WCID_AQUI}"
APPLE_SIGN_ID="${3:-Developer ID Application: WYATT Sounds}"

echo "Assinando '$AAX_BUNDLE' com PACE Eden wraptool para Pro Tools..."
wraptool sign \
  --verbose \
  --account "${PACE_ACCOUNT:-wyattsounds}" \
  --wcguid "${PACE_WCID}" \
  --signid "${APPLE_SIGN_ID}" \
  --in "${AAX_BUNDLE}" \
  --out "${AAX_BUNDLE}"

echo "Assinatura AAX concluida! Copie para: /Library/Application Support/Avid/Audio/Plug-Ins/"
