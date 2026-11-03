#include "PluginEditor.h"

using namespace kinglion::gui;

namespace
{
constexpr int kDesignWidth  = 1080;
constexpr int kDesignHeight = 650;

void setupRotaryKnob(juce::Slider& s, int numTicks = 11)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setPopupDisplayEnabled(true, true, nullptr);
    s.getProperties().set("numTicks", numTicks);
}

void drawValuePill(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, bool highlight = false)
{
    g.setColour(juce::Colour(0xff0b0c0e).withAlpha(0.88f));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(highlight ? Palette::accentAmber.withAlpha(0.75f) : Palette::panelTrim.withAlpha(0.45f));
    g.drawRoundedRectangle(r, 4.0f, 0.9f);

    g.setFont(makeFont(10.5f, true, false));
    g.setColour(highlight ? Palette::accentAmber : Palette::textGold);
    g.drawText(text, r, juce::Justification::centred, false);
}
} // namespace

KingLionBoxAudioProcessorEditor::KingLionBoxAudioProcessorEditor(KingLionBoxAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      rackCanvas(p),
      lowCutSwitch(p.getAPVTS().getParameter("low_cut"), true),
      deEssModeSwitch(p.getAPVTS().getParameter("deess_mode"), false)
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(rackCanvas);

    auto& apvts = audioProcessor.getAPVTS();

    // VU + Barra de Presets + Master
    rackCanvas.addAndMakeVisible(vuMeter);

    const auto& presets = KingLionBoxAudioProcessor::getFactoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        presetBox.addItem(juce::String::fromUTF8(presets[i].name), static_cast<int>(i + 1));
    presetBox.setSelectedId(audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]()
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0)
            audioProcessor.setCurrentProgram(idx);
    };
    rackCanvas.addAndMakeVisible(presetBox);

    setupRotaryKnob(meterModeKnob, 5);
    rackCanvas.addAndMakeVisible(meterModeKnob);
    meterModeAttach = std::make_unique<SliderAttachment>(apvts, "meter_mode", meterModeKnob);

    rackCanvas.addAndMakeVisible(powerToggle);
    powerAttach = std::make_unique<ButtonAttachment>(apvts, "power", powerToggle);

    rackCanvas.addAndMakeVisible(xfmrToggle);
    xfmrAttach = std::make_unique<ButtonAttachment>(apvts, "xfmr_on", xfmrToggle);

    // 1. Pre Valvulado
    rackCanvas.addAndMakeVisible(lowCutSwitch);

    phaseToggle.getProperties().set("invertLever", true); // 0 deg no topo, 180 deg embaixo
    rackCanvas.addAndMakeVisible(phaseToggle);
    phaseAttach = std::make_unique<ButtonAttachment>(apvts, "phase", phaseToggle);

    setupRotaryKnob(inputKnob, 11);
    rackCanvas.addAndMakeVisible(inputKnob);
    inputAttach = std::make_unique<SliderAttachment>(apvts, "input_trim", inputKnob);

    setupRotaryKnob(preampGainKnob, 5);
    rackCanvas.addAndMakeVisible(preampGainKnob);
    preampGainAttach = std::make_unique<SliderAttachment>(apvts, "preamp_gain", preampGainKnob);

    // 2. Compressor Optico 3:1
    rackCanvas.addAndMakeVisible(compOnToggle);
    compOnAttach = std::make_unique<ButtonAttachment>(apvts, "comp_on", compOnToggle);

    rackCanvas.addAndMakeVisible(stereoLinkToggle);
    stereoLinkAttach = std::make_unique<ButtonAttachment>(apvts, "stereo_link", stereoLinkToggle);

    setupRotaryKnob(compThreshKnob, 11);
    rackCanvas.addAndMakeVisible(compThreshKnob);
    compThreshAttach = std::make_unique<SliderAttachment>(apvts, "comp_thresh", compThreshKnob);

    setupRotaryKnob(compAttackKnob, 5);
    rackCanvas.addAndMakeVisible(compAttackKnob);
    compAttackAttach = std::make_unique<SliderAttachment>(apvts, "comp_attack", compAttackKnob);

    setupRotaryKnob(compReleaseKnob, 5);
    rackCanvas.addAndMakeVisible(compReleaseKnob);
    compReleaseAttach = std::make_unique<SliderAttachment>(apvts, "comp_release", compReleaseKnob);

    // 3. EQ Passivo Pultec 33 Frequencias
    rackCanvas.addAndMakeVisible(eqOnToggle);
    eqOnAttach = std::make_unique<ButtonAttachment>(apvts, "eq_on", eqOnToggle);

    for (auto* k : { &eqLowFreqKnob, &eqMidFreqKnob, &eqHighFreqKnob })
    {
        setupRotaryKnob(*k, 11);
        rackCanvas.addAndMakeVisible(*k);
    }
    for (auto* k : { &eqLowBoostKnob, &eqMidDipKnob, &eqHighBoostKnob })
    {
        setupRotaryKnob(*k, 11);
        rackCanvas.addAndMakeVisible(*k);
    }

    eqLowFreqAttach   = std::make_unique<SliderAttachment>(apvts, "eq_low_freq",   eqLowFreqKnob);
    eqLowBoostAttach  = std::make_unique<SliderAttachment>(apvts, "eq_low_boost",  eqLowBoostKnob);
    eqMidFreqAttach   = std::make_unique<SliderAttachment>(apvts, "eq_mid_freq",   eqMidFreqKnob);
    eqMidDipAttach    = std::make_unique<SliderAttachment>(apvts, "eq_mid_dip",    eqMidDipKnob);
    eqHighFreqAttach  = std::make_unique<SliderAttachment>(apvts, "eq_high_freq",  eqHighFreqKnob);
    eqHighBoostAttach = std::make_unique<SliderAttachment>(apvts, "eq_high_boost", eqHighBoostKnob);

    // 4. De-Esser / Limitador + Saida
    rackCanvas.addAndMakeVisible(deEssModeSwitch);
    rackCanvas.addAndMakeVisible(soloSToggle);
    soloSAttach = std::make_unique<ButtonAttachment>(apvts, "solo_s", soloSToggle);

    setupRotaryKnob(deEssFreqKnob, 4);
    rackCanvas.addAndMakeVisible(deEssFreqKnob);
    deEssFreqAttach = std::make_unique<SliderAttachment>(apvts, "deess_freq", deEssFreqKnob);

    setupRotaryKnob(deEssThreshKnob, 11);
    rackCanvas.addAndMakeVisible(deEssThreshKnob);
    deEssThreshAttach = std::make_unique<SliderAttachment>(apvts, "deess_thresh", deEssThreshKnob);

    setupRotaryKnob(outputKnob, 11);
    rackCanvas.addAndMakeVisible(outputKnob);
    outputAttach = std::make_unique<SliderAttachment>(apvts, "output_trim", outputKnob);

    // Posicionamento geometrico em coordenadas logicas (1080 x 650)
    rackCanvas.setBounds(0, 0, kDesignWidth, kDesignHeight);

    // Coluna Central Master (x = 388..692)
    vuMeter.setBounds(402, 22, 276, 146);
    presetBox.setBounds(414, 273, 252, 25);
    xfmrToggle.setBounds(415, 314, 30, 36);
    meterModeKnob.setBounds(514, 310, 48, 48);
    powerToggle.setBounds(630, 314, 30, 36);

    // Subpainel 1: Pre Valvulado (x = 52..376, y = 20..356)
    lowCutSwitch.setBounds(100, 82, 32, 46);
    phaseToggle.setBounds(268, 82, 32, 46);
    inputKnob.setBounds(96, 198, 84, 84);
    preampGainKnob.setBounds(252, 198, 84, 84);

    // Subpainel 2: Compressor Optico (x = 704..1028, y = 20..356)
    compOnToggle.setBounds(740, 62, 30, 40);
    stereoLinkToggle.setBounds(952, 62, 30, 40);
    compThreshKnob.setBounds(822, 124, 88, 88);
    compAttackKnob.setBounds(726, 258, 68, 68);
    compReleaseKnob.setBounds(938, 258, 68, 68);

    // Subpainel 3: EQ Passivo Pultec (x = 52..630, y = 374..630)
    eqOnToggle.setBounds(86, 434, 30, 40);
    eqLowFreqKnob.setBounds(196, 424, 62, 62);
    eqLowBoostKnob.setBounds(191, 530, 72, 72);
    eqMidFreqKnob.setBounds(352, 424, 62, 62);
    eqMidDipKnob.setBounds(347, 530, 72, 72);
    eqHighFreqKnob.setBounds(508, 424, 62, 62);
    eqHighBoostKnob.setBounds(503, 530, 72, 72);

    // Subpainel 4: De-Esser / Limitador + Saida (x = 646..1028, y = 374..630)
    deEssModeSwitch.setBounds(668, 432, 32, 46);
    soloSToggle.setBounds(668, 550, 32, 42);
    deEssFreqKnob.setBounds(806, 424, 62, 62);
    deEssThreshKnob.setBounds(801, 530, 72, 72);
    outputKnob.setBounds(918, 468, 84, 84);

    setResizable(true, true);
    setResizeLimits(864, 520, 1620, 975);
    getConstrainer()->setFixedAspectRatio(static_cast<double>(kDesignWidth) / static_cast<double>(kDesignHeight));
    setSize(kDesignWidth, kDesignHeight);

    startTimerHz(30);
}

