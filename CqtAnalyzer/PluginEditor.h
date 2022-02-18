#pragma once

#include "PluginProcessor.h"

#include "../include/gui/MagnitudesComponent.h"
#include "../include/gui/OtherLookAndFeel.h"

//==============================================================================
class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void channelButtonClicked(const int channel);
    void rangeSliderChanged();
    void tuningSliderChanged();
    void smoothingSliderChanged();

    AudioPluginAudioProcessor& processorRef;
    juce::AudioProcessorValueTreeState& mParameters;

    juce::Label mChannelLabel;
    juce::Label mRangeLabel;
    juce::Label mTuningLabel;
    juce::Label mSmoothingLabel;
    
    juce::Label mHeadingLabel;
    juce::Label mVersionLabel;
    juce::Label mWebsiteLabel;

    juce::TextButton mLeftChannelButton{"L"};
    juce::TextButton mRightChannelButton{"R"};
    juce::TextButton mMidChannelButton{"M"};
    juce::TextButton mSideChannelButton{"S"};

    juce::Slider mRangeSlider;
    juce::Slider mTuningSlider;
    juce::Slider mSmoothingSlider;

    juce::TooltipWindow mFrequencyTooltip;

    MagnitudesComponent<BinsPerOctave, OctaveNumber> mMagnitudesComponent{ processorRef };

    OtherLookAndFeel mOtherLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
