#include "PluginEditor.h"

using namespace juce;

static Font makeFont (float size, bool bold = false)
{
    return Font (FontOptions (size, bold ? Font::bold : Font::plain));
}

//==============================================================================
// PedalComponent
//==============================================================================
PedalComponent::PedalComponent (RangedAudioParameter& param)
    : attachment (param, [this] (float v) { value = v; repaint(); })
{
    setMouseCursor (MouseCursor::UpDownResizeCursor);
    attachment.sendInitialUpdate();
}

void PedalComponent::setValueFromUser (float v, bool partOfGesture)
{
    value = jlimit (0.0f, 1.0f, v);
    if (partOfGesture) attachment.setValueAsPartOfGesture (value);
    else               attachment.setValueAsCompleteGesture (value);
    repaint();
}

void PedalComponent::mouseDown (const MouseEvent&)
{
    dragStartValue = value;
    attachment.beginGesture();
}

void PedalComponent::mouseDrag (const MouseEvent& e)
{
    // Down = toe down = brighter. Hold shift for fine control.
    const float sensitivity = e.mods.isShiftDown() ? 1200.0f : 240.0f;
    setValueFromUser (dragStartValue + (float) e.getDistanceFromDragStartY() / sensitivity, true);
}

void PedalComponent::mouseUp (const MouseEvent&) { attachment.endGesture(); }

void PedalComponent::mouseDoubleClick (const MouseEvent&)
{
    dragStartValue = 0.5f;
    setValueFromUser (0.5f, true); // inside the gesture opened by mouseDown
}

void PedalComponent::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    const float d = w.isReversed ? -w.deltaY : w.deltaY;   // scroll up = toe up = darker
    setValueFromUser (value - d * 0.15f, false);
}

