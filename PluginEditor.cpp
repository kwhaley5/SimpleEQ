/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) {
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    auto enabled = slider.isEnabled();

    g.setColour(enabled ? Colour(97u, 18u, 167u) : Colours::darkgrey);
    g.fillEllipse(bounds);

    g.setColour(enabled ? Colour(255u, 154u, 1u) : Colours::grey);
    g.drawEllipse(bounds, 1.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider)) {
        auto  center = bounds.getCentre();

        Path p;

        Rectangle <float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f); 

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(center);

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    };

}

void LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& toggleButton, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {

    using namespace juce;

    Path powerButton;

    auto bounds = toggleButton.getLocalBounds();
    auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
    auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

    float ang = 30.f;

    size -= 6;

    powerButton.addCentredArc(r.getCentreX(), r.getCentreY(), size * .5, size * .5, 0.f, degreesToRadians(ang), degreesToRadians(360 - ang), true);

    powerButton.startNewSubPath(r.getCentreX(), r.getY());
    powerButton.lineTo(r.getCentre());

    PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

    auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);

    g.setColour(color);
    g.strokePath(powerButton, pst);
    g.drawEllipse(r, 2);

}

void RotarySliderWithLabels::paint(juce::Graphics& g) {
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    //g.setColour(Colours::red);
    //g.drawRect(getLocalBounds());
    //g.setColour(Colours::yellow);
    //g.drawRect(sliderBounds);

    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        startAng,
        endAng,
        *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * .5f;

    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(15);

    auto numChoices = labels.size();

    //labels for rotarty
    for (int i = 0; i < numChoices; i++) {

        auto pos = labels[i].pos;
        //jassert(0.f <= pos);
        //jassert(pos = 1.f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * .5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const {
    
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextBoxHeight() * 1.5;

    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
    
    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const {

    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    bool addk = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {

        float val = getValue();

        if (val > 999) {
            val /= 1000.f;
            addk = true;
        }

        str = juce::String(val, (addk ? 2 : 0));

    }
    else {

        jassertfalse; //this shouldn't happen!
    }

    if (suffix.isNotEmpty()) {

        str << " ";
            if(addk)
                str << "k";

            str << suffix;
    }

    return str;
}

ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : audioProcessor(p) {

    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    UpdateChain();

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent() {
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback() {
    if (parametersChanged.compareAndSetBool(false, true)) {
        //update the monochain
        UpdateChain();
        //signal a repaint
        repaint();

    }
}

void ResponseCurveComponent::UpdateChain() {

    auto ChainSettings = getChainSettings(audioProcessor.apvts);

    monoChain.setBypassed<ChainPositions::LowCut>(ChainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::Mid>(ChainSettings.midBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(ChainSettings.highCutBypassed);

    auto midCoefficients = makeMidFilter(ChainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Mid>().coefficients, midCoefficients);

    auto lowCutCoefficients = makeLowCutFilter(ChainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(ChainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, ChainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, ChainSettings.highCutSlope);
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);

    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysisArea();

    auto width = responseArea.getWidth();

    auto& lowCut = monoChain.get<ChainPositions::LowCut>();
    auto& mid = monoChain.get<ChainPositions::Mid>();
    auto& highCut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;
    mags.resize(width);

    for (int i = 0; i < width; i++) {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(width), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Mid>()) {
            mag *= mid.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!monoChain.isBypassed<ChainPositions::LowCut>())
        {
            if (!lowCut.isBypassed<0>())
                mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowCut.isBypassed<1>())
                mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowCut.isBypassed<2>())
                mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowCut.isBypassed<3>())
                 mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!monoChain.isBypassed<ChainPositions::HighCut>())
        {
            if (!highCut.isBypassed<0>())
                mag *= highCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highCut.isBypassed<1>())
                mag *= highCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highCut.isBypassed<2>())
                mag *= highCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highCut.isBypassed<3>())
                mag *= highCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); i++) {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
     
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized() {

    using namespace juce;

    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

    Graphics g(background);

    Array <float> freqs{
        20,50,100,
        200,500,1000,
        2000,5000,10000,
        20000
    };

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    Array<float> xs;
    for (auto f : freqs) {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);

    }

    g.setColour(Colours::dimgrey);
    for (auto x : xs) {
        g.drawVerticalLine(x, top, bottom); 
    }

    Array<float> gain{
        -24, -12, 0, 12, 24
    };

    for (auto gdB : gain) {
        auto y = jmap(gdB, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gdB == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);

    }

    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);


    //top side, frequency line text
    for (int i = 0; i < freqs.size(); i++) {

        auto f = freqs[i];
        auto x = xs[i];

        bool addk = false;
        String str;
        if (f > 999.f) {
            addk = true;
            f /= 1000.f;
        }

        str << f;
        if (addk)
            str << "k";
        str << "Hz";

        Rectangle<int> r;
        r.setSize(g.getCurrentFont().getStringWidth(str), fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    for (auto gdB : gain) {

        //curve gain, right side
        auto y = jmap(gdB, -24.f, 24.f, float(bottom), float(top));

        String str;
        if (gdB > 0)
            str << "+";
        str << gdB;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gdB == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);

        g.drawFittedText(str, r, juce::Justification::centred, 1);

        str.clear();
        str << (gdB - 24.f);

        // analyzer, left side gain
        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centred, 1);

    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea() {

    auto bounds = getLocalBounds();

    //bounds.reduce(10, //JUCE_LIVE_CONSTANT(10),
    //    8 //JUCE_LIVE_CONSTANT(8)
     //   );

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20); 

    return bounds;
}

