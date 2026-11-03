#include "PluginProcessor.h"
#include "PluginEditor.h"

KingLionBoxAudioProcessor::KingLionBoxAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "KINGLION_PARAMS", createParameterLayout())
{
}

const std::array<KingLionBoxAudioProcessor::FactoryPreset, 8>& KingLionBoxAudioProcessor::getFactoryPresets()
{
    static const std::array<FactoryPreset, 8> kPresets {{
        // 0: Inicial (Flat / Calibrado)
        { "Inicial", 0, false, 0.0f, 2, true, 0.0f, 2, 1, true, 4, 0.0f, 2, 0.0f, 8, 0.0f, 0, 1, 0.0f, true, 0.0f },
        // 1: Voz Pop Frente (Pegada Classica VOXBOX: Ganho alto 55 dB + Entrada recuada + Ar em 12k)
        { "Voz Pop Frente (Vox Classic)", 1, false, -2.5f, 3, true, 5.4f, 2, 1, true, 4, 2.5f, 2, 3.0f, 8, 4.2f, 0, 1, 4.8f, true, 0.0f },
        // 2: Voz R&B Seda (16 kHz Air + Graves Quentes em 70 Hz)
        { "Voz R&B Seda (16k Air)", 1, false, -1.5f, 2, true, 4.8f, 1, 1, true, 3, 3.2f, 1, 2.5f, 9, 5.5f, 0, 2, 4.4f, true, -0.5f },
        // 3: Locucao & Podcast Quente (Presenca proxima e controle firme)
        { "Locucao & Podcast Quente", 1, false, -2.0f, 3, true, 6.0f, 3, 3, true, 4, 3.5f, 2, 3.8f, 7, 2.8f, 0, 1, 5.5f, true, 0.0f },
        // 4: Vocal Rock Agressivo (Valvula 60 dB + Limitador 10:1 no final)
        { "Vocal Rock Agressivo (60dB)", 2, false, -3.8f, 4, true, 6.4f, 4, 3, true, 5, 2.2f, 3, 2.5f, 6, 4.5f, 2, 1, 5.0f, true, -1.0f },
        // 5: Baixo DI Valvulado (Peso Magnetico 50 Hz + Limpeza 300 Hz)
        { "Baixo DI Valvulado (Fat)", 0, false, -2.0f, 3, true, 5.6f, 0, 2, true, 2, 5.2f, 1, 3.5f, 1, 3.0f, 2, 1, 4.2f, true, -1.0f },
        // 6: Violao Acustico Nobre (Classe A Limpo 45 dB + Brilho 16 kHz)
        { "Violao Acustico Nobre", 1, false, 0.0f, 1, true, 4.0f, 1, 2, true, 4, 2.0f, 2, 3.2f, 9, 4.5f, 1, 2, 0.0f, true, 0.0f },
        // 7: De-Ess Cirurgico + Ar 20 kHz
        { "De-Ess Cirurgico + Ar 20k", 1, false, 0.0f, 2, false, 0.0f, 2, 1, true, 4, 0.0f, 4, 1.5f, 10, 5.5f, 0, 1, 6.5f, true, 0.0f }
    }};
    return kPresets;
}