void PedalComponent::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float cx = b.getCentreX();
    const float ground = b.getBottom() - 58.0f;
    const float pivotY = ground - 112.0f;
    const float maxAngle = degreesToRadians (11.0f);
    const float angle = (value - 0.5f) * 2.0f * maxAngle;   // + = toe down (clockwise)

    // floor shadow
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (cx - 230.0f, ground - 8.0f, 460.0f, 30.0f);
    g.setColour (Colours::black.withAlpha (0.3f));
    g.fillEllipse (cx - 200.0f, ground - 4.0f, 400.0f, 18.0f);

    // base (wedge body)
    Path base;
    base.startNewSubPath (cx - 206.0f, ground);
    base.lineTo (cx + 206.0f, ground);
    base.lineTo (cx + 178.0f, ground - 84.0f);
    base.lineTo (cx - 178.0f, ground - 84.0f);
    base.closeSubPath();
    const auto basePath = base.createPathWithRoundedCorners (14.0f);
    g.setGradientFill (ColourGradient (Colour (0xff3a3d46), cx, ground - 84.0f, Colour (0xff0e0f12), cx, ground, false));
    g.fillPath (basePath);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.strokePath (basePath, PathStrokeType (1.2f));
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawLine (cx - 160.0f, ground - 83.0f, cx + 160.0f, ground - 83.0f, 1.5f);

    // rubber feet
    g.setColour (Colour (0xff070708));
    g.fillRoundedRectangle (cx - 196.0f, ground, 70.0f, 9.0f, 3.5f);
    g.fillRoundedRectangle (cx + 126.0f, ground, 70.0f, 9.0f, 3.5f);

    // pivot post
    g.setGradientFill (ColourGradient (Colour (0xff9aa0aa), cx - 22.0f, 0.0f, Colour (0xff50555e), cx + 22.0f, 0.0f, false));
    g.fillRoundedRectangle (cx - 22.0f, pivotY, 44.0f, ground - 80.0f - pivotY, 4.0f);

    // status LED – brightens as the pedal goes toe-down
    {
        const float lx = cx + 150.0f, ly = ground - 34.0f;
        g.setColour (ui::amber.withAlpha (0.10f + 0.35f * value));
        g.fillEllipse (lx - 14.0f, ly - 14.0f, 28.0f, 28.0f);
        g.setColour (ui::amber.interpolatedWith (Colour (0xff4a2a08), 1.0f - (0.35f + 0.65f * value)));
        g.fillEllipse (lx - 5.0f, ly - 5.0f, 10.0f, 10.0f);
        g.setColour (Colours::white.withAlpha (0.35f));
        g.fillEllipse (lx - 2.5f, ly - 3.5f, 4.0f, 3.0f);
    }

    // heel / toe labels on the base
    g.setColour (ui::dim);
    g.setFont (makeFont (12.0f, true));
    g.drawText ("HEEL  (dark)", Rectangle<float> (cx - 200.0f, ground - 44.0f, 130.0f, 16.0f), Justification::centredLeft);
    g.drawText ("TOE  (bright)", Rectangle<float> (cx - 60.0f, ground - 44.0f, 140.0f, 16.0f), Justification::centred);

    // treadle (rotates about the pivot)
    {
        Graphics::ScopedSaveState state (g);
        g.addTransform (AffineTransform::rotation (angle, cx, pivotY));

        const float tx = cx - 192.0f, tw = 384.0f, th = 46.0f, ty = pivotY - th * 0.5f;

        // drop shadow
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (tx + 2.0f, ty + 6.0f, tw, th, 16.0f);

        // body
        g.setGradientFill (ColourGradient (Colour (0xff565a64), 0.0f, ty, Colour (0xff17181c), 0.0f, ty + th, false));
        g.fillRoundedRectangle (tx, ty, tw, th, 16.0f);
        g.setColour (Colours::white.withAlpha (0.18f));
        g.drawRoundedRectangle (tx + 0.5f, ty + 0.5f, tw - 1.0f, th - 1.0f, 16.0f, 1.0f);

        // rubber grip ribs along the top
        for (int i = 0; i < 26; ++i)
        {
            const float rx = tx + 22.0f + (float) i * ((tw - 44.0f - 8.0f) / 25.0f);
            g.setColour (Colour (0xff060607));
            g.fillRoundedRectangle (rx, ty - 7.0f, 8.0f, 11.0f, 2.5f);
            g.setColour (Colours::white.withAlpha (0.20f));
            g.fillRoundedRectangle (rx + 1.0f, ty - 6.5f, 6.0f, 1.8f, 0.9f);
        }

        // chrome side plate
        const auto plate = Rectangle<float> (tx + 20.0f, ty + 12.0f, tw - 40.0f, th - 20.0f);
        ColourGradient chrome (Colour (0xfff0f2f6), 0.0f, plate.getY(), Colour (0xff6c717b), 0.0f, plate.getBottom(), false);
        chrome.addColour (0.45, Colour (0xffc9cdd4));
        chrome.addColour (0.55, Colour (0xff8d929b));
        g.setGradientFill (chrome);
        g.fillRoundedRectangle (plate, 8.0f);
        g.setColour (Colours::black.withAlpha (0.45f));
        g.drawRoundedRectangle (plate, 8.0f, 1.0f);

        g.setColour (Colour (0xff23252b));
        g.setFont (makeFont (13.0f, true));
        auto left = plate.withTrimmedRight (plate.getWidth() * 0.5f + 20.0f);
        auto right = plate.withTrimmedLeft (plate.getWidth() * 0.5f + 20.0f);
        g.drawText ("S O L O", left, Justification::centred);
        g.drawText ("W A H", right, Justification::centred);

        // pivot bolt
        g.setGradientFill (ColourGradient (Colour (0xfff4f5f8), cx - 9.0f, pivotY - 9.0f, Colour (0xff5d626c), cx + 9.0f, pivotY + 9.0f, false));
        g.fillEllipse (cx - 11.0f, pivotY - 11.0f, 22.0f, 22.0f);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawEllipse (cx - 11.0f, pivotY - 11.0f, 22.0f, 22.0f, 1.0f);
        g.setColour (Colour (0xff2a2c32));
        g.fillEllipse (cx - 3.5f, pivotY - 3.5f, 7.0f, 7.0f);
    }

    // hint
    g.setColour (ui::dim);
    g.setFont (makeFont (12.5f));
    g.drawText ("Drag the pedal up / down or scroll the mouse wheel  -  double-click to centre",
                Rectangle<float> (0.0f, 8.0f, b.getWidth(), 18.0f), Justification::centred);
}

//==============================================================================
// ResponseDisplay
//==============================================================================
ResponseDisplay::ResponseDisplay (AudioProcessorValueTreeState& s) : apvts (s)
{
    startTimerHz (30);
}