juce::Rectangle <int> ResponseCurveComponent::getAnalysisArea() {

    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    midFreqSlider(*audioProcessor.apvts.getParameter("Mid Freq"), "Hz"),
    midGainSlider(*audioProcessor.apvts.getParameter("Mid Gain"), "dB"),
    midQualitySlider(*audioProcessor.apvts.getParameter("Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
    responseCurveComponent(audioProcessor),
    midFreqSliderAT(audioProcessor.apvts, "Mid Freq", midFreqSlider),
    midGainSliderAT(audioProcessor.apvts, "Mid Gain", midGainSlider),
    midQualitySliderAT(audioProcessor.apvts, "Quality", midQualitySlider),
    lowCutFreqSliderAT(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAT(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAT(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAT(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),
    lowCutBypassButtonAT(audioProcessor.apvts, "LowCut Bypassed", lowCutBypassButton),
    midBypassButtonAT(audioProcessor.apvts, "Mid Bypassed", midBypassButton),
    highCutBypassButtonAT(audioProcessor.apvts, "HighCut Bypassed", highCutBypassButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    midFreqSlider.labels.add({ 0.f, "20Hz" });
    midFreqSlider.labels.add({ 1.f, "20kHz" });
    midGainSlider.labels.add({ 0.f, "-24 dB" });
    midGainSlider.labels.add({ 1.f, "24 dB" });
    midQualitySlider.labels.add({ 0.f, "0.1" });
    midQualitySlider.labels.add({ 1.f, "10" });
    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20kHz" });
    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20kHz" });
    //lowCutSlopeSlider.labels.add({ 0.f, "6 dB/Oct" });
    //lowCutSlopeSlider.labels.add({ 1.f, "24 dB/Oct" });
   // highCutSlopeSlider.labels.add({ 0.f, "6 dB/Oct" });
   // highCutSlopeSlider.labels.add({ 1.f, "24 dB/Oct" });


    for (auto* comp : getComps()) {
        addAndMakeVisible(comp);
    }

    midBypassButton.setLookAndFeel(&lnf);
    lowCutBypassButton.setLookAndFeel(&lnf);
    highCutBypassButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<SimpleEQAudioProcessorEditor>(this);
    midBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->midBypassButton.getToggleState();

            comp->midFreqSlider.setEnabled(!bypassed);
            comp->midGainSlider.setEnabled(!bypassed);
            comp->midQualitySlider.setEnabled(!bypassed);
        }
    };

    lowCutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent()) {

            auto bypassed = comp->lowCutBypassButton.getToggleState();

            comp->lowCutFreqSlider.setEnabled(!bypassed);
            comp->lowCutSlopeSlider.setEnabled(!bypassed);
        }
    };

    highCutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent()) {

            auto bypassed = comp->highCutBypassButton.getToggleState();

            comp->highCutFreqSlider.setEnabled(!bypassed);
            comp->highCutSlopeSlider.setEnabled(!bypassed);
        }
    };

    setSize (600, 480);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    midBypassButton.setLookAndFeel(nullptr);
    lowCutBypassButton.setLookAndFeel(nullptr);
    highCutBypassButton.setLookAndFeel(nullptr);
    
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
 
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    //float hRatio = JUCE_LIVE_CONSTANT(33) / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * .25);

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(5);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * .33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * .5);

    lowCutBypassButton.setBounds(lowCutArea.removeFromBottom(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * .5 ));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highCutBypassButton.setBounds(highCutArea.removeFromBottom(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * .5));
    highCutSlopeSlider.setBounds(highCutArea);

    midBypassButton.setBounds(bounds.removeFromBottom(25));
    midFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * .33));
    midGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * .5));
    midQualitySlider.setBounds(bounds);
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps() {

    return
    {
        &midFreqSlider,
        &midGainSlider,
        &midQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,
        &lowCutBypassButton,
        &midBypassButton,
        &highCutBypassButton

    };
}