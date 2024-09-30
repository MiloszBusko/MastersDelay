/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    auto enabled = slider.isEnabled();

    g.setColour(enabled ? Colour(255u, 126u, 13u) : Colours::darkgrey); //apka Digital Color Meter
    g.fillEllipse(bounds);

    g.setColour(enabled ? Colour(207u, 34u, 0u) : Colours::grey);
    g.drawEllipse(bounds, 2.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();

        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle); //zmapowanie wartosci radionow pomiedzy granice rotary slidera

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
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& toggleButton,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    using namespace juce;

    Path powerButton;

    auto bounds = toggleButton.getLocalBounds();
    auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 80;
    auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

    float ang = 30.f;

    size -= 10;

    powerButton.addCentredArc(r.getCentreX(),
        r.getCentreY(),
        size * 0.5,
        size * 0.5, 0.f,
        degreesToRadians(ang),
        degreesToRadians(360.f - ang),
        true);

    powerButton.startNewSubPath(r.getCentreX(), r.getY());
    powerButton.lineTo(r.getCentre());

    PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

    auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(207u, 34u, 0u);

    g.setColour(color);
    g.strokePath(powerButton, pst);
    g.drawEllipse(r, 2);
}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 55.f);
    auto endAng = degreesToRadians(180.f - 55.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        startAng,
        endAng,
        *this);

    //$12 rysowanie wartosci granicznych
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(255u, 126u, 13u));

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.22f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;

        if (labels[i].pos == 1.22f) {
            g.setFont(getTextHeight() + 1);
            r.setSize(g.getCurrentFont().getStringWidthFloat(str), getTextHeight() - 6);
            r.setCentre(c);
            r.setY(r.getY() + getTextHeight() - 6);
        }
        else {
            g.setFont(getTextHeight() - 2);
            r.setSize(g.getCurrentFont().getStringWidthFloat(str), getTextHeight() + 2);
            r.setCentre(c);
            r.setY(r.getY() + getTextHeight() + 2);
        }

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::verticallyCentred, 1);
    }
    //$12
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    //return getLocalBounds();
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        return choiceParam->getCurrentChoiceName();
    }

    juce::String str;

    auto numChoices = showPercentages.size();
    for (int i = 0; i < numChoices; ++i)
    {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            auto val = getValue();
            float minValue = floatParam->range.start;
            float maxValue = floatParam->range.end;

            float percentValue = round((val - minValue) / (maxValue - minValue) * 100.f);

            auto perc = showPercentages[i].showPercentage;
            if (perc == true) {
                str = juce::String(percentValue) + " %"; // 1 decimal place
            }
            else {
                str = juce::String(val);
            }
        }
        else
        {
            jassertfalse; //to sie nie powinno przydac ale jest na wszelki wypadek
        }
    }

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        auto val = getValue();

        if (suffix == "ms") {
            val *= 1000.f;
            str = juce::String(val, 0);
        }
    }
    else
    {
        jassertfalse; //to sie nie powinno przydac ale jest na wszelki wypadek
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        str << suffix;
    }

    return str;
}

void RotarySliderWithLabels::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown() && this->isEnabled())
    {
        showTextEditor();
    }
    else
    {
        juce::Slider::mouseDown(event);
    }
}

void RotarySliderWithLabels::showTextEditor()
{
    auto* editor = new juce::TextEditor();
    auto* editorListener = new juce::TextEditor::Listener;
    auto localBounds = getLocalBounds();
    editor->setJustification(juce::Justification::centred);
    if (suffix == "ms") {
        editor->setText(juce::String(getValue() * 1000));
    }
    else if (suffix == " ") {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            auto val = getValue();
            float minValue = floatParam->range.start;
            float maxValue = floatParam->range.end;

            editor->setText(juce::String(round((val - minValue) / (maxValue - minValue) * 100.f)));
            
        }
        else
        {
            jassertfalse; //to sie nie powinno przydac ale jest na wszelki wypadek
        }
    }
    else if (suffix == "Voices") {
        editor->setText(juce::String(getValue() + 2));
    }
    else {
        editor->setText(juce::String(getValue()));
    }
    editor->setBounds(localBounds.getCentreX(), localBounds.getCentreY(), 50, 25);
    editor->addListener(editorListener);

    editor->onReturnKey = [this, editor]() {
        updateSliderValue(editor, this);
        };

    editor->onFocusLost = [this, editor]() {
        updateSliderValue(editor, this);
        };

    addAndMakeVisible(editor);
    editor->grabKeyboardFocus();
}

void RotarySliderWithLabels::updateSliderValue(juce::TextEditor* editor, juce::Slider* slider)
{
    //auto chainSettings = getChainSettings();
    // Update the slider value
    if (suffix == "ms") {
        double newVal = editor->getText().getDoubleValue() / double(1000);
        slider->setValue(newVal);
    }
    else if (suffix == " ") {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            auto val = getValue();
            float minVal = floatParam->range.start;
            float maxVal = floatParam->range.end;

            double newVal = editor->getText().getDoubleValue();
            newVal = minVal + (newVal / 100.f) * (maxVal - minVal);
            slider->setValue(newVal);
        }
        else
        {
            jassertfalse; //to sie nie powinno przydac ale jest na wszelki wypadek
        }
    }
    else if (suffix == "Voices") {
        double newVal = editor->getText().getDoubleValue() - 2;
        slider->setValue(newVal);
    }
    else {
        double newVal = editor->getText().getDoubleValue();
        slider->setValue(newVal);
    }

    // Remove the editor
    editor->removeChildComponent(editor);
    delete editor;
}