void ResponseDisplay::paint (Graphics& g)
{
    const float pedal = apvts.getRawParameterValue ("pedal")->load();
    const int voice   = (int) std::lround (apvts.getRawParameterValue ("voicing")->load());
    const float res   = apvts.getRawParameterValue ("resonance")->load();
    const float range = apvts.getRawParameterValue ("range")->load();
    const auto st = solowah::settingsFor (voice, pedal, res, range);

    auto area = getLocalBounds().toFloat().reduced (14.0f, 12.0f);
    auto graph = area.removeFromTop (172.0f);
    auto readout = area.withTrimmedTop (10.0f);

    // graph background
    g.setColour (Colour (0xff0b0c0f));
    g.fillRoundedRectangle (graph, 6.0f);

    const float fLo = 60.0f, fHi = 8000.0f, dbLo = -30.0f, dbHi = 15.0f;
    auto xOf = [&] (float f) { return graph.getX() + graph.getWidth() * std::log (f / fLo) / std::log (fHi / fLo); };
    auto yOf = [&] (float db) { return graph.getBottom() - graph.getHeight() * (db - dbLo) / (dbHi - dbLo); };

    g.setFont (makeFont (10.0f));
    for (float f : { 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f })
    {
        g.setColour (Colours::white.withAlpha (0.06f));
        g.drawVerticalLine ((int) xOf (f), graph.getY(), graph.getBottom());
        g.setColour (ui::dim.withAlpha (0.8f));
        g.drawText (f >= 1000.0f ? String (f / 1000.0f, 0) + "k" : String ((int) f),
                    Rectangle<float> (xOf (f) - 16.0f, graph.getBottom() - 14.0f, 32.0f, 12.0f), Justification::centred);
    }
    for (float db : { -24.0f, -12.0f, 0.0f, 12.0f })
    {
        g.setColour (Colours::white.withAlpha (db == 0.0f ? 0.16f : 0.06f));
        g.drawHorizontalLine ((int) yOf (db), graph.getX(), graph.getRight());
        g.setColour (ui::dim.withAlpha (0.8f));
        g.drawText (String ((int) db), Rectangle<float> (graph.getX() + 3.0f, yOf (db) - 12.0f, 26.0f, 11.0f), Justification::centredLeft);
    }

    Path curve;
    const int w = (int) graph.getWidth();
    for (int px = 0; px <= w; ++px)
    {
        const float f = fLo * std::pow (fHi / fLo, (float) px / (float) w);
        const float db = jlimit (dbLo, dbHi, 20.0f * std::log10 (jmax (1.0e-4f, solowah::magnitude (st, 48000.0, f))));
        if (px == 0) curve.startNewSubPath (graph.getX(), yOf (db));
        else         curve.lineTo (graph.getX() + (float) px, yOf (db));
    }
    Path fill (curve);
    fill.lineTo (graph.getRight(), graph.getBottom());
    fill.lineTo (graph.getX(), graph.getBottom());
    fill.closeSubPath();
    g.setGradientFill (ColourGradient (ui::amber.withAlpha (0.45f), 0.0f, graph.getY(), ui::amber.withAlpha (0.0f), 0.0f, graph.getBottom(), false));
    g.fillPath (fill);
    g.setColour (ui::amber);
    g.strokePath (curve, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));

    const float mx = xOf (st.fc);
    g.setColour (ui::amber.withAlpha (0.5f));
    g.drawVerticalLine ((int) mx, graph.getY(), graph.getBottom());

    // readouts
    g.setColour (ui::dim);
    g.setFont (makeFont (11.0f, true));
    g.drawText ("CENTRE FREQUENCY", readout.removeFromTop (14.0f), Justification::centredLeft);
    g.setColour (ui::text);
    g.setFont (makeFont (40.0f, true));
    const String freqText = st.fc >= 1000.0f ? String (st.fc / 1000.0f, 2) + " kHz" : String (juce::roundToInt (st.fc)) + " Hz";
    g.drawText (freqText, readout.removeFromTop (48.0f), Justification::centredLeft);

    g.setColour (ui::dim);
    g.setFont (makeFont (12.0f));
    g.drawText ("Pedal " + String (juce::roundToInt (pedal * 100.0f)) + " %   |   Q " + String (st.q, 1)
                    + "   |   " + String (solowah::voicing (voice).name),
                readout.removeFromTop (18.0f), Justification::centredLeft);
}

//==============================================================================
// ChoiceSelector
//==============================================================================
ChoiceSelector::ChoiceSelector (RangedAudioParameter& param, StringArray names)
    : items (std::move (names)),
      attachment (param, [this] (float v) { selected = juce::roundToInt (v); repaint(); })
{
    setMouseCursor (MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void ChoiceSelector::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0c0f));
    g.fillRoundedRectangle (b, 8.0f);
    const float segW = b.getWidth() / (float) items.size();
    for (int i = 0; i < items.size(); ++i)
    {
        const auto seg = Rectangle<float> (b.getX() + segW * (float) i, b.getY(), segW, b.getHeight()).reduced (3.0f);
        if (i == selected)
        {
            g.setColour (ui::amber);
            g.fillRoundedRectangle (seg, 6.0f);
        }
        g.setColour (i == selected ? Colour (0xff1a1205) : ui::dim);
        g.setFont (makeFont (12.5f, true));
        g.drawText (items[i], seg, Justification::centred);
    }
    g.setColour (ui::edge);
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);
}