KingLionBoxAudioProcessorEditor::~KingLionBoxAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void KingLionBoxAudioProcessorEditor::timerCallback()
{
    auto& apvts = audioProcessor.getAPVTS();
    const bool pwr = apvts.getRawParameterValue("power")->load() > 0.5f;
    const int mMode = static_cast<int>(apvts.getRawParameterValue("meter_mode")->load());

    vuMeter.setMeterState(audioProcessor.getDSP().getVuNeedleDb(), mMode, pwr);
    lowCutSwitch.repaint();
    deEssModeSwitch.repaint();
    rackCanvas.repaint();
}

void KingLionBoxAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0c0a));
}

void KingLionBoxAudioProcessorEditor::resized()
{
    const float scaleX = static_cast<float>(getWidth())  / static_cast<float>(kDesignWidth);
    const float scaleY = static_cast<float>(getHeight()) / static_cast<float>(kDesignHeight);
    const float scale  = std::min(scaleX, scaleY);
    rackCanvas.setTransform(juce::AffineTransform::scale(scale));
}

void KingLionBoxAudioProcessorEditor::RackCanvas::paint(juce::Graphics& g)
{
    const auto full = getLocalBounds().toFloat();
    auto& apvts = processor.getAPVTS();

    const bool powerOn  = apvts.getRawParameterValue("power")->load() > 0.5f;
    const bool soloS    = apvts.getRawParameterValue("solo_s")->load() > 0.5f;
    const bool xfmrOn   = apvts.getRawParameterValue("xfmr_on")->load() > 0.5f;
    const int gainIdx   = static_cast<int>(apvts.getRawParameterValue("preamp_gain")->load());
    const int atkIdx    = static_cast<int>(apvts.getRawParameterValue("comp_attack")->load());
    const int relIdx    = static_cast<int>(apvts.getRawParameterValue("comp_release")->load());
    const int lowFIdx   = static_cast<int>(apvts.getRawParameterValue("eq_low_freq")->load());
    const int midFIdx   = static_cast<int>(apvts.getRawParameterValue("eq_mid_freq")->load());
    const int highFIdx  = static_cast<int>(apvts.getRawParameterValue("eq_high_freq")->load());
    const float inDb    = apvts.getRawParameterValue("input_trim")->load();
    const float outDb   = apvts.getRawParameterValue("output_trim")->load();
    const float compGr  = processor.getDSP().getCompGrDb();

    // 1. Chassi Principal Ouro/Champagne Anodizado Escovado 3U
    juce::ColourGradient chassisGrad(Palette::goldLight, 0.0f, 0.0f,
                                     Palette::goldDark,  0.0f, full.getHeight(), false);
    chassisGrad.addColour(0.28, Palette::goldMid);
    chassisGrad.addColour(0.62, juce::Colour(0xffe2c174));
    g.setGradientFill(chassisGrad);
    g.fillRect(full);

    // Textura horizontal sutil de aluminio escovado
    for (int y = 0; y < static_cast<int>(full.getHeight()); y += 2)
    {
        const float alpha = (y % 6 == 0) ? 0.045f : 0.020f;
        g.setColour((y % 4 == 0) ? juce::Colours::black.withAlpha(alpha)
                                 : juce::Colours::white.withAlpha(alpha));
        g.drawHorizontalLine(y, 0.0f, full.getWidth());
    }

    // Abas laterais de Rack 19" 3U com furos ovais e parafusos sextavados
    const float earW = 36.0f;
    g.setColour(juce::Colours::black.withAlpha(0.16f));
    g.fillRect(0.0f, 0.0f, earW, full.getHeight());
    g.fillRect(full.getWidth() - earW, 0.0f, earW, full.getHeight());
    g.setColour(Palette::goldDeepBorder.withAlpha(0.65f));
    g.drawVerticalLine(static_cast<int>(earW), 0.0f, full.getHeight());
    g.drawVerticalLine(static_cast<int>(full.getWidth() - earW), 0.0f, full.getHeight());

    for (float ry : { 64.0f, 232.0f, 418.0f, 586.0f })
    {
        for (float rx : { 18.0f, full.getWidth() - 18.0f })
        {
            g.setColour(juce::Colour(0xff1b160c));
            g.fillRoundedRectangle(rx - 9.0f, ry - 6.5f, 18.0f, 13.0f, 6.0f);
            drawMachinedScrew(g, rx, ry, 5.6f);
        }
    }

    // Chanfro superior/inferior de luz no painel frontal
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.drawHorizontalLine(1, 0.0f, full.getWidth());
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.drawHorizontalLine(static_cast<int>(full.getHeight()) - 2, 0.0f, full.getWidth());

    // 2. Os 4 Subpaineis Obsidiana Parafusados
    drawSubPanel(g, {  52.0f,  20.0f, 324.0f, 336.0f }, juce::String::fromUTF8("PRÉ VALVULADO CLASSE A"));
    drawSubPanel(g, { 704.0f,  20.0f, 324.0f, 336.0f }, juce::String::fromUTF8("COMPRESSOR ÓPTICO VACTROL"));
    drawSubPanel(g, {  52.0f, 374.0f, 578.0f, 256.0f }, juce::String::fromUTF8("EQ PASSIVO PULTEC • 33 BANDAS"));
    drawSubPanel(g, { 646.0f, 374.0f, 382.0f, 256.0f }, juce::String::fromUTF8("DE-ESSER • LIMITADOR 10:1"));

    // 3. Coluna Central Master (Brasao do Leao, Tipografia KINGLION BOX, VU Selector e LED Rubi)
    drawGoldenLionMedallion(g, 540.0f, 204.0f, 28.5f);

    g.setFont(makeFont(21.0f, true, true));
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.drawText("KINGLION BOX", { 390.0f, 237.0f, 300.0f, 24.0f }, juce::Justification::centred, false);
    g.setColour(juce::Colour(0xff2c1d06));
    g.drawText("KINGLION BOX", { 390.0f, 236.0f, 300.0f, 24.0f }, juce::Justification::centred, false);

    g.setFont(makeFont(9.5f, true, false));
    g.setColour(juce::Colour(0xff422d0b));
    g.drawText(juce::String::fromUTF8("TUBE VOICE CHANNEL  •  WYATT SOUNDS"),
               { 390.0f, 256.0f, 300.0f, 14.0f }, juce::Justification::centred, false);

    // Chave XFMR + Rotulos do Seletor VU de 5 posicoes + LED Rubi + LIGA/DESL.
    g.setFont(makeFont(9.5f, true, false));
    g.setColour(xfmrOn ? juce::Colour(0xff2b1b04) : juce::Colour(0xff5e4922));
    g.drawText("XFMR", { 404.0f, 301.0f, 52.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText("FERRO", { 404.0f, 349.0f, 52.0f, 12.0f }, juce::Justification::centred, false);

    g.setColour(juce::Colour(0xff2c1d06));
    g.drawText("G-R",  { 474.0f, 330.0f, 34.0f, 12.0f }, juce::Justification::centredRight, false);
    g.drawText("ENT",  { 486.0f, 308.0f, 34.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText(juce::String::fromUTF8("PRÉ"), { 521.0f, 299.0f, 34.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText("EQ",   { 554.0f, 308.0f, 30.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText("D-S",  { 566.0f, 330.0f, 34.0f, 12.0f }, juce::Justification::centredLeft, false);

    // Joia LED Rubi (Power Ready)
    const float ledCx = 609.0f, ledCy = 332.0f;
    if (powerOn)
    {
        juce::ColourGradient halo(Palette::accentRed.withAlpha(0.55f), ledCx, ledCy,
                                  Palette::accentRed.withAlpha(0.0f),  ledCx + 15.0f, ledCy, true);
        g.setGradientFill(halo);
        g.fillEllipse(ledCx - 15.0f, ledCy - 15.0f, 30.0f, 30.0f);
    }
    g.setColour(juce::Colour(0xff2a1e0d));
    g.fillEllipse(ledCx - 6.5f, ledCy - 6.5f, 13.0f, 13.0f);
    g.setColour(powerOn ? juce::Colour(0xffff3b2f) : juce::Colour(0xff4a1916));
    g.fillEllipse(ledCx - 5.0f, ledCy - 5.0f, 10.0f, 10.0f);
    g.setColour(juce::Colours::white.withAlpha(powerOn ? 0.7f : 0.2f));
    g.fillEllipse(ledCx - 2.5f, ledCy - 3.0f, 3.2f, 3.2f);

    g.setFont(makeFont(9.5f, true, false));
    g.setColour(juce::Colour(0xff2c1d06));
    g.drawText("LIGA",  { 622.0f, 301.0f, 46.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText("DESL.", { 622.0f, 349.0f, 46.0f, 12.0f }, juce::Justification::centred, false);

    // 4. Legendas Subpainel 1: PRE VALVULADO
    g.setFont(makeFont(10.5f, true, false));
    g.setColour(Palette::textCream);
    g.drawText("CORTE", { 92.0f, 58.0f, 80.0f, 14.0f }, juce::Justification::centredLeft, false);
    g.drawText("FASE",  { 260.0f, 58.0f, 76.0f, 14.0f }, juce::Justification::centredLeft, false);

    g.setFont(makeFont(9.5f, false, false));
    g.setColour(Palette::textMuted);
    g.drawText("120 Hz", { 134.0f,  80.0f, 48.0f, 13.0f }, juce::Justification::centredLeft, false);
    g.drawText("80 Hz",  { 134.0f,  98.0f, 48.0f, 13.0f }, juce::Justification::centredLeft, false);
    g.drawText("FLAT",   { 134.0f, 116.0f, 48.0f, 13.0f }, juce::Justification::centredLeft, false);

    g.drawText(juce::String::fromUTF8("0°"),   { 302.0f,  82.0f, 40.0f, 13.0f }, juce::Justification::centredLeft, false);
    g.drawText(juce::String::fromUTF8("180°"), { 302.0f, 114.0f, 40.0f, 13.0f }, juce::Justification::centredLeft, false);

    g.setFont(makeFont(11.0f, true, false));
    g.setColour(Palette::textCream);
    g.drawText("ENTRADA",  {  84.0f, 166.0f, 108.0f, 16.0f }, juce::Justification::centred, false);
    g.drawText("GANHO dB", { 240.0f, 166.0f, 108.0f, 16.0f }, juce::Justification::centred, false);

    g.setFont(makeFont(9.0f, false, false));
    g.setColour(Palette::textMuted);
    g.drawText("-24", {  74.0f, 276.0f, 28.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText("0",   { 148.0f, 184.0f, 24.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText("+12", { 176.0f, 276.0f, 28.0f, 12.0f }, juce::Justification::centred, false);

    // Escala escalonada lateral 40..60 dB com realce ambar no estagio de valvula ativo
    static constexpr const char* kGainLabels[5] = { "40", "45", "50", "55", "60" };
    static constexpr float kGainY[5] = { 274.0f, 254.0f, 234.0f, 214.0f, 194.0f };
    for (int i = 0; i < 5; ++i)
    {
        const bool active = (i == gainIdx);
        g.setFont(makeFont(active ? 10.5f : 9.5f, active, false));
        g.setColour(active ? Palette::accentAmber : Palette::textMuted);
        g.drawText(kGainLabels[i], { 214.0f, kGainY[i], 24.0f, 13.0f }, juce::Justification::centredRight, false);
        g.drawLine(241.0f, kGainY[i] + 6.5f, 255.0f, kGainY[i] + 6.5f, active ? 1.5f : 0.8f);
    }

    drawValuePill(g, { 104.0f, 294.0f, 68.0f, 18.0f },
                  (inDb >= 0.0f ? "+" : "") + juce::String(inDb, 1) + " dB");
    drawValuePill(g, { 260.0f, 294.0f, 68.0f, 18.0f },
                  juce::String(40 + gainIdx * 5) + " dB NFB", gainIdx >= 3);

    // 5. Legendas Subpainel 2: COMPRESSOR OPTICO 3:1
    g.setFont(makeFont(9.5f, true, false));
    g.setColour(Palette::textCream);
    g.drawText("COMPRIME 3:1", { 772.0f, 60.0f, 88.0f, 13.0f }, juce::Justification::centredLeft, false);
    g.setColour(Palette::textMuted);
    g.drawText("BYPASS",       { 772.0f, 88.0f, 88.0f, 13.0f }, juce::Justification::centredLeft, false);

    g.setColour(Palette::textCream);
    g.drawText("LINK",         { 884.0f, 60.0f, 64.0f, 13.0f }, juce::Justification::centredRight, false);
    g.setColour(Palette::textMuted);
    g.drawText("SEPARADO",     { 874.0f, 88.0f, 74.0f, 13.0f }, juce::Justification::centredRight, false);

    g.setFont(makeFont(11.0f, true, false));
    g.setColour(Palette::textCream);
    g.drawText("THRESHOLD", { 812.0f, 104.0f, 108.0f, 15.0f }, juce::Justification::centred, false);

    if (compGr > 0.15f)
        drawValuePill(g, { 830.0f, 214.0f, 72.0f, 17.0f }, "-" + juce::String(compGr, 1) + " dB", true);

    g.setFont(makeFont(10.5f, true, false));
    g.setColour(Palette::textCream);
    g.drawText("ATTACK",  { 722.0f, 238.0f, 76.0f, 14.0f }, juce::Justification::centred, false);
    g.drawText("RELEASE", { 934.0f, 238.0f, 76.0f, 14.0f }, juce::Justification::centred, false);

    // Escada central compartilhada FAST .. SLOW
    static constexpr const char* kRateNames[5] = { "SLOW", "MED SLOW", "MEDIUM", "MED FAST", "FAST" };
    static constexpr float kRateY[5] = { 320.0f, 302.0f, 284.0f, 266.0f, 248.0f };
    for (int i = 0; i < 5; ++i)
    {
        const bool matchAtk = (i == atkIdx);
        const bool matchRel = (i == relIdx);
        g.setFont(makeFont(8.8f, matchAtk || matchRel, false));
        g.setColour((matchAtk || matchRel) ? Palette::textGold : Palette::textMuted);
        g.drawText(kRateNames[i], { 822.0f, kRateY[i], 88.0f, 12.0f }, juce::Justification::centred, false);

        g.setColour(matchAtk ? Palette::accentAmber : Palette::textMuted.withAlpha(0.35f));
        g.drawLine(794.0f, kRateY[i] + 6.0f, 822.0f, kRateY[i] + 6.0f, matchAtk ? 1.4f : 0.8f);

        g.setColour(matchRel ? Palette::accentAmber : Palette::textMuted.withAlpha(0.35f));
        g.drawLine(910.0f, kRateY[i] + 6.0f, 938.0f, kRateY[i] + 6.0f, matchRel ? 1.4f : 0.8f);
    }

    // 6. Legendas Subpainel 3: EQ PASSIVO PULTEC (33 FREQUENCIAS)
    g.setFont(makeFont(9.5f, true, false));
    g.setColour(Palette::textCream);
    g.drawText("LIGADO", { 118.0f, 434.0f, 54.0f, 13.0f }, juce::Justification::centredLeft, false);
    g.setColour(Palette::textMuted);
    g.drawText("DESL.",  { 118.0f, 462.0f, 54.0f, 13.0f }, juce::Justification::centredLeft, false);

    g.setFont(makeFont(9.5f, true, false));
    g.setColour(Palette::textGold.withAlpha(0.85f));
    g.drawText("FREQ",       { 74.0f, 496.0f, 76.0f, 13.0f }, juce::Justification::centredLeft, false);
    g.drawText("GANHO (dB)", { 74.0f, 560.0f, 84.0f, 13.0f }, juce::Justification::centredLeft, false);

    g.setFont(makeFont(11.0f, true, false));
    g.setColour(Palette::textCream);
    g.drawText("GRAVE (BOOST)",              { 168.0f, 404.0f, 118.0f, 15.0f }, juce::Justification::centred, false);
    g.drawText(juce::String::fromUTF8("MÉDIO (DIP)"), { 324.0f, 404.0f, 118.0f, 15.0f }, juce::Justification::centred, false);
    g.drawText("AGUDO (BOOST)",              { 480.0f, 404.0f, 118.0f, 15.0f }, juce::Justification::centred, false);

    static constexpr const char* kLowTxt[11]  = { "20 Hz","35 Hz","50 Hz","70 Hz","100 Hz","150 Hz","200 Hz","300 Hz","500 Hz","700 Hz","1 kHz" };
    static constexpr const char* kMidTxt[11]  = { "200 Hz","300 Hz","500 Hz","700 Hz","1 kHz","1.5 kHz","2 kHz","3 kHz","4 kHz","5 kHz","7 kHz" };
    static constexpr const char* kHighTxt[11] = { "1.5 kHz","2 kHz","3 kHz","4 kHz","5 kHz","6.4 kHz","8 kHz","10 kHz","12 kHz","16 kHz","20 kHz" };

    drawValuePill(g, { 193.0f, 493.0f, 68.0f, 17.0f }, kLowTxt[juce::jlimit(0, 10, lowFIdx)]);
    drawValuePill(g, { 349.0f, 493.0f, 68.0f, 17.0f }, kMidTxt[juce::jlimit(0, 10, midFIdx)]);
    drawValuePill(g, { 505.0f, 493.0f, 68.0f, 17.0f }, kHighTxt[juce::jlimit(0, 10, highFIdx)]);

    g.setFont(makeFont(8.8f, false, false));
    g.setColour(Palette::textMuted);
    for (float cx : { 227.0f, 539.0f })
    {
        g.drawText("0",   { cx - 44.0f, 594.0f, 22.0f, 11.0f }, juce::Justification::centred, false);
        g.drawText("+5",  { cx - 12.0f, 516.0f, 24.0f, 11.0f }, juce::Justification::centred, false);
        g.drawText("+10", { cx + 22.0f, 594.0f, 26.0f, 11.0f }, juce::Justification::centred, false);
    }
    g.drawText("0",   { 383.0f - 44.0f, 594.0f, 22.0f, 11.0f }, juce::Justification::centred, false);
    g.drawText("-5",  { 383.0f - 12.0f, 516.0f, 24.0f, 11.0f }, juce::Justification::centred, false);
    g.drawText("-10", { 383.0f + 22.0f, 594.0f, 26.0f, 11.0f }, juce::Justification::centred, false);

    // 7. Legendas Subpainel 4: DE-ESSER / LIMITADOR 10:1 + SAIDA
    g.setFont(makeFont(9.2f, true, false));
    const int dsMode = static_cast<int>(apvts.getRawParameterValue("deess_mode")->load());
    g.setColour(dsMode == 0 ? Palette::accentAmber : Palette::textMuted);
    g.drawText("DE-ESSER",  { 702.0f, 430.0f, 76.0f, 13.0f }, juce::Justification::centredLeft, false);
    g.setColour(dsMode == 1 ? Palette::textCream : Palette::textMuted);
    g.drawText("DESLIGADO", { 702.0f, 449.0f, 76.0f, 13.0f }, juce::Justification::centredLeft, false);
    g.setColour(dsMode == 2 ? Palette::accentAmber : Palette::textMuted);
    g.drawText("LIMITADOR", { 702.0f, 468.0f, 76.0f, 13.0f }, juce::Justification::centredLeft, false);

    g.setColour(soloS ? Palette::accentAmber : Palette::textCream);
    g.drawText("OUVIR S", { 702.0f, 548.0f, 70.0f, 13.0f }, juce::Justification::centredLeft, false);
    g.setColour(!soloS ? Palette::textCream : Palette::textMuted);
    g.drawText("NORMAL",  { 702.0f, 578.0f, 70.0f, 13.0f }, juce::Justification::centredLeft, false);

    g.setFont(makeFont(9.0f, true, false));
    g.setColour(Palette::textGold);
    g.drawText("3k",  { 780.0f, 476.0f, 24.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText("6k",  { 792.0f, 414.0f, 24.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText("9k",  { 858.0f, 414.0f, 24.0f, 12.0f }, juce::Justification::centred, false);
    g.drawText("12k", { 868.0f, 476.0f, 26.0f, 12.0f }, juce::Justification::centred, false);

    g.setFont(makeFont(10.0f, true, false));
    g.setColour(Palette::textCream);
    g.drawText("THRESHOLD", { 792.0f, 604.0f, 90.0f, 14.0f }, juce::Justification::centred, false);

    g.setFont(makeFont(11.0f, true, false));
    g.drawText(juce::String::fromUTF8("SAÍDA"), { 914.0f, 444.0f, 92.0f, 16.0f }, juce::Justification::centred, false);
    drawValuePill(g, { 926.0f, 562.0f, 68.0f, 18.0f },
                  (outDb >= 0.0f ? "+" : "") + juce::String(outDb, 1) + " dB");
}
