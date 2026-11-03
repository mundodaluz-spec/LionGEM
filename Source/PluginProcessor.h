#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/KingLionChannelStrip.h"
#include <array>

class KingLionBoxAudioProcessor final : public juce::AudioProcessor
{
public:
    struct FactoryPreset
    {
        const char* name;
        int lowCut;
        bool phase;
        float inputDb;
        int preampGain;
        bool compOn;
        float compThresh;
        int compAtk;
        int compRel;
        bool eqOn;
        int lowFreq;
        float lowBoost;
        int midFreq;
        float midDip;
        int highFreq;
        float highBoost;
        int deEssMode;
        int deEssFreq;
        float deEssThresh;
        bool xfmrOn;
        float outDb;
    };

    KingLionBoxAudioProcessor();
    ~KingLionBoxAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPresetIndex; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const kinglion::dsp::KingLionChannelStrip& getDSP() const noexcept { return channelStrip; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static const std::array<FactoryPreset, 8>& getFactoryPresets();

private:
    kinglion::dsp::KingLionParameters readCurrentParameters() const noexcept;

    juce::AudioProcessorValueTreeState apvts;
    kinglion::dsp::KingLionChannelStrip channelStrip;
    int currentPresetIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KingLionBoxAudioProcessor)
};