void ChoiceSelector::mouseDown (const MouseEvent& e)
{
    const int idx = jlimit (0, items.size() - 1, (int) (e.position.x / ((float) getWidth() / (float) items.size())));
    attachment.setValueAsCompleteGesture ((float) idx);
}

//==============================================================================
// KnobLookAndFeel
//==============================================================================
KnobLookAndFeel::KnobLookAndFeel()
{
    setColour (Slider::textBoxTextColourId, ui::text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
}

void KnobLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, Slider&)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // value arc
    const float arcR = radius - 1.0f;
    Path track, value;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (Colour (0xff0b0c0f));
    g.strokePath (track, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (ui::amber);
    g.strokePath (value, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // knob body
    const float bodyR = radius - 9.0f;
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - bodyR, c.y - bodyR + 3.0f, bodyR * 2.0f, bodyR * 2.0f);
    g.setGradientFill (ColourGradient (Colour (0xff4a4e58), c.x, c.y - bodyR, Colour (0xff17181c), c.x, c.y + bodyR, false));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (Colours::white.withAlpha (0.16f));
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // pointer
    Path p;
    p.addRoundedRectangle (-1.8f, -bodyR + 4.0f, 3.6f, bodyR * 0.45f, 1.5f);
    g.setColour (ui::amber);
    g.fillPath (p, AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
// SoloWahEditor
//==============================================================================
SoloWahEditor::SoloWahEditor (SoloWahProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      pedal (*p.apvts.getParameter ("pedal")),
      display (p.apvts),
      voicingSelector (*p.apvts.getParameter ("voicing"),
                       StringArray { solowah::voicing (0).name, solowah::voicing (1).name, solowah::voicing (2).name })
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (pedal);
    addAndMakeVisible (display);
    addAndMakeVisible (voicingSelector);

    addKnob (resonance, "RESONANCE", "resonance");
    addKnob (range,     "RANGE",     "range");
    addKnob (drive,     "DRIVE",     "drive");
    addKnob (mix,       "MIX",       "mix");
    addKnob (output,    "OUTPUT",    "output");

    setSize (780, 556);
}

SoloWahEditor::~SoloWahEditor() { setLookAndFeel (nullptr); }

void SoloWahEditor::addKnob (Knob& k, const String& label, const String& paramId)
{
    k.label = label;
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (degreesToRadians (225.0f), degreesToRadians (495.0f), true);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 20);
    k.slider.setMouseDragSensitivity (180);
    k.slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    k.slider.setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
    addAndMakeVisible (k.slider);
}

void SoloWahEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (ui::bgTop, 0.0f, 0.0f, ui::bgBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // header
    g.setColour (ui::text);
    g.setFont (makeFont (30.0f, true));
    g.drawText ("SOLO WAH", Rectangle<int> (24, 12, 260, 40), Justification::centredLeft);
    g.setColour (ui::amber);
    g.fillRoundedRectangle (24.0f, 52.0f, 54.0f, 3.0f, 1.5f);
    g.setColour (ui::dim);
    g.setFont (makeFont (12.0f));
    g.drawText ("Expressive lead wah", Rectangle<int> (90, 44, 200, 16), Justification::centredLeft);

    auto drawPanel = [&] (Rectangle<int> r)
    {
        g.setColour (ui::panel);
        g.fillRoundedRectangle (r.toFloat(), 12.0f);
        g.setColour (ui::edge);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 12.0f, 1.0f);
    };
    drawPanel (pedalPanel);
    drawPanel (displayPanel);
    drawPanel (knobPanel);

    // knob labels
    g.setColour (ui::dim);
    g.setFont (makeFont (11.5f, true));
    for (auto* k : { &resonance, &range, &drive, &mix, &output })
        g.drawText (k->label, k->slider.getBounds().withHeight (16).translated (0, -14), Justification::centred);
}

void SoloWahEditor::resized()
{
    auto r = getLocalBounds();
    voicingSelector.setBounds (r.getWidth() - 24 - 340, 14, 340, 34);

    pedalPanel   = { 20, 74, 452, 318 };
    displayPanel = { 486, 74, 274, 318 };
    knobPanel    = { 20, 406, 740, 134 };

    pedal.setBounds (pedalPanel.reduced (1));
    display.setBounds (displayPanel);

    const int cellW = knobPanel.getWidth() / 5;
    int i = 0;
    for (auto* k : { &resonance, &range, &drive, &mix, &output })
    {
        const int cx = knobPanel.getX() + cellW * i + cellW / 2;
        k->slider.setBounds (cx - 52, knobPanel.getY() + 26, 104, 104);
        ++i;
    }
}
