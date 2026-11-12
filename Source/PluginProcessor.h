#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "WahDSP.h"

class SoloWahProcessor : public juce::AudioProcessor
{
public:
    SoloWahProcessor();
    ~SoloWahProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Solo Wah"; }
    bool acceptsMidi() const override { return true; }   // CC 11 / CC 4 drive the pedal
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    solowah::WahFilter wah;
    std::atomic<float>* pPedal = nullptr;
    std::atomic<float>* pVoicing = nullptr;
    std::atomic<float>* pRes = nullptr;
    std::atomic<float>* pRange = nullptr;
    std::atomic<float>* pDrive = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOut = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoloWahProcessor)
};
