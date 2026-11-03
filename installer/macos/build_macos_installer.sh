#!/usr/bin/env bash
# ============================================================================
# Gerador de Instalador Nativo macOS Universal (.pkg + .dmg)
# Produto: KingLion Box 1.0 — WYATT Sounds
# Compativel com Apple Silicon (M1/M2/M3/M4) e Intel x86_64
# ============================================================================

set -euo pipefail

VERSION="1.0.0"
BUNDLE_ID="com.wyattsounds.kinglionbox"
ART_DIR="${1:-build/KingLionBox_artefacts/Release}"
OUT_DIR="${2:-dist/macOS_Installer}"

rm -rf packaging pkgroot dmg_staging "$OUT_DIR"
mkdir -p packaging/resources pkgroot/vst3 pkgroot/au pkgroot/app pkgroot/aax dmg_staging "$OUT_DIR"

echo "==> [1/4] Preparando e assinando bundles macOS Universal..."
SIGN_ID="${DEVELOPER_ID_APPLICATION:--}"

disable_relocation() {
  local plist_path="$1"
  python3 - "$plist_path" << 'PY'
import plistlib, sys
p = sys.argv[1]
with open(p, 'rb') as f:
    data = plistlib.load(f)
if isinstance(data, list):
    for item in data:
        if isinstance(item, dict):
            item['BundleIsRelocatable'] = False
with open(p, 'wb') as f:
    plistlib.dump(data, f)
PY
}

CHOICE_LINES=""
CHOICE_DEFS=""

# 1. VST3
if [ -d "$ART_DIR/VST3/KingLion Box.vst3" ]; then
  cp -R "$ART_DIR/VST3/KingLion Box.vst3" pkgroot/vst3/
  codesign --force --deep --sign "$SIGN_ID" "pkgroot/vst3/KingLion Box.vst3"
  pkgbuild --analyze --root pkgroot/vst3 packaging/vst3.plist
  disable_relocation packaging/vst3.plist
  pkgbuild \
    --identifier "${BUNDLE_ID}.vst3.pkg" \
    --version "$VERSION" \
    --root pkgroot/vst3 \
    --component-plist packaging/vst3.plist \
    --install-location "/Library/Audio/Plug-Ins/VST3" \
    "packaging/KingLionBox.vst3.pkg"
  CHOICE_LINES="${CHOICE_LINES}    <line choice=\"vst3\"/>\n"
  CHOICE_DEFS="${CHOICE_DEFS}  <choice id=\"vst3\" visible=\"true\" start_selected=\"true\" title=\"Plugin VST3 (Reaper, Cubase, Ableton, Studio One)\"><pkg-ref id=\"${BUNDLE_ID}.vst3.pkg\"/></choice>\n  <pkg-ref id=\"${BUNDLE_ID}.vst3.pkg\" version=\"${VERSION}\" onConclusion=\"none\">KingLionBox.vst3.pkg</pkg-ref>\n"
fi

# 2. Audio Unit (AU - Logic Pro / GarageBand)
if [ -d "$ART_DIR/AU/KingLion Box.component" ]; then
  cp -R "$ART_DIR/AU/KingLion Box.component" pkgroot/au/
  codesign --force --deep --sign "$SIGN_ID" "pkgroot/au/KingLion Box.component"
  pkgbuild --analyze --root pkgroot/au packaging/au.plist
  disable_relocation packaging/au.plist
  pkgbuild \
    --identifier "${BUNDLE_ID}.au.pkg" \
    --version "$VERSION" \
    --root pkgroot/au \
    --component-plist packaging/au.plist \
    --install-location "/Library/Audio/Plug-Ins/Components" \
    "packaging/KingLionBox.au.pkg"
  CHOICE_LINES="${CHOICE_LINES}    <line choice=\"au\"/>\n"
  CHOICE_DEFS="${CHOICE_DEFS}  <choice id=\"au\" visible=\"true\" start_selected=\"true\" title=\"Plugin Audio Unit AU (Logic Pro, GarageBand)\"><pkg-ref id=\"${BUNDLE_ID}.au.pkg\"/></choice>\n  <pkg-ref id=\"${BUNDLE_ID}.au.pkg\" version=\"${VERSION}\" onConclusion=\"none\">KingLionBox.au.pkg</pkg-ref>\n"
fi

# 3. Aplicativo Standalone (.app)
if [ -d "$ART_DIR/Standalone/KingLion Box.app" ]; then
  cp -R "$ART_DIR/Standalone/KingLion Box.app" pkgroot/app/
  codesign --force --deep --sign "$SIGN_ID" "pkgroot/app/KingLion Box.app"
  pkgbuild --analyze --root pkgroot/app packaging/app.plist
  disable_relocation packaging/app.plist
  pkgbuild \
    --identifier "${BUNDLE_ID}.app.pkg" \
    --version "$VERSION" \
    --root pkgroot/app \
    --component-plist packaging/app.plist \
    --install-location "/Applications" \
    "packaging/KingLionBox.app.pkg"
  CHOICE_LINES="${CHOICE_LINES}    <line choice=\"app\"/>\n"
  CHOICE_DEFS="${CHOICE_DEFS}  <choice id=\"app\" visible=\"true\" start_selected=\"true\" title=\"Aplicativo Standalone (/Applications)\"><pkg-ref id=\"${BUNDLE_ID}.app.pkg\"/></choice>\n  <pkg-ref id=\"${BUNDLE_ID}.app.pkg\" version=\"${VERSION}\" onConclusion=\"none\">KingLionBox.app.pkg</pkg-ref>\n"
fi

