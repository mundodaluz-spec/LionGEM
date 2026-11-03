# KingLion Box 1.0 — WYATT Sounds
**Tube Voice Channel Strip (`VST3` • `AU` • `AAX Pro Tools` • `Standalone` + Instaladores `.EXE` e `.DMG`/`.PKG`)**

O **KingLion Box 1.0** da **WYATT Sounds** é um *channel strip* valvulado e eletro-óptico completo em C++17 / [JUCE 8](https://github.com/juce-framework/JUCE), com geração 100% automatizada de **instaladores gráficos oficiais para Windows (`.exe`) e macOS Universal (`.dmg` e `.pkg`)** direto na nuvem do GitHub Actions.

---

## 1. Fluxo Inteligente: Compilar Grátis como Público → Baixar Instaladores → Trancar como Privado

Repositórios **Públicos** no GitHub têm minutos gratuitos ilimitados no GitHub Actions (inclusive nas máquinas `macos-latest`, que em repositórios privados consomem 10x mais minutos). Para aproveitar isso com segurança:

### Passo a passo rápido no seu navegador (2 minutos)
1. No GitHub, clique em **New repository**, dê o nome `kinglion-box`, deixe marcado como **Public** e clique em **Create repository**.
2. Extraia este `.zip` no seu PC, abra a pasta `KingLionBox` e arraste **todos os arquivos e pastas internas** (incluindo `.github`, `installer`, `Source`, `tests`, `CMakeLists.txt`, etc.) para a tela de upload do repositório no GitHub (ou suba via Git Desktop / terminal) e clique em **Commit changes**.
3. Clique na aba **Actions** do repositório. O workflow **`Build Instaladores KingLion Box 1.0`** começará a rodar sozinho e, em ~6 a 8 minutos, gerará na seção **Artifacts** o pacote único **`KINGLION-BOX-1.0-INSTALADORES-WINDOWS-E-MAC`** contendo:
   - **Windows 10/11 (64-bit):** `KingLion_Box_1.0_WYATT_Sounds_Windows_x64_Setup.exe` (instalador gráfico em Português com seleção de `VST3`, `Standalone` + atalho no Menu Iniciar + Desinstalador, e `AAX` quando compilado com o SDK da Avid).
   - **macOS Universal (Apple Silicon M1–M4 + Intel):** `KingLion_Box_1.0_WYATT_Sounds_macOS_Universal.dmg` e `.pkg` (instalador nativo da Apple seguindo o padrão `pkgbuild`/`productbuild` documentado no [guia de instaladores JUCE da Moonbase](https://moonbase.sh/articles/how-to-make-macos-installers-for-juce-projects-with-pkgbuild-and-productbuild/), instalando em `/Library/Audio/Plug-Ins/VST3`, `/Library/Audio/Plug-Ins/Components` e `/Applications`).
4. **Trancar como Privado:** Assim que baixar os instaladores:
   - **Manual (2 cliques):** Vá em **Settings** → role até **Danger Zone** → **Change repository visibility** → **Change to private**.
   - **100% Automático (opcional):** Se quiser que o próprio GitHub tranque o repositório para **Private** no exato segundo em que a compilação terminar, crie um *Personal Access Token* (com permissão `repo` / `Administration: Read and write`) e salve em **Settings → Secrets and variables → Actions** com o nome **`AUTO_PRIVATIZE_PAT`**. O último estágio do `build.yml` executará a trava sozinho!

---

## 2. Como Rodar no Avid Pro Tools (`AAX` Nativo ou Via Host Imediato)

1. **Uso imediato no Pro Tools Comercial:** Instale o `VST3` pelo instalador `.exe`/`.pkg` e carregue-o dentro do Pro Tools através de um host AAX assinado como o [Blue Cat's PatchWork](https://www.bluecataudio.com/Doc/Product_PatchWork/).
2. **Gerar `.aaxplugin` nativo dentro do próprio instalador:**
   - Baixe o **AAX SDK** gratuitamente na sua conta do [Avid Developer — AAX SDK](https://developer.avid.com/aax/).
   - Converta o `.zip` do SDK para Base64 e salve no GitHub em **Settings → Secrets → Actions** como `AAX_SDK_ZIP_BASE64`. Tanto o instalador `.exe` (Windows) quanto o `.pkg`/`.dmg` (macOS) passarão a incluir automaticamente a opção **`Plugin AAX (Avid Pro Tools)`**!
   - Para assinar digitalmente o `.aaxplugin` para o Pro Tools comercial, utilize os scripts prontos em `scripts/` seguindo o [guia oficial PACE AAX Code Signing](https://paceap.com/getting-started-with-aax-code-signing-for-pro-tools-plugins/).

---

## 3. Resumo da Cadeia DSP (KingLion Box 1.0)

Baseado na arquitetura clássica documentada da [Manley VOXBOX](https://www.manley.com/products/pro-audio/channel-strips/voxbox) e no [manual técnico da UAD](https://help.uaudio.com/hc/en-us/articles/17790028958228-Manley-VOXBOX-Channel-Strip-Manual):
- **Entrada & Low Cut:** Trim `-24` a `+12 dB`, fase `0°/180°` e corte suave de `6 dB/oitava` (`80 Hz` / `120 Hz`).
- **Compressor Óptico Vactrol Pré-Préamp:** Razão ~`3:1` até `16 dB`, 5 posições de Attack (`4–70 ms`) e 5 de Release (`0,3–5,0 s`) com memória óptica dupla antes das válvulas ([Recording Magazine Review](https://www.recordingmag.com/resources/featured-reviews/universal-audio-manley-voxbox/)).
- **Pré Valvulado Classe A (4x Oversampling):** Chave `GANHO dB` (`40` a `60 dB`) controlando a realimentação negativa (*negative feedback*).
- **EQ Passivo LC Pultec (33 Frequências):** `GRAVE` Boost (`20 Hz–1 kHz`), `MÉDIO` Dip (`200 Hz–7 kHz`, [especificação de 33 bandas](https://www.adorama.com/manley-voxbox-combo-tube-channel-strip-microphone-preamp/p/mlmvbx)) e `AGUDO` Boost (`1,5–20 kHz`) sem distorção de fase nos agudos.
- **De-Esser / Limitador 10:1 + `OUVIR S` + `XFMR`:** Filtro passivo em `3, 6, 9, 12 kHz` ou `LIMITADOR` de banda inteira, monitoração isolada da sibilância, transformador de saída `XFMR` e medidor VU Sifam de 5 modos (`G-R`, `ENT`, `PRÉ`, `EQ`, `D-S`).
