#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout SoloWahProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct = [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + " %"; };

    // The pedal. 0 = heel down (dark), 1 = toe down (bright). Automatable / MIDI-learnable in the host.
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "pedal", 1 }, "Pedal",
                    NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.5f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "voicing", 1 }, "Voicing",
                    StringArray { solowah::voicing (0).name, solowah::voicing (1).name, solowah::voicing (2).name }, 1));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "resonance", 1 }, "Resonance",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "range", 1 }, "Range",
                    NormalisableRange<float> (0.5f, 1.5f, 0.001f), 1.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (
                        [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + " %"; })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                    NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (
                        [] (float v, int) { return String (v, 1) + " dB"; })));

    return { p.begin(), p.end() };
}

SoloWahProcessor::SoloWahProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pPedal   = apvts.getRawParameterValue ("pedal");
    pVoicing = apvts.getRawParameterValue ("voicing");
    pRes     = apvts.getRawParameterValue ("resonance");
    pRange   = apvts.getRawParameterValue ("range");
    pDrive   = apvts.getRawParameterValue ("drive");
    pMix     = apvts.getRawParameterValue ("mix");
    pOut     = apvts.getRawParameterValue ("output");
}

bool SoloWahProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo());
}

void SoloWahProcessor::prepareToPlay (double sampleRate, int)
{
    wah.prepare (sampleRate, getTotalNumOutputChannels());
}

void SoloWahProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;

    // Hardware expression pedals (CC 11 expression, CC 4 foot controller) move the on-screen pedal.
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isController() && (msg.getControllerNumber() == 11 || msg.getControllerNumber() == 4))
            if (auto* param = apvts.getParameter ("pedal"))
                param->setValueNotifyingHost ((float) msg.getControllerValue() / 127.0f);
    }

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    wah.setParams (pPedal->load(), (int) std::lround (pVoicing->load()), pRes->load(), pRange->load(),
                   pDrive->load(), pMix->load(), pOut->load());
    wah.process (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
}

AudioProcessorEditor* SoloWahProcessor::createEditor() { return new SoloWahEditor (*this); }

void SoloWahProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void SoloWahProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SoloWahProcessor(); }
