#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace ui
{
const juce::Colour bgTop    { 0xff1c1e24 };
const juce::Colour bgBottom { 0xff0d0e11 };
const juce::Colour panel    { 0xff15171c };
const juce::Colour edge     { 0xff2c2f38 };
const juce::Colour amber    { 0xffffa028 };
const juce::Colour dim      { 0xff8a8f9c };
const juce::Colour text     { 0xffe6e8ee };
} // namespace ui

//==============================================================================
// The pedal. Drag up/down or use the mouse wheel to rock it. Double-click re-centres it.
class PedalComponent : public juce::Component
{
public:
    explicit PedalComponent (juce::RangedAudioParameter& param);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void setValueFromUser (float v, bool partOfGesture);

    juce::ParameterAttachment attachment;
    float value = 0.5f, dragStartValue = 0.5f;
};

//==============================================================================
// Frequency-response graph + readouts (uses the same maths as the audio engine).
class ResponseDisplay : public juce::Component, private juce::Timer
{
public:
    explicit ResponseDisplay (juce::AudioProcessorValueTreeState& state);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }
    juce::AudioProcessorValueTreeState& apvts;
};

//==============================================================================
class ChoiceSelector : public juce::Component
{
public:
    ChoiceSelector (juce::RangedAudioParameter& param, juce::StringArray names);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::StringArray items;
    juce::ParameterAttachment attachment;
    int selected = 0;
};

//==============================================================================
class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

//==============================================================================
class SoloWahEditor : public juce::AudioProcessorEditor
{
public:
    explicit SoloWahEditor (SoloWahProcessor&);
    ~SoloWahEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::String label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (Knob& k, const juce::String& label, const juce::String& paramId);

    SoloWahProcessor& proc;
    KnobLookAndFeel lnf;
    PedalComponent pedal;
    ResponseDisplay display;
    ChoiceSelector voicingSelector;
    Knob resonance, range, drive, mix, output;
    juce::Rectangle<int> pedalPanel, displayPanel, knobPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoloWahEditor)
};
