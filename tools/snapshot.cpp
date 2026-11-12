// Renders the real plugin editor to PNG files (one per pedal position) without needing a DAW.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::File outDir (argc > 1 ? argv[1] : ".");
    outDir.createDirectory();

    for (float pos : { 0.0f, 0.5f, 1.0f })
    {
        SoloWahProcessor proc;
        proc.apvts.getParameter ("pedal")->setValueNotifyingHost (pos);
        std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
        auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);

        auto file = outDir.getChildFile ("ui_pedal_" + juce::String (juce::roundToInt (pos * 100)) + ".png");
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat png;
        png.writeImageToStream (img, out);
        std::printf ("wrote %s (%dx%d)\n", file.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    }

    // ---- interaction self-test: synthetic mouse drag / wheel / MIDI CC through the real components ----
    int fails = 0;
    auto check = [&] (bool ok, const char* what) { std::printf ("%-58s %s\n", what, ok ? "PASS" : "FAIL"); fails += ! ok; };
    {
        SoloWahProcessor proc;
        std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
        auto* pedal = dynamic_cast<PedalComponent*> (editor->getChildComponent (0));
        auto value = [&] { return proc.apvts.getRawParameterValue ("pedal")->load(); };
        check (pedal != nullptr, "editor contains the pedal component");

        auto src = juce::Desktop::getInstance().getMainMouseSource();
        auto ev = [&] (float y, float downY, bool dragged)
        {
            return juce::MouseEvent (src, { 100.0f, y }, juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, pedal, pedal,
                                     juce::Time::getCurrentTime(), { 100.0f, downY }, juce::Time::getCurrentTime(), 1, dragged);
        };

        pedal->mouseDown (ev (100, 100, false));
        pedal->mouseDrag (ev (160, 100, true));       // 60 px down  -> +0.25
        check (std::abs (value() - 0.75f) < 0.01f, "drag down 60px -> toe down (pedal 0.75)");
        pedal->mouseDrag (ev (40, 100, true));        // 60 px up    -> -0.25
        check (std::abs (value() - 0.25f) < 0.01f, "drag up 60px -> toe up (pedal 0.25)");
        pedal->mouseDrag (ev (700, 100, true));
        check (value() == 1.0f, "drag far down clamps at 1.0");
        pedal->mouseDrag (ev (-700, 100, true));
        check (value() == 0.0f, "drag far up clamps at 0.0");
        pedal->mouseDoubleClick (ev (100, 100, false));
        pedal->mouseUp (ev (100, 100, false));
        check (std::abs (value() - 0.5f) < 0.001f, "double-click re-centres pedal");

        juce::MouseWheelDetails wheel { 0.0f, -1.0f, false, false, false };   // scroll down
        pedal->mouseWheelMove (ev (100, 100, false), wheel);
        check (std::abs (value() - 0.65f) < 0.01f, "wheel scroll down -> toe down (+0.15)");
        wheel.deltaY = 2.0f;
        pedal->mouseWheelMove (ev (100, 100, false), wheel);
        check (std::abs (value() - 0.35f) < 0.01f, "wheel scroll up x2 -> toe up (-0.30)");

        // MIDI expression pedal + real audio block through the whole processor
        proc.setPlayConfigDetails (2, 2, 48000.0, 256);
        proc.prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> buf (2, 256);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 256; ++i) buf.setSample (c, i, 0.3f * std::sin (0.05f * (float) i));
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 11, 127), 0);
        proc.processBlock (buf, midi);
        check (value() > 0.99f, "MIDI CC11 = 127 sets pedal to toe-down");
        bool ok = true; for (int i = 0; i < 256; ++i) ok &= std::isfinite (buf.getSample (0, i));
        check (ok && buf.getMagnitude (0, 256) > 0.0f, "processBlock produces finite, non-silent audio");

        // state save / restore
        proc.apvts.getParameter ("pedal")->setValueNotifyingHost (0.8f);
        juce::MemoryBlock mb; proc.getStateInformation (mb);
        proc.apvts.getParameter ("pedal")->setValueNotifyingHost (0.1f);
        proc.setStateInformation (mb.getData(), (int) mb.getSize());
        check (std::abs (value() - 0.8f) < 0.001f, "plugin state saves and restores");
    }
    std::printf (fails ? "%d FAILURE(S)\n" : "All interaction checks passed\n", fails);
    return fails ? 1 : 0;
}
