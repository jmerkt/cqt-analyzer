#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../include/TimerMt.h"
#include "../submodules/rt-cqt/include/ConstantQTransform.h"

constexpr int BinsPerOctave{ 48 };
constexpr int OctaveNumber{ 10 };

//==============================================================================
class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    double mCqtDataStorage[OctaveNumber][BinsPerOctave];
    double mKernelFreqs[OctaveNumber][BinsPerOctave];
    bool mNewKernelFreqs{false};
    void setTuning(const double tuning);
    void setChannel(const int channel);
    void setSmoothing(const double smoothingUp, const double smoothingDown);
    void setRange(const double rangeMin, const double rangeMax);
private:
    //==============================================================================
    std::vector<double> mCqtSampleBuffer;
    Cqt::ConstantQTransform<BinsPerOctave, OctaveNumber> mCqt;

    void threadedCqtCall(const Cqt::ScheduleElement schedule);

    std::vector<std::unique_ptr<TimerMt>> mCqtTimers;

    juce::AudioProcessorValueTreeState mParameters;
    juce::AudioParameterInt* mChannelParameter{ nullptr };
    juce::AudioParameterFloat* mTuningParameter{ nullptr };
    juce::AudioParameterFloat* mRangeMinParameter{ nullptr };
    juce::AudioParameterFloat* mRangeMaxParameter{ nullptr };
    juce::AudioParameterFloat* mSmoothingUpParameter{ nullptr };
    juce::AudioParameterFloat* mSmoothingDownParameter{ nullptr };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