void PowerButton::paint(juce::Graphics& g)
{
    using namespace juce;

    auto buttonBounds = getButtonBounds();

    getLookAndFeel().drawToggleButton(g,
        *this,
        true,
        true);

    auto center = buttonBounds.toFloat().getCentre();
    auto radius = buttonBounds.getWidth() * 0.5f;

    g.setColour(Colour(255u, 126u, 13u));

    auto numChoices = names.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, degreesToRadians(180.f));

        Rectangle<float> r;
        auto str = names[i].name;

        g.setFont(getTextHeight());
        r.setSize(g.getCurrentFont().getStringWidthFloat(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight() - 20);

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::verticallyCentred, 1);
    }
}

juce::Rectangle<int> PowerButton::getButtonBounds() const
{
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}


void BpmEditor::setBpmEditor(juce::Slider* slider)
{

    auto val = slider->getValue();
    
    this->setText(juce::String(round(60.f / val)));

    this->onReturnKey = [this, slider]() {
        updateDelayValue(slider);
        this->setCaretVisible(false);
        };

    this->onFocusLost = [this, slider]() {
        updateDelayValue(slider);
        this->setCaretVisible(false);
        };

    this->grabKeyboardFocus();
}

void BpmEditor::updateDelayValue(juce::Slider* slider)
{
    double newVal = this->getText().getDoubleValue();
    slider->setValue(60.f / newVal);
}