# 4. Avid Pro Tools AAX (se compilado com AAX SDK)
if [ -d "$ART_DIR/AAX/KingLion Box.aaxplugin" ]; then
  cp -R "$ART_DIR/AAX/KingLion Box.aaxplugin" pkgroot/aax/
  pkgbuild --analyze --root pkgroot/aax packaging/aax.plist
  disable_relocation packaging/aax.plist
  pkgbuild \
    --identifier "${BUNDLE_ID}.aax.pkg" \
    --version "$VERSION" \
    --root pkgroot/aax \
    --component-plist packaging/aax.plist \
    --install-location "/Library/Application Support/Avid/Audio/Plug-Ins" \
    "packaging/KingLionBox.aax.pkg"
  CHOICE_LINES="${CHOICE_LINES}    <line choice=\"aax\"/>\n"
  CHOICE_DEFS="${CHOICE_DEFS}  <choice id=\"aax\" visible=\"true\" start_selected=\"true\" title=\"Plugin AAX (Avid Pro Tools)\"><pkg-ref id=\"${BUNDLE_ID}.aax.pkg\"/></choice>\n  <pkg-ref id=\"${BUNDLE_ID}.aax.pkg\" version=\"${VERSION}\" onConclusion=\"none\">KingLionBox.aax.pkg</pkg-ref>\n"
fi

echo "==> [2/4] Gerando telas de boas-vindas e distribution.xml..."
cat << 'EOF' > packaging/resources/welcome.html
<!DOCTYPE html>
<html>
<body style="font-family: -apple-system, Helvetica, Arial, sans-serif; padding: 14px;">
  <h2 style="margin-bottom: 6px;">KingLion Box 1.0 — WYATT Sounds</h2>
  <p>Bem-vindo ao instalador oficial macOS Universal (Apple Silicon &amp; Intel) do <b>KingLion Box 1.0 Tube Voice Channel</b>.</p>
  <p>Este assistente instalara automaticamente os formatos selecionados nas pastas padrao de plugins de audio do macOS:</p>
  <ul>
    <li><b>VST3:</b> <code>/Library/Audio/Plug-Ins/VST3</code></li>
    <li><b>Audio Unit (AU):</b> <code>/Library/Audio/Plug-Ins/Components</code></li>
    <li><b>Standalone App:</b> <code>/Applications</code></li>
  </ul>
</body>
</html>
EOF

cat << 'EOF' > packaging/resources/conclusion.html
<!DOCTYPE html>
<html>
<body style="font-family: -apple-system, Helvetica, Arial, sans-serif; padding: 14px;">
  <h2>Instalacao Concluida!</h2>
  <p>O <b>KingLion Box 1.0 (WYATT Sounds)</b> foi instalado com sucesso.</p>
  <p>Reinicie sua DAW (Logic Pro, Reaper, Pro Tools, Ableton Live, Studio One ou Cubase) para escanear o novo plugin.</p>
</body>
</html>
EOF

cat << EOF > packaging/distribution.xml
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>KingLion Box 1.0 — WYATT Sounds</title>
  <organization>com.wyattsounds</organization>
  <domains enable_localSystem="true"/>
  <options customize="always" require-scripts="false" rootVolumeOnly="true" hostArchitectures="x86_64,arm64"/>
  <welcome file="welcome.html" mime-type="text/html"/>
  <conclusion file="conclusion.html" mime-type="text/html"/>
  <choices-outline>
$(printf "%b" "$CHOICE_LINES")
  </choices-outline>
$(printf "%b" "$CHOICE_DEFS")
</installer-gui-script>
EOF

FINAL_PKG="$OUT_DIR/KingLion_Box_1.0_WYATT_Sounds_macOS_Universal.pkg"
FINAL_DMG="$OUT_DIR/KingLion_Box_1.0_WYATT_Sounds_macOS_Universal.dmg"

echo "==> [3/4] Construindo instalador .pkg com productbuild..."
if [ -n "${DEVELOPER_ID_INSTALLER:-}" ]; then
  productbuild \
    --distribution packaging/distribution.xml \
    --resources packaging/resources \
    --package-path packaging \
    --sign "$DEVELOPER_ID_INSTALLER" \
    --timestamp \
    "$FINAL_PKG"
else
  productbuild \
    --distribution packaging/distribution.xml \
    --resources packaging/resources \
    --package-path packaging \
    "$FINAL_PKG"
fi

echo "==> [4/4] Empacotando imagem de disco .dmg..."
cp "$FINAL_PKG" "dmg_staging/Instalar KingLion Box 1.0.pkg"
cat << 'EOF' > "dmg_staging/LEIA-ME (macOS Gatekeeper).txt"
KingLion Box 1.0 — WYATT Sounds (macOS Universal: Apple Silicon & Intel)
========================================================================

1. De um duplo clique em "Instalar KingLion Box 1.0.pkg" para instalar os plugins VST3, AU, AAX e Standalone.
2. Caso o macOS exiba um aviso por o instalador ainda nao estar assinado com certificado Apple Developer ($99/ano):
   - Abra "Ajustes do Sistema" -> "Privacidade e Seguranca", role ate o final e clique em "Abrir Mesmo Assim", OU
   - No Terminal, rode: sudo xattr -rd com.apple.quarantine "/Volumes/KingLion Box 1.0 - WYATT Sounds/Instalar KingLion Box 1.0.pkg"
EOF

hdiutil create \
  -volname "KingLion Box 1.0 - WYATT Sounds" \
  -srcfolder dmg_staging \
  -ov \
  -format UDZO \
  "$FINAL_DMG"

ls -lh "$OUT_DIR"
echo "==> Instaladores macOS (.pkg e .dmg) criados com sucesso!"