juce::AudioProcessorValueTreeState::ParameterLayout KingLionBoxAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "power", 1 }, "Liga / Bypass", true));

    // Pre Valvulado
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "low_cut", 1 }, "Pre Corte (HPF)",
        juce::StringArray { "FLAT", "80 Hz", "120 Hz" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "phase", 1 }, "Pre Fase 180", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "input_trim", 1 }, "Pre Entrada",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "preamp_gain", 1 }, "Pre Ganho Valvula",
        juce::StringArray { "40 dB", "45 dB", "50 dB", "55 dB", "60 dB" }, 2));

    // Compressor Optico 3:1
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "comp_on", 1 }, "CompComprime 3:1", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "stereo_link", 1 }, "Comp Link Estereo", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "comp_thresh", 1 }, "Comp Threshold",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.05f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "comp_attack", 1 }, "Comp Attack",
        juce::StringArray { "SLOW", "MED SLOW", "MEDIUM", "MED FAST", "FAST" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "comp_release", 1 }, "Comp Release",
        juce::StringArray { "SLOW", "MED SLOW", "MEDIUM", "MED FAST", "FAST" }, 1));

    // EQ Passivo Pultec 33 Bandas
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "eq_on", 1 }, "EQ Ligado", true));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "eq_low_freq", 1 }, "Grave Frequencia",
        juce::StringArray { "20 Hz", "35 Hz", "50 Hz", "70 Hz", "100 Hz", "150 Hz",
                            "200 Hz", "300 Hz", "500 Hz", "700 Hz", "1 kHz" }, 4));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eq_low_boost", 1 }, "Grave Boost",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "eq_mid_freq", 1 }, "Medio Frequencia",
        juce::StringArray { "200 Hz", "300 Hz", "500 Hz", "700 Hz", "1 kHz", "1.5 kHz",
                            "2 kHz", "3 kHz", "4 kHz", "5 kHz", "7 kHz" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eq_mid_dip", 1 }, "Medio Dip (Corte)",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "eq_high_freq", 1 }, "Agudo Frequencia",
        juce::StringArray { "1.5 kHz", "2 kHz", "3 kHz", "4 kHz", "5 kHz", "6.4 kHz",
                            "8 kHz", "10 kHz", "12 kHz", "16 kHz", "20 kHz" }, 8));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eq_high_boost", 1 }, "Agudo Boost",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // De-Esser / Limitador Optico 10:1 + Saida
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "deess_mode", 1 }, "Modo De-Esser / Limitador",
        juce::StringArray { "DE-ESSER", "DESLIGADO", "LIMITADOR" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "deess_freq", 1 }, "De-Esser Frequencia",
        juce::StringArray { "3 kHz", "6 kHz", "9 kHz", "12 kHz" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "deess_thresh", 1 }, "De-Esser Threshold",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.05f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "solo_s", 1 }, "Ouvir S (Sidechain)", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "xfmr_on", 1 }, "Transformador XFMR", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output_trim", 1 }, "Saida",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // Medidor VU (5 posicoes)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "meter_mode", 1 }, "Leitura VU",
        juce::StringArray { "G-R", "ENT", "PRE", "EQ", "D-S" }, 0));

    return { params.begin(), params.end() };
}

void KingLionBoxAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    channelStrip.prepare(sampleRate);
    channelStrip.updateParameters(readCurrentParameters());
}

void KingLionBoxAudioProcessor::releaseResources()
{
    channelStrip.reset();
}

bool KingLionBoxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == layouts.getMainInputChannelSet();
}

kinglion::dsp::KingLionParameters KingLionBoxAudioProcessor::readCurrentParameters() const noexcept
{
    kinglion::dsp::KingLionParameters p {};
    p.powerOn        = apvts.getRawParameterValue("power")->load() > 0.5f;
    p.lowCutMode     = static_cast<int>(apvts.getRawParameterValue("low_cut")->load());
    p.phaseInvert    = apvts.getRawParameterValue("phase")->load() > 0.5f;
    p.inputTrimDb    = apvts.getRawParameterValue("input_trim")->load();
    p.preampGainStep = static_cast<int>(apvts.getRawParameterValue("preamp_gain")->load());

    p.compEngaged     = apvts.getRawParameterValue("comp_on")->load() > 0.5f;
    p.stereoLink      = apvts.getRawParameterValue("stereo_link")->load() > 0.5f;
    p.compThreshold   = apvts.getRawParameterValue("comp_thresh")->load();
    p.compAttackStep  = static_cast<int>(apvts.getRawParameterValue("comp_attack")->load());
    p.compReleaseStep = static_cast<int>(apvts.getRawParameterValue("comp_release")->load());

    p.eqEngaged    = apvts.getRawParameterValue("eq_on")->load() > 0.5f;
    p.lowFreqStep  = static_cast<int>(apvts.getRawParameterValue("eq_low_freq")->load());
    p.lowBoostDb   = apvts.getRawParameterValue("eq_low_boost")->load();
    p.midFreqStep  = static_cast<int>(apvts.getRawParameterValue("eq_mid_freq")->load());
    p.midDipDb     = apvts.getRawParameterValue("eq_mid_dip")->load();
    p.highFreqStep = static_cast<int>(apvts.getRawParameterValue("eq_high_freq")->load());
    p.highBoostDb  = apvts.getRawParameterValue("eq_high_boost")->load();

    p.deEssMode      = static_cast<int>(apvts.getRawParameterValue("deess_mode")->load());
    p.deEssFreqStep  = static_cast<int>(apvts.getRawParameterValue("deess_freq")->load());
    p.deEssThreshold = apvts.getRawParameterValue("deess_thresh")->load();
    p.soloSibilance  = apvts.getRawParameterValue("solo_s")->load() > 0.5f;
    p.xfmrEngaged    = apvts.getRawParameterValue("xfmr_on")->load() > 0.5f;
    p.outputTrimDb   = apvts.getRawParameterValue("output_trim")->load();
    p.meterMode      = static_cast<int>(apvts.getRawParameterValue("meter_mode")->load());
    return p;
}

void KingLionBoxAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || totalNumInputChannels <= 0)
        return;

    channelStrip.updateParameters(readCurrentParameters());

    float* left  = buffer.getWritePointer(0);
    float* right = (totalNumInputChannels > 1) ? buffer.getWritePointer(1) : nullptr;
    channelStrip.processBlock(left, right, numSamples);
}

int KingLionBoxAudioProcessor::getNumPrograms()
{
    return static_cast<int>(getFactoryPresets().size());
}

const juce::String KingLionBoxAudioProcessor::getProgramName(int index)
{
    const auto& presets = getFactoryPresets();
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return juce::String::fromUTF8(presets[static_cast<size_t>(index)].name);
    return {};
}

void KingLionBoxAudioProcessor::changeProgramName(int, const juce::String&) {}

void KingLionBoxAudioProcessor::setCurrentProgram(int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return;

    currentPresetIndex = index;
    const auto& pr = presets[static_cast<size_t>(index)];

    auto setParamValue = [this](const char* paramId, float plainVal)
    {
        if (auto* param = apvts.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(plainVal));
    };

    setParamValue("power",        1.0f);
    setParamValue("low_cut",      static_cast<float>(pr.lowCut));
    setParamValue("phase",        pr.phase ? 1.0f : 0.0f);
    setParamValue("input_trim",   pr.inputDb);
    setParamValue("preamp_gain",  static_cast<float>(pr.preampGain));
    setParamValue("comp_on",      pr.compOn ? 1.0f : 0.0f);
    setParamValue("comp_thresh",  pr.compThresh);
    setParamValue("comp_attack",  static_cast<float>(pr.compAtk));
    setParamValue("comp_release", static_cast<float>(pr.compRel));
    setParamValue("eq_on",        pr.eqOn ? 1.0f : 0.0f);
    setParamValue("eq_low_freq",  static_cast<float>(pr.lowFreq));
    setParamValue("eq_low_boost", pr.lowBoost);
    setParamValue("eq_mid_freq",  static_cast<float>(pr.midFreq));
    setParamValue("eq_mid_dip",   pr.midDip);
    setParamValue("eq_high_freq", static_cast<float>(pr.highFreq));
    setParamValue("eq_high_boost", pr.highBoost);
    setParamValue("deess_mode",   static_cast<float>(pr.deEssMode));
    setParamValue("deess_freq",   static_cast<float>(pr.deEssFreq));
    setParamValue("deess_thresh", pr.deEssThresh);
    setParamValue("solo_s",       0.0f);
    setParamValue("xfmr_on",      pr.xfmrOn ? 1.0f : 0.0f);
    setParamValue("output_trim",  pr.outDb);
}

void KingLionBoxAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("presetIndex", currentPresetIndex, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void KingLionBoxAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto restored = juce::ValueTree::fromXml(*xmlState);
        currentPresetIndex = restored.getProperty("presetIndex", 0);
        apvts.replaceState(restored);
    }
}

juce::AudioProcessorEditor* KingLionBoxAudioProcessor::createEditor()
{
    return new KingLionBoxAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KingLionBoxAudioProcessor();
}
