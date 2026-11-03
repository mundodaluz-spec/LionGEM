cmake_minimum_required(VERSION 3.22)

project(KingLionBox VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 1. Baixa automaticamente o JUCE 8.0.15 caso nao exista uma pasta local ./JUCE
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(STATUS "Usando JUCE local em ./JUCE")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    message(STATUS "Baixando JUCE 8.0.15 via FetchContent...")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.15
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# 2. Formatos multi-DAW: VST3 + Standalone (Windows/macOS/Linux), AU (macOS) e AAX (Pro Tools)
set(KINGLION_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND KINGLION_FORMATS AU)
endif()

# Deteccao automatica do Avid AAX SDK para compilar nativamente para Pro Tools (.aaxplugin)
set(AAX_SDK_PATH "" CACHE PATH "Caminho raiz do Avid AAX SDK (ex.: SDKs/AAX)")
if(NOT AAX_SDK_PATH AND DEFINED ENV{AAX_SDK_PATH})
    set(AAX_SDK_PATH "$ENV{AAX_SDK_PATH}")
elseif(NOT AAX_SDK_PATH AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDKs/AAX")
    set(AAX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/SDKs/AAX")
endif()

if(AAX_SDK_PATH AND EXISTS "${AAX_SDK_PATH}")
    message(STATUS "Avid AAX SDK detectado em '${AAX_SDK_PATH}' -> Habilitando compilacao AAX (Pro Tools)!")
    juce_set_aax_sdk_path("${AAX_SDK_PATH}")
    list(APPEND KINGLION_FORMATS AAX)
else()
    message(STATUS "Avid AAX SDK ausente -> Compilando ${KINGLION_FORMATS}. Para gerar .aaxplugin nativo, coloque o SDK em SDKs/AAX ou defina AAX_SDK_PATH.")
endif()

# 3. Definicao oficial do plugin KingLion Box 1.0 — WYATT Sounds
juce_add_plugin(KingLionBox
    VERSION                     1.0.0
    COMPANY_NAME                "WYATT Sounds"
    COMPANY_WEBSITE             "https://wyattsounds.com"
    BUNDLE_ID                   "com.wyattsounds.kinglionbox"
    AAX_IDENTIFIER              "com.wyattsounds.kinglionbox"
    PLUGIN_MANUFACTURER_CODE    Wyat
    PLUGIN_CODE                 Klbx
    FORMATS                     ${KINGLION_FORMATS}
    PRODUCT_NAME                "KingLion Box"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    NEEDS_WEB_BROWSER           FALSE
    NEEDS_CURL                  FALSE
)

target_sources(KingLionBox
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(KingLionBox
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(KingLionBox
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(KingLionBox
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

# 4. Alvo de verificacao matematica do motor DSP (independente de GUI)
enable_testing()
add_executable(KingLionDSPVerify tests/dsp_verify.cpp)
target_include_directories(KingLionDSPVerify PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
add_test(NAME KingLionDSPVerify COMMAND KingLionDSPVerify)