//==============================================================================
MastersDelayAudioProcessorEditor::MastersDelayAudioProcessorEditor (MastersDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    delayTimeSlider(*audioProcessor.apvts.getParameter("Delay Time"), "ms"),
    feedbackSlider(*audioProcessor.apvts.getParameter("Feedback"), " "),
    dryLevelSlider(*audioProcessor.apvts.getParameter("Dry Level"), " "),
    wetLevelSlider(*audioProcessor.apvts.getParameter("Wet Level"), " "),

    flangDelaySlider(*audioProcessor.apvts.getParameter("Flanger Delay"), "ms"),
    flangWidthSlider(*audioProcessor.apvts.getParameter("Flanger Width"), "ms"),
    flangDepthSlider(*audioProcessor.apvts.getParameter("Flanger Depth"), " "),
    flangFeedbackSlider(*audioProcessor.apvts.getParameter("Flanger Feedback"), " "),
    flangLfoFreqSlider(*audioProcessor.apvts.getParameter("Flanger LFO Frequency"), "Hz"),

    vibWidthSlider(*audioProcessor.apvts.getParameter("Vibrato Width"), "ms"),
    vibDepthSlider(*audioProcessor.apvts.getParameter("Vibrato Depth"), " "),
    vibLfoFreqSlider(*audioProcessor.apvts.getParameter("Vibrato LFO Frequency"), "Hz"),

    chorDelaySlider(*audioProcessor.apvts.getParameter("Chorus Delay"), "ms"),
    chorWidthSlider(*audioProcessor.apvts.getParameter("Chorus Width"), "ms"),
    chorDepthSlider(*audioProcessor.apvts.getParameter("Chorus Depth"), " "),
    chorLfoFreqSlider(*audioProcessor.apvts.getParameter("Chorus LFO Frequency"), "Hz"),
    numOfVoicesSlider(*audioProcessor.apvts.getParameter("Number of Voices"), "Voices"),

    dryReverbSlider(*audioProcessor.apvts.getParameter("Dry Reverb"), " "),
    wetReverbSlider(*audioProcessor.apvts.getParameter("Wet Reverb"), " "),
    roomSizeSlider(*audioProcessor.apvts.getParameter("Room Size"), " "),
    dampingSlider(*audioProcessor.apvts.getParameter("Damping"), " "),
    revWidthSlider(*audioProcessor.apvts.getParameter("Reverb Width"), " "),

    delayTimeSliderAttachment(audioProcessor.apvts, "Delay Time", delayTimeSlider),
    feedbackSliderAttachment(audioProcessor.apvts, "Feedback", feedbackSlider),
    dryLevelSliderAttachment(audioProcessor.apvts, "Dry Level", dryLevelSlider),
    wetLevelSliderAttachment(audioProcessor.apvts, "Wet Level", wetLevelSlider),

    flangDelaySliderAttachment(audioProcessor.apvts, "Flanger Delay", flangDelaySlider),
    flangWidthSliderAttachment(audioProcessor.apvts, "Flanger Width", flangWidthSlider),
    flangDepthSliderAttachment(audioProcessor.apvts, "Flanger Depth", flangDepthSlider),
    flangFeedbackSliderAttachment(audioProcessor.apvts, "Flanger Feedback", flangFeedbackSlider),
    flangLfoFreqSliderAttachment(audioProcessor.apvts, "Flanger LFO Frequency", flangLfoFreqSlider),

    vibWidthSliderAttachment(audioProcessor.apvts, "Vibrato Width", vibWidthSlider),
    vibDepthSliderAttachment(audioProcessor.apvts, "Vibrato Depth", vibDepthSlider),
    vibLfoFreqSliderAttachment(audioProcessor.apvts, "Vibrato LFO Frequency", vibLfoFreqSlider),

    chorDelaySliderAttachment(audioProcessor.apvts, "Chorus Delay", chorDelaySlider),
    chorWidthSliderAttachment(audioProcessor.apvts, "Chorus Width", chorWidthSlider),
    chorDepthSliderAttachment(audioProcessor.apvts, "Chorus Depth", chorDepthSlider),
    chorLfoFreqSliderAttachment(audioProcessor.apvts, "Chorus LFO Frequency", chorLfoFreqSlider),
    numOfVoicesSliderAttachment(audioProcessor.apvts, "Number of Voices", numOfVoicesSlider),

    dryReverbSliderAttachment(audioProcessor.apvts, "Dry Reverb", dryReverbSlider),
    wetReverbSliderAttachment(audioProcessor.apvts, "Wet Reverb", wetReverbSlider),
    roomSizeSliderAttachment(audioProcessor.apvts, "Room Size", roomSizeSlider),
    dampingSliderAttachment(audioProcessor.apvts, "Damping", dampingSlider),
    revWidthSliderAttachment(audioProcessor.apvts, "Reverb Width", revWidthSlider),

    flangerButtonAttachment(audioProcessor.apvts, "Flanger On", flangerButton),
    vibratoButtonAttachment(audioProcessor.apvts, "Vibrato On", vibratoButton),
    chorusButtonAttachment(audioProcessor.apvts, "Chorus On", chorusButton),
    dryReverbButtonAttachment(audioProcessor.apvts, "Dry Reverb On", dryReverbButton),
    wetReverbButtonAttachment(audioProcessor.apvts, "Wet Reverb On", wetReverbButton)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    delayTimeSlider.labels.add({ 0.f, "0.1s" });
    delayTimeSlider.labels.add({ 1.22f, "Delay Time" });
    delayTimeSlider.labels.add({ 1.f, "3.0s" });
    feedbackSlider.labels.add({ 0.f, "0%" });
    feedbackSlider.labels.add({ 1.22f, "Feedback" });
    feedbackSlider.labels.add({ 1.f, "100%" });
    dryLevelSlider.labels.add({ 0.f, "0%" });
    dryLevelSlider.labels.add({ 1.22f, "Dry Amount" });
    dryLevelSlider.labels.add({ 1.f, "100%" });
    wetLevelSlider.labels.add({ 0.f, "0%" });
    wetLevelSlider.labels.add({ 1.22f, "Wet Amount" });
    wetLevelSlider.labels.add({ 1.f, "100%" });

    flangDelaySlider.labels.add({ 0.f, "1ms" });
    flangDelaySlider.labels.add({ 1.22f, "Delay Time" });
    flangDelaySlider.labels.add({ 1.f, "20ms" });
    flangWidthSlider.labels.add({ 0.f, "1ms" });
    flangWidthSlider.labels.add({ 1.22f, "Width" });
    flangWidthSlider.labels.add({ 1.f, "20ms" });
    flangDepthSlider.labels.add({ 0.f, "0%" });
    flangDepthSlider.labels.add({ 1.22f, "Amount" });
    flangDepthSlider.labels.add({ 1.f, "100%" });
    flangFeedbackSlider.labels.add({ 0.f, "0%" });
    flangFeedbackSlider.labels.add({ 1.22f, "Feedback" });
    flangFeedbackSlider.labels.add({ 1.f, "100%" });
    flangLfoFreqSlider.labels.add({ 0.f, "0.1Hz" });
    flangLfoFreqSlider.labels.add({ 1.22f, "Rate" });
    flangLfoFreqSlider.labels.add({ 1.f, "2.0Hz" });

    vibWidthSlider.labels.add({ 0.f, "1ms" });
    vibWidthSlider.labels.add({ 1.22f, "Width" });
    vibWidthSlider.labels.add({ 1.f, "40ms" });
    vibDepthSlider.labels.add({ 0.f, "0%" });
    vibDepthSlider.labels.add({ 1.22f, "Amount" });
    vibDepthSlider.labels.add({ 1.f, "100%" });
    vibLfoFreqSlider.labels.add({ 0.f, "0.4Hz" });
    vibLfoFreqSlider.labels.add({ 1.22f, "Rate" });
    vibLfoFreqSlider.labels.add({ 1.f, "8Hz" });

    chorDelaySlider.labels.add({ 0.f, "10ms" });
    chorDelaySlider.labels.add({ 1.22f, "Delay Time" });
    chorDelaySlider.labels.add({ 1.f, "50ms" });
    chorWidthSlider.labels.add({ 0.f, "1ms" });
    chorWidthSlider.labels.add({ 1.22f, "Width" });
    chorWidthSlider.labels.add({ 1.f, "30ms" });
    chorDepthSlider.labels.add({ 0.f, "0%" });
    chorDepthSlider.labels.add({ 1.22f, "Amount" });
    chorDepthSlider.labels.add({ 1.f, "100%" });
    chorLfoFreqSlider.labels.add({ 0.f, "0.1Hz" });
    chorLfoFreqSlider.labels.add({ 1.22f, "Rate" });
    chorLfoFreqSlider.labels.add({ 1.f, "2.0Hz" });
    numOfVoicesSlider.labels.add({ 0.f, "2" });
    numOfVoicesSlider.labels.add({ 1.22f, "Voices" });
    numOfVoicesSlider.labels.add({ 1.f, "5" });

    dryReverbSlider.labels.add({ 0.f, "0%" });
    dryReverbSlider.labels.add({ 1.22f, "Direct Reverb Amount" });
    dryReverbSlider.labels.add({ 1.f, "100%" });
    wetReverbSlider.labels.add({ 0.f, "0%" });
    wetReverbSlider.labels.add({ 1.22f, "Delayed Reverb Amount" });
    wetReverbSlider.labels.add({ 1.f, "100%" });
    roomSizeSlider.labels.add({ 0.f, "0%" });
    roomSizeSlider.labels.add({ 1.22f, "Room Size" });
    roomSizeSlider.labels.add({ 1.f, "100%" });
    dampingSlider.labels.add({ 0.f, "0%" });
    dampingSlider.labels.add({ 1.22f, "Damping" });
    dampingSlider.labels.add({ 1.f, "100%" });
    revWidthSlider.labels.add({ 0.f, "0%" });
    revWidthSlider.labels.add({ 1.22f, "Width" });
    revWidthSlider.labels.add({ 1.f, "100%" });

    delayTimeSlider.showPercentages.add({ false });
    feedbackSlider.showPercentages.add({ true });
    dryLevelSlider.showPercentages.add({ true });
    wetLevelSlider.showPercentages.add({ true });

    flangDelaySlider.showPercentages.add({ false });
    flangWidthSlider.showPercentages.add({ false });
    flangDepthSlider.showPercentages.add({ true });
    flangFeedbackSlider.showPercentages.add({ true });
    flangLfoFreqSlider.showPercentages.add({ false });

    vibWidthSlider.showPercentages.add({ false });
    vibDepthSlider.showPercentages.add({ true });
    vibLfoFreqSlider.showPercentages.add({ false });

    chorDelaySlider.showPercentages.add({ false });
    chorWidthSlider.showPercentages.add({ false });
    chorDepthSlider.showPercentages.add({ true });
    chorLfoFreqSlider.showPercentages.add({ false });
    numOfVoicesSlider.showPercentages.add({ false });

    dryReverbSlider.showPercentages.add({ true });
    wetReverbSlider.showPercentages.add({ true });
    roomSizeSlider.showPercentages.add({ true });
    dampingSlider.showPercentages.add({ true });
    revWidthSlider.showPercentages.add({ true });

    flangerButton.names.add({ "FLANGER" });
    vibratoButton.names.add({ "VIBRATO" });
    chorusButton.names.add({ "CHORUS" });
    dryReverbButton.names.add({ "DIRECT REVERB" });
    wetReverbButton.names.add({ "DELAYED REVERB" });

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    
    for (auto* comp : getBypassedComps())
    {
        comp->setEnabled(false);
    }

    flangerButton.setLookAndFeel(&lnf);
    vibratoButton.setLookAndFeel(&lnf);
    chorusButton.setLookAndFeel(&lnf);
    dryReverbButton.setLookAndFeel(&lnf);
    wetReverbButton.setLookAndFeel(&lnf);

    syncButton.setLookAndFeel(&lnf);
    downButton.setLookAndFeel(&lnf);
    upButton.setLookAndFeel(&lnf);
    tapTempoButton.setLookAndFeel(&lnf);
    tempoDownButton.setLookAndFeel(&lnf);
    tempoUpButton.setLookAndFeel(&lnf);

    syncButton.setButtonText("SYNC\nRATE");
    downButton.setButtonText("RATE\nDOWN");
    upButton.setButtonText("RATE\nUP");
    tapTempoButton.setButtonText("TAP BPM");
    tempoDownButton.setButtonText("TEMPO\nDOWN");
    tempoUpButton.setButtonText("TEMPO\nUP");
    
    bpmEditor.setJustification(juce::Justification::centred);
    bpmEditor.setCaretVisible(false);
    bpmEditor.setText(juce::String(round(60.f / delayTimeSlider.getValue())));

    auto safePtr = juce::Component::SafePointer<MastersDelayAudioProcessorEditor>(this);

    auto chainSettings = getChainSettings(audioProcessor.apvts);

    flangerButton.setToggleState(chainSettings.flangerOn, false);
    vibratoButton.setToggleState(chainSettings.vibratoOn, false);
    chorusButton.setToggleState(chainSettings.chorusOn, false);
    dryReverbButton.setToggleState(chainSettings.dryReverbOn, false);
    wetReverbButton.setToggleState(chainSettings.wetReverbOn, false);

    if (!chainSettings.flangerOn)
    {
        flangDelaySlider.setEnabled(true);
        flangWidthSlider.setEnabled(true);
        flangDepthSlider.setEnabled(true);
        flangFeedbackSlider.setEnabled(true);
        flangLfoFreqSlider.setEnabled(true);
    }
    if (!chainSettings.vibratoOn)
    {
        vibWidthSlider.setEnabled(true);
        vibDepthSlider.setEnabled(true);
        vibLfoFreqSlider.setEnabled(true);
    }
    if (!chainSettings.chorusOn)
    {
        chorDelaySlider.setEnabled(true);
        chorWidthSlider.setEnabled(true);
        chorDepthSlider.setEnabled(true);
        numOfVoicesSlider.setEnabled(true);
        chorLfoFreqSlider.setEnabled(true);
    }
    if (!chainSettings.dryReverbOn)
    {
        dryReverbSlider.setEnabled(true);
        roomSizeSlider.setEnabled(true);
        dampingSlider.setEnabled(true);
        revWidthSlider.setEnabled(true);
    }
    if (!chainSettings.wetReverbOn)
    {
        wetReverbSlider.setEnabled(true);
        roomSizeSlider.setEnabled(true);
        dampingSlider.setEnabled(true);
        revWidthSlider.setEnabled(true);
    }

    flangerButton.onClick = [safePtr]()
        {
            if (auto* comp = safePtr.getComponent())
            {
                auto bypassed = comp->flangerButton.getToggleState();

                comp->flangDelaySlider.setEnabled(!bypassed);
                comp->flangWidthSlider.setEnabled(!bypassed);
                comp->flangDepthSlider.setEnabled(!bypassed);
                comp->flangFeedbackSlider.setEnabled(!bypassed);
                comp->flangLfoFreqSlider.setEnabled(!bypassed);

                if (!(comp->flangerButton.getToggleState())) {
                    comp->vibWidthSlider.setEnabled(false);
                    comp->vibDepthSlider.setEnabled(false);
                    comp->vibLfoFreqSlider.setEnabled(false);

                    comp->chorDelaySlider.setEnabled(false);
                    comp->chorWidthSlider.setEnabled(false);
                    comp->chorDepthSlider.setEnabled(false);
                    comp->chorLfoFreqSlider.setEnabled(false);
                    comp->numOfVoicesSlider.setEnabled(false);

                    comp->vibratoButton.setToggleState(true, true);
                    comp->chorusButton.setToggleState(true, true);
                }
            }
        };

    vibratoButton.onClick = [safePtr]()
        {
            if (auto* comp = safePtr.getComponent())
            {
                auto bypassed = comp->vibratoButton.getToggleState();

                comp->vibWidthSlider.setEnabled(!bypassed);
                comp->vibDepthSlider.setEnabled(!bypassed);
                comp->vibLfoFreqSlider.setEnabled(!bypassed);

                if (!(comp->vibratoButton.getToggleState())) {
                    comp->flangDelaySlider.setEnabled(false);
                    comp->flangWidthSlider.setEnabled(false);
                    comp->flangDepthSlider.setEnabled(false);
                    comp->flangLfoFreqSlider.setEnabled(false);
                    comp->flangFeedbackSlider.setEnabled(false);

                    comp->chorDelaySlider.setEnabled(false);
                    comp->chorWidthSlider.setEnabled(false);
                    comp->chorDepthSlider.setEnabled(false);
                    comp->chorLfoFreqSlider.setEnabled(false);
                    comp->numOfVoicesSlider.setEnabled(false);

                    comp->flangerButton.setToggleState(true, true);
                    comp->chorusButton.setToggleState(true, true);
                }
            }
        };

    chorusButton.onClick = [safePtr]()
        {
            if (auto* comp = safePtr.getComponent())
            {
                auto bypassed = comp->chorusButton.getToggleState();

                comp->chorDelaySlider.setEnabled(!bypassed);
                comp->chorWidthSlider.setEnabled(!bypassed);
                comp->chorDepthSlider.setEnabled(!bypassed);
                comp->chorLfoFreqSlider.setEnabled(!bypassed);
                comp->numOfVoicesSlider.setEnabled(!bypassed);

                if (!(comp->chorusButton.getToggleState())) {
                    comp->flangDelaySlider.setEnabled(false);
                    comp->flangWidthSlider.setEnabled(false);
                    comp->flangDepthSlider.setEnabled(false);
                    comp->flangLfoFreqSlider.setEnabled(false);
                    comp->flangFeedbackSlider.setEnabled(false);

                    comp->vibWidthSlider.setEnabled(false);
                    comp->vibDepthSlider.setEnabled(false);
                    comp->vibLfoFreqSlider.setEnabled(false);

                    comp->flangerButton.setToggleState(true, true);
                    comp->vibratoButton.setToggleState(true, true);
                }
            }
        };

    

    if (flangerButton.getToggleState()) {
        chainSettings.flangerOn = true;
        chainSettings.vibratoOn = true;
        chainSettings.chorusOn = true;
    }
    else {
        chainSettings.flangerOn = false;
        chainSettings.vibratoOn = true;
        chainSettings.chorusOn = true;
    }
    if (vibratoButton.getToggleState()) {
        chainSettings.flangerOn = true;
        chainSettings.vibratoOn = true;
        chainSettings.chorusOn = true;
    }
    else {
        chainSettings.flangerOn = true;
        chainSettings.vibratoOn = false;
        chainSettings.chorusOn = true;
    }
    if (chorusButton.getToggleState()) {
        chainSettings.flangerOn = true;
        chainSettings.vibratoOn = true;
        chainSettings.chorusOn = true;
    }
    else {
        chainSettings.flangerOn = true;
        chainSettings.vibratoOn = true;
        chainSettings.chorusOn = false;
    }

    dryReverbButton.onClick = [safePtr]()
        {
            if (auto* comp = safePtr.getComponent())
            {
                auto bypassed = comp->dryReverbButton.getToggleState();

                comp->dryReverbSlider.setEnabled(!bypassed);
                if ((comp->wetReverbButton.getToggleState())) {
                    comp->roomSizeSlider.setEnabled(!bypassed);
                    comp->dampingSlider.setEnabled(!bypassed);
                    comp->revWidthSlider.setEnabled(!bypassed);
                }
            }
        };

    wetReverbButton.onClick = [safePtr]()
        {
            if (auto* comp = safePtr.getComponent())
            {
                auto bypassed = comp->wetReverbButton.getToggleState();

                comp->wetReverbSlider.setEnabled(!bypassed);
                if ((comp->dryReverbButton.getToggleState())) {
                    comp->roomSizeSlider.setEnabled(!bypassed);
                    comp->dampingSlider.setEnabled(!bypassed);
                    comp->revWidthSlider.setEnabled(!bypassed);
                }
            }
        };

    syncButton.onClick = [this]()
        {
            auto chainSettings = getChainSettings(audioProcessor.apvts);

            if (!flangerButton.getToggleState()) {
                flangLfoFreqSlider.setValue(chainSettings.delayTime);
                if (chainSettings.delayTime > flangLfoFreqSlider.getMaximum()) {
                    flangLfoFreqSlider.setValue(chainSettings.delayTime / 2.f);
                }
            }
            else if (!vibratoButton.getToggleState()) {
                vibLfoFreqSlider.setValue(4.f * chainSettings.delayTime);
                if (4.f * chainSettings.delayTime > vibLfoFreqSlider.getMaximum()) {
                    vibLfoFreqSlider.setValue(4.f * chainSettings.delayTime / 2.f);
                }
            }
            else if (!chorusButton.getToggleState()) {
                chorLfoFreqSlider.setValue(chainSettings.delayTime);
                if (chainSettings.delayTime > chorLfoFreqSlider.getMaximum()) {
                    chorLfoFreqSlider.setValue(chainSettings.delayTime / 2.f);
                }
            }
        };

    downButton.onClick = [this]()
        {
            auto chainSettings = getChainSettings(audioProcessor.apvts);

            if (!flangerButton.getToggleState()) {
                if (chainSettings.flangLfoFreq / 2.f >= flangLfoFreqSlider.getMinimum()) {
                    flangLfoFreqSlider.setValue(flangLfoFreqSlider.getValue() / 2.f);
                }
            }
            else if (!vibratoButton.getToggleState()) {
                if(chainSettings.vibLfoFreq / 2.f >= vibLfoFreqSlider.getMinimum()) {
                    vibLfoFreqSlider.setValue(vibLfoFreqSlider.getValue() / 2.f);
                }
            }
            else if (!chorusButton.getToggleState()) {
                if (chainSettings.chorLfoFreq / 2.f >= chorLfoFreqSlider.getMinimum()) {
                    chorLfoFreqSlider.setValue(chorLfoFreqSlider.getValue() / 2.f);
                }
            }
        };

    upButton.onClick = [this]()
        {
            auto chainSettings = getChainSettings(audioProcessor.apvts);

            if (!flangerButton.getToggleState()) {
                if (chainSettings.flangLfoFreq * 2.f <= flangLfoFreqSlider.getMaximum()) {
                    flangLfoFreqSlider.setValue(flangLfoFreqSlider.getValue() * 2.f);
                }
            }
            else if (!vibratoButton.getToggleState()) {
                if (chainSettings.vibLfoFreq * 2.f <= vibLfoFreqSlider.getMaximum()) {
                    vibLfoFreqSlider.setValue(vibLfoFreqSlider.getValue() * 2.f);
                }
            }
            else if (!chorusButton.getToggleState()) {
                if (chainSettings.chorLfoFreq * 2.f <= chorLfoFreqSlider.getMaximum()) {
                    chorLfoFreqSlider.setValue(chorLfoFreqSlider.getValue() * 2.f);
                }
            }
        };

    tapTempoButton.onClick = [this]()
        {
            auto currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
            tapTimes.push_back(currentTime);

            if (tapTimes.size() >= 2)
            {
                calculateTapTempo();
            }

            if (tapTimes.size() > 4)
            {
                tapTimes.erase(tapTimes.begin());
            }
        };
    

    tempoDownButton.onClick = [this]()
        {
            auto chainSettings = getChainSettings(audioProcessor.apvts);

            if (chainSettings.delayTime / 2.f >= delayTimeSlider.getMinimum()) {
                delayTimeSlider.setValue(delayTimeSlider.getValue() / 2.f);
            }
        };

    tempoUpButton.onClick = [this]()
        {
            auto chainSettings = getChainSettings(audioProcessor.apvts);

            if (chainSettings.delayTime * 2.f <= delayTimeSlider.getMaximum()) {
                delayTimeSlider.setValue(delayTimeSlider.getValue() * 2.f);
            }
        };

    delayTimeSlider.onValueChange = [this]()
        {
            bpmEditor.setBpmEditor(&delayTimeSlider);
        };

    setSize (1000, 800);
}

