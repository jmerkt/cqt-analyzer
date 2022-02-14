#pragma once

#include "PluginProcessor.h"

#include "../include/gui/MagnitudesComponent.h"

//==============================================================================
class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void channelButtonClicked(const int channel);
    void rangeSliderChanged();
    void tuningSliderChanged();

    AudioPluginAudioProcessor& processorRef;

    juce::Label mChannelLabel;
    juce::Label mRangeLabel;
    juce::Label mTuningLabel;
    
    juce::Label mHeadingLabel;
    juce::Label mVersionLabel;
    juce::Label mWebsiteLabel;

    juce::TextButton mLeftChannelButton{"L"};
    juce::TextButton mRightChannelButton{"R"};
    juce::TextButton mMidChannelButton{"M"};
    juce::TextButton mSideChannelButton{"S"};

    juce::Slider mRangeSlider;
    juce::Slider mTuningSlider;

    MagnitudesComponent<BinsPerOctave, OctaveNumber> mMagnitudesComponent{ processorRef };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
