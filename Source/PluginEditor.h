#pragma once

#include "PluginProcessor.h"
#include "GUI/KingLionLookAndFeel.h"
#include "GUI/VuMeterComponent.h"

class KingLionBoxAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit KingLionBoxAudioProcessorEditor(KingLionBoxAudioProcessor&);
    ~KingLionBoxAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    /**
     * Canvas logico fixo de 1080x650 que sofre transformacao vetorial HiDPI
     * proporcional quando o usuario redimensiona a janela na DAW.
     */
    class RackCanvas final : public juce::Component
    {
    public:
        explicit RackCanvas(KingLionBoxAudioProcessor& proc) : processor(proc) {}
        void paint(juce::Graphics& g) override;

    private:
        KingLionBoxAudioProcessor& processor;
    };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    KingLionBoxAudioProcessor& audioProcessor;
    kinglion::gui::KingLionLookAndFeel lookAndFeel;
    RackCanvas rackCanvas;

    // Medidor VU central + Presets + Master
    kinglion::gui::VuMeterComponent vuMeter;
    juce::ComboBox presetBox;
    juce::Slider meterModeKnob;
    juce::ToggleButton powerToggle;
    juce::ToggleButton xfmrToggle;

    // Secao 1: Pre Valvulado
    kinglion::gui::ThreeWayToggleSwitch lowCutSwitch;
    juce::ToggleButton phaseToggle;
    juce::Slider inputKnob;
    juce::Slider preampGainKnob;

    // Secao 2: Compressor Optico 3:1
    juce::ToggleButton compOnToggle;
    juce::ToggleButton stereoLinkToggle;
    juce::Slider compThreshKnob;
    juce::Slider compAttackKnob;
    juce::Slider compReleaseKnob;

    // Secao 3: EQ Passivo Pultec 33 Frequencias
    juce::ToggleButton eqOnToggle;
    juce::Slider eqLowFreqKnob,  eqLowBoostKnob;
    juce::Slider eqMidFreqKnob,  eqMidDipKnob;
    juce::Slider eqHighFreqKnob, eqHighBoostKnob;

    // Secao 4: De-Esser / Limitador 10:1 + Saida
    kinglion::gui::ThreeWayToggleSwitch deEssModeSwitch;
    juce::ToggleButton soloSToggle;
    juce::Slider deEssFreqKnob;
    juce::Slider deEssThreshKnob;
    juce::Slider outputKnob;

    // Attachments APVTS (automacao em tempo real no Pro Tools / VST3 / AU)
    std::unique_ptr<ButtonAttachment> powerAttach, xfmrAttach, phaseAttach;
    std::unique_ptr<ButtonAttachment> compOnAttach, stereoLinkAttach, eqOnAttach, soloSAttach;

    std::unique_ptr<SliderAttachment> meterModeAttach, inputAttach, preampGainAttach;
    std::unique_ptr<SliderAttachment> compThreshAttach, compAttackAttach, compReleaseAttach;
    std::unique_ptr<SliderAttachment> eqLowFreqAttach, eqLowBoostAttach;
    std::unique_ptr<SliderAttachment> eqMidFreqAttach, eqMidDipAttach;
    std::unique_ptr<SliderAttachment> eqHighFreqAttach, eqHighBoostAttach;
    std::unique_ptr<SliderAttachment> deEssFreqAttach, deEssThreshAttach, outputAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KingLionBoxAudioProcessorEditor)
};