MastersDelayAudioProcessorEditor::~MastersDelayAudioProcessorEditor()
{
    flangerButton.setLookAndFeel(nullptr);
    vibratoButton.setLookAndFeel(nullptr);
    chorusButton.setLookAndFeel(nullptr);
    dryReverbButton.setLookAndFeel(nullptr);
    wetReverbButton.setLookAndFeel(nullptr);

    syncButton.setLookAndFeel(nullptr);
    downButton.setLookAndFeel(nullptr);
    upButton.setLookAndFeel(nullptr);
    tapTempoButton.setLookAndFeel(nullptr);
    tempoDownButton.setLookAndFeel(nullptr);
    tempoUpButton.setLookAndFeel(nullptr);
}

//==============================================================================
void MastersDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);

    bool enabled = false;
    if (!flangerButton.getToggleState() || !vibratoButton.getToggleState() || !chorusButton.getToggleState()) {
        enabled = true;
    }
    else {
        enabled = false;
    }

    syncButton.setColour(juce::TextButton::buttonColourId, enabled ? Colour(255u, 126u, 13u) : Colours::darkgrey);
    syncButton.setColour(juce::ComboBox::outlineColourId, enabled ? Colour(207u, 34u, 0u) : Colours::grey);
    syncButton.setColour(juce::TextButton::textColourOffId, enabled ? Colours::white : Colours::lightgrey);

    downButton.setColour(juce::TextButton::buttonColourId, enabled ? Colour(255u, 126u, 13u) : Colours::darkgrey);
    downButton.setColour(juce::ComboBox::outlineColourId, enabled ? Colour(207u, 34u, 0u) : Colours::grey);
    downButton.setColour(juce::TextButton::textColourOffId, enabled ? Colours::white : Colours::lightgrey);

    upButton.setColour(juce::TextButton::buttonColourId, enabled ? Colour(255u, 126u, 13u) : Colours::darkgrey);
    upButton.setColour(juce::ComboBox::outlineColourId, enabled ? Colour(207u, 34u, 0u) : Colours::grey);
    upButton.setColour(juce::TextButton::textColourOffId, enabled ? Colours::white : Colours::lightgrey);

    tapTempoButton.setColour(juce::TextButton::buttonColourId, Colour(255u, 126u, 13u));
    tapTempoButton.setColour(juce::ComboBox::outlineColourId, Colour(207u, 34u, 0u));
    tapTempoButton.setColour(juce::TextButton::textColourOffId, Colours::white);

    tempoDownButton.setColour(juce::TextButton::buttonColourId, Colour(255u, 126u, 13u));
    tempoDownButton.setColour(juce::ComboBox::outlineColourId, Colour(207u, 34u, 0u));
    tempoDownButton.setColour(juce::TextButton::textColourOffId, Colours::white);

    tempoUpButton.setColour(juce::TextButton::buttonColourId, Colour(255u, 126u, 13u));
    tempoUpButton.setColour(juce::ComboBox::outlineColourId, Colour(207u, 34u, 0u));
    tempoUpButton.setColour(juce::TextButton::textColourOffId, Colours::white);

    if (bpmEditor.isMouseButtonDown()) {
        bpmEditor.setBpmEditor(&delayTimeSlider);
        bpmEditor.setCaretVisible(true);
    }
}

void MastersDelayAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    float oneThirdRatio = 1.f / 3.f;
    float oneFifthRatio = 1.f / 5.f;

    bounds.removeFromTop(20);
    bounds.removeFromBottom(20);

    auto delayArea = bounds.removeFromTop(bounds.getHeight() * 0.4f);

    auto reverbArea = delayArea.removeFromTop(delayArea.getHeight() * 0.4f);
    delayArea.removeFromTop(30);

    auto dryReverbArea = delayArea.removeFromLeft(delayArea.getWidth() * oneFifthRatio);
    auto wetReverbArea = delayArea.removeFromRight(delayArea.getWidth() * 0.25f);

    auto flangerArea = bounds.removeFromLeft(bounds.getWidth() * oneThirdRatio);
    auto chorusArea = bounds.removeFromRight(bounds.getWidth() * 0.5f);
    auto vibratoArea = bounds.removeFromTop(bounds.getHeight() * 0.75f);
    auto syncArea = bounds;

    delayTimeSlider.setBounds(delayArea.removeFromLeft(delayArea.getWidth() * 0.25f));
    wetLevelSlider.setBounds(delayArea.removeFromRight(delayArea.getWidth() * oneThirdRatio));
    feedbackSlider.setBounds(delayArea.removeFromLeft(delayArea.getWidth() * 0.5f));
    dryLevelSlider.setBounds(delayArea);

    flangerButton.setBounds(flangerArea.removeFromTop(flangerArea.getHeight() * 0.25f));
    auto flangerArea2 = flangerArea.removeFromTop(flangerArea.getHeight() * oneThirdRatio);
    flangDelaySlider.setBounds(flangerArea2.removeFromRight(flangerArea2.getWidth() * 0.5f));
    flangWidthSlider.setBounds(flangerArea2);
    flangFeedbackSlider.setBounds(flangerArea.removeFromTop(flangerArea.getHeight() * 0.5f));
    flangDepthSlider.setBounds(flangerArea.removeFromRight(flangerArea.getWidth() * 0.5f));
    flangLfoFreqSlider.setBounds(flangerArea);

    vibratoButton.setBounds(vibratoArea.removeFromTop(vibratoArea.getHeight() * oneThirdRatio));
    vibWidthSlider.setBounds(vibratoArea.removeFromTop(vibratoArea.getHeight() * 0.5f));
    vibDepthSlider.setBounds(vibratoArea.removeFromRight(vibratoArea.getWidth() * 0.5f));
    vibLfoFreqSlider.setBounds(vibratoArea);

    chorusButton.setBounds(chorusArea.removeFromTop(chorusArea.getHeight() * 0.25f));
    auto chorusArea2 = chorusArea.removeFromTop(chorusArea.getHeight() * oneThirdRatio);
    chorDelaySlider.setBounds(chorusArea2.removeFromRight(chorusArea2.getWidth() * 0.5f));
    chorWidthSlider.setBounds(chorusArea2);
    numOfVoicesSlider.setBounds(chorusArea.removeFromTop(chorusArea.getHeight() * 0.5f));
    chorDepthSlider.setBounds(chorusArea.removeFromRight(chorusArea.getWidth() * 0.5f));
    chorLfoFreqSlider.setBounds(chorusArea);

    dryReverbArea.setHeight(flangerArea.getHeight());
    wetReverbArea.setHeight(flangerArea.getHeight());
    dryReverbButton.setBounds(dryReverbArea);
    wetReverbButton.setBounds(wetReverbArea);

    dryReverbSlider.setBounds(reverbArea.removeFromLeft(reverbArea.getWidth() * oneFifthRatio));
    wetReverbSlider.setBounds(reverbArea.removeFromRight(reverbArea.getWidth() * 0.25f));
    roomSizeSlider.setBounds(reverbArea.removeFromLeft(reverbArea.getWidth() * oneThirdRatio));
    revWidthSlider.setBounds(reverbArea.removeFromRight(reverbArea.getWidth() * 0.5f));
    dampingSlider.setBounds(reverbArea);

    syncArea.removeFromTop(10);
    auto tapTempoArea = syncArea.removeFromBottom(syncArea.getHeight() * 0.5f);
    auto downArea = syncArea.removeFromLeft(syncArea.getWidth() * oneThirdRatio);
    downArea.reduce(downArea.getWidth() * 0.1f, downArea.getHeight() * 0.1f);
    auto upArea = syncArea.removeFromRight(syncArea.getWidth() * 0.5f);
    upArea.reduce(upArea.getWidth() * 0.1f, upArea.getHeight() * 0.1f);
    syncArea.reduce(syncArea.getWidth() * 0.1f, syncArea.getHeight() * 0.1f);
    auto tempoDownArea = tapTempoArea.removeFromLeft(tapTempoArea.getWidth() * oneThirdRatio);
    auto tempoUpArea = tapTempoArea.removeFromRight(tapTempoArea.getWidth() * 0.5f);
    tapTempoArea.reduce(tapTempoArea.getWidth() * 0.1f, tapTempoArea.getHeight() * 0.1f);
    tempoDownArea.reduce(tapTempoArea.getWidth() * 0.1f, tapTempoArea.getHeight() * 0.1f);
    tempoUpArea.reduce(tapTempoArea.getWidth() * 0.1f, tapTempoArea.getHeight() * 0.1f);
    auto bpmEditorArea = tapTempoArea.removeFromTop(tapTempoArea.getHeight() * 0.5f);
    bpmEditorArea.removeFromBottom(4);
    bpmEditorArea.reduce(10, 0);

    syncButton.setBounds(syncArea);
    downButton.setBounds(downArea);
    upButton.setBounds(upArea);
    bpmEditor.setBounds(bpmEditorArea);
    tapTempoButton.setBounds(tapTempoArea);
    tempoDownButton.setBounds(tempoDownArea);
    tempoUpButton.setBounds(tempoUpArea);
}

void MastersDelayAudioProcessorEditor::calculateTapTempo()
{
    if (tapTimes.size() < 2)
        return;

    auto numIntervals = tapTimes.size() - 1;
    auto sumIntervals = 0.0f;

    for (size_t i = 1; i < tapTimes.size(); ++i)
    {
        sumIntervals += (tapTimes[i] - tapTimes[i - 1]);
    }

    auto averageInterval = sumIntervals / numIntervals;
    
    audioProcessor.setDelayTimeFromTapTempo(averageInterval);
    delayTimeSlider.setValue(averageInterval);


    tapTimes.clear();
}

std::vector<juce::Component*> MastersDelayAudioProcessorEditor::getComps()
{
    return
    {
        &delayTimeSlider,
        &feedbackSlider,
        &dryLevelSlider,
        &wetLevelSlider,

        &flangDelaySlider,
        &flangWidthSlider,
        &flangDepthSlider,
        &flangFeedbackSlider,
        &flangLfoFreqSlider,

        &vibWidthSlider,
        &vibDepthSlider,
        &vibLfoFreqSlider,

        &chorDelaySlider,
        &chorWidthSlider,
        &chorDepthSlider,
        &chorLfoFreqSlider,
        &numOfVoicesSlider,

        &dryReverbSlider,
        &wetReverbSlider,
        &roomSizeSlider,
        &dampingSlider,
        &revWidthSlider,

        &flangerButton,
        &vibratoButton,
        &chorusButton,
        &dryReverbButton,
        &wetReverbButton,

        &syncButton,
        &downButton,
        &upButton,
        &tapTempoButton,
        &tempoDownButton,
        &tempoUpButton,

        &bpmEditor
    };
}

std::vector<juce::Component*> MastersDelayAudioProcessorEditor::getBypassedComps()
{
    return
    {
        &flangDelaySlider,
        &flangWidthSlider,
        &flangDepthSlider,
        &flangFeedbackSlider,
        &flangLfoFreqSlider,

        &vibWidthSlider,
        &vibDepthSlider,
        &vibLfoFreqSlider,

        &chorDelaySlider,
        &chorWidthSlider,
        &chorDepthSlider,
        &chorLfoFreqSlider,
        &numOfVoicesSlider,

        &dryReverbSlider,
        &wetReverbSlider,
        &roomSizeSlider,
        &dampingSlider,
        &revWidthSlider
    };
}
