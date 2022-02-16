#include "PluginProcessor.h"
#include "PluginEditor.h"

constexpr float LabelSize{15.f};
constexpr float HeadingSize{30.f};
constexpr float WebsiteSize{16.f};

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    addAndMakeVisible(mMagnitudesComponent);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (PLUGIN_WIDTH, PLUGIN_HEIGHT);
    setResizable(true, true);

    // global LookAndFeel
    getLookAndFeel().setColour(juce::Slider::thumbColourId, juce::Colours::white);
    getLookAndFeel().setColour(juce::Slider::trackColourId, juce::Colours::blue);
    //getLookAndFeel().setColour(juce::Slider::textBoxBackgroundColourId , juce::Colours::grey);

    // labels
    addAndMakeVisible(mChannelLabel);
    addAndMakeVisible(mRangeLabel);
    addAndMakeVisible(mTuningLabel);
    addAndMakeVisible(mSmoothingLabel);
    addAndMakeVisible(mHeadingLabel);
    addAndMakeVisible(mVersionLabel);
    addAndMakeVisible(mWebsiteLabel);

    mChannelLabel.setText("Channel: ", juce::dontSendNotification);
    mRangeLabel.setText("Range: ", juce::dontSendNotification);
    mTuningLabel.setText("Tuning: ", juce::dontSendNotification);
    mSmoothingLabel.setText("Smoothing: ", juce::dontSendNotification);
    mHeadingLabel.setText("CqtAnalyzer", juce::dontSendNotification);
    mVersionLabel.setText("Version 0.2.0", juce::dontSendNotification);
    mWebsiteLabel.setText("www.ChromaDSP.com", juce::dontSendNotification);

    mChannelLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mRangeLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mTuningLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mSmoothingLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mHeadingLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mVersionLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mWebsiteLabel.setColour (juce::Label::textColourId, juce::Colours::white);

    mHeadingLabel.setColour (juce::Label::backgroundColourId, juce::Colours::black);

    mHeadingLabel.setJustificationType (juce::Justification::centred);
    mVersionLabel.setJustificationType (juce::Justification::left);
    mWebsiteLabel.setJustificationType (juce::Justification::right);

    // buttons and controls
    addAndMakeVisible(mLeftChannelButton);
    addAndMakeVisible(mRightChannelButton);
    addAndMakeVisible(mMidChannelButton);
    addAndMakeVisible(mSideChannelButton);

    mLeftChannelButton.onClick = [this] {channelButtonClicked(0);};
    mRightChannelButton.onClick = [this] {channelButtonClicked(1);};
    mMidChannelButton.onClick = [this] {channelButtonClicked(2);};
    mSideChannelButton.onClick = [this] {channelButtonClicked(3);};
    const juce::Colour activeColour = juce::Colours::blue;
    const juce::Colour inactiveColour = juce::Colours::black;
    mLeftChannelButton.setColour (juce::TextButton::buttonColourId, activeColour);
    mRightChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
    mMidChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
    mSideChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);

    addAndMakeVisible(mRangeSlider);
    addAndMakeVisible(mTuningSlider);
    addAndMakeVisible(mSmoothingSlider);

    mRangeSlider.setSliderStyle(juce::Slider::SliderStyle::TwoValueHorizontal);
    mRangeSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    mRangeSlider.setRange(-120., 40., 5.);
    mRangeSlider.setMinValue(-50., juce::dontSendNotification);
    mRangeSlider.setMaxValue(10., juce::dontSendNotification);
    mRangeSlider.onValueChange = [this]{rangeSliderChanged();};

    mTuningSlider.setRange(415., 465., 0.01);
    mTuningSlider.setValue(440., juce::dontSendNotification);
    mTuningSlider.setTextValueSuffix (" Hz");
    mTuningSlider.onValueChange = [this]{tuningSliderChanged();};

    mSmoothingSlider.setRange(0., 1., 0.001);
    mSmoothingSlider.setSkewFactor(4.);
    mSmoothingSlider.setSliderStyle(juce::Slider::SliderStyle::TwoValueHorizontal);
    mSmoothingSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    mSmoothingSlider.setMinValue(0.7, juce::dontSendNotification);
    mSmoothingSlider.setMaxValue(0.85, juce::dontSendNotification);
    mSmoothingSlider.onValueChange = [this]{smoothingSliderChanged();};

    addAndMakeVisible(mFrequencyTooltip);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    //juce::Colour backgroundColour = juce::Colour::fromRGBA(7u, 13u, 255u, 63u );
    juce::Colour backgroundColour = juce::Colours::black;
    g.fillAll (backgroundColour);
}

void AudioPluginAudioProcessorEditor::resized()
{
    const float controlYFrac = 0.04f;
    const float headingYFrac = 0.08f;
    const float magXFrac = 1.0f; 
    auto b = getBounds().toFloat();

    auto controlRect = b.withTrimmedBottom((1.f - controlYFrac) * b.getHeight());
    controlRect = controlRect.withTrimmedRight((1.f - magXFrac) * b.getWidth());

    auto spectrumRect = b.withTrimmedTop((controlYFrac) * b.getHeight());
    spectrumRect = spectrumRect.withTrimmedBottom(headingYFrac * b.getHeight());
    spectrumRect = spectrumRect.withTrimmedRight((1.f - magXFrac) * b.getWidth());

    auto featureRect = b.withTrimmedLeft(magXFrac * b.getWidth());
    featureRect.withTrimmedBottom(headingYFrac * b.getHeight());

    auto headingRect = b.withTrimmedTop((1.f - headingYFrac) * b.getHeight());

    // controls
    const float numControls = 4.f * 2.f; // 2.f to weight controls size
    const float numLabels = 4.f;
    const float controlWidth = 1.f / (numControls + numLabels);
    const float controlFill = 0.8f;

    controlRect = controlRect.withTrimmedRight((1.f - controlWidth) * controlRect.getWidth());
    mChannelLabel.setBounds(controlRect.toNearestIntEdges());
    controlRect.translate(controlRect.getWidth(), 0.f);

    auto channelRect = controlRect.withTrimmedRight(-controlRect.getWidth());
    channelRect = channelRect.withTrimmedRight(0.8f * channelRect.getWidth());
    mLeftChannelButton.setBounds(channelRect.toNearestIntEdges());
    channelRect.translate(channelRect.getWidth(), 0.f);
    mRightChannelButton.setBounds(channelRect.toNearestIntEdges());
    channelRect.translate(channelRect.getWidth(), 0.f);
    mMidChannelButton.setBounds(channelRect.toNearestIntEdges());
    channelRect.translate(channelRect.getWidth(), 0.f);
    mSideChannelButton.setBounds(channelRect.toNearestIntEdges());

    controlRect.translate(controlRect.getWidth() * 2.f, 0.f);
    mRangeLabel.setBounds(controlRect.toNearestIntEdges());
    controlRect.translate(controlRect.getWidth(), 0.f);
    mRangeSlider.setBounds(controlRect.withTrimmedRight(-controlRect.getWidth()).toNearestIntEdges());

    controlRect.translate(controlRect.getWidth()* 2.f, 0.f);
    mSmoothingLabel.setBounds(controlRect.toNearestIntEdges());
    controlRect.translate(controlRect.getWidth(), 0.f);
    mSmoothingSlider.setBounds(controlRect.withTrimmedRight(-controlRect.getWidth()).toNearestIntEdges());

    controlRect.translate(controlRect.getWidth() * 2.f, 0.f);
    mTuningLabel.setBounds(controlRect.toNearestIntEdges());
    controlRect.translate(controlRect.getWidth(), 0.f);
    mTuningSlider.setBounds(controlRect.withTrimmedRight(-controlRect.getWidth()).toNearestIntEdges());

    // spectrum
    mMagnitudesComponent.setBounds(spectrumRect.toNearestIntEdges());

    // heading
    const float sideGap = 0.02f;
    mHeadingLabel.setBounds(headingRect.toNearestIntEdges());
    mVersionLabel.setBounds(headingRect.withTrimmedLeft(sideGap * headingRect.getWidth()).toNearestIntEdges());
    mWebsiteLabel.setBounds(headingRect.withTrimmedRight(sideGap * headingRect.getWidth()).toNearestIntEdges());

    mFrequencyTooltip.setBounds(b.toNearestIntEdges());

    mChannelLabel.setFont (juce::Font (LabelSize / static_cast<float>(PLUGIN_HEIGHT) * b.getHeight(), juce::Font::bold));
    mRangeLabel.setFont (juce::Font (LabelSize / static_cast<float>(PLUGIN_HEIGHT) * b.getHeight(), juce::Font::bold));
    mTuningLabel.setFont (juce::Font (LabelSize / static_cast<float>(PLUGIN_HEIGHT) * b.getHeight(), juce::Font::bold));
    mSmoothingLabel.setFont (juce::Font (LabelSize / static_cast<float>(PLUGIN_HEIGHT) * b.getHeight(), juce::Font::bold));
    mHeadingLabel.setFont (juce::Font (HeadingSize / static_cast<float>(PLUGIN_HEIGHT) * b.getHeight(), juce::Font::bold));
    mVersionLabel.setFont (juce::Font (WebsiteSize / static_cast<float>(PLUGIN_HEIGHT) * b.getHeight(), juce::Font::bold));
    mWebsiteLabel.setFont (juce::Font (WebsiteSize / static_cast<float>(PLUGIN_HEIGHT) * b.getHeight(), juce::Font::bold));
}

void AudioPluginAudioProcessorEditor::channelButtonClicked(const int channel)
{
    const juce::Colour activeColour = juce::Colours::blue;
    const juce::Colour inactiveColour = juce::Colours::black;
    switch(channel)
    {
        case 0:
            mLeftChannelButton.setColour (juce::TextButton::buttonColourId, activeColour);
            mRightChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            mMidChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            mSideChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            break;
        case 1:
            mLeftChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            mRightChannelButton.setColour (juce::TextButton::buttonColourId, activeColour);
            mMidChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            mSideChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            break;
        case 2:
            mLeftChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            mRightChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            mMidChannelButton.setColour (juce::TextButton::buttonColourId, activeColour);
            mSideChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            break;
        case 3:
            mLeftChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            mRightChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            mMidChannelButton.setColour (juce::TextButton::buttonColourId, inactiveColour);
            mSideChannelButton.setColour (juce::TextButton::buttonColourId, activeColour);
            break;
        default:
            break;
    }
    processorRef.setChannel(channel);
}


void AudioPluginAudioProcessorEditor::rangeSliderChanged()
{
    mMagnitudesComponent.setRangeMin(mRangeSlider.getMinValue());
    mMagnitudesComponent.setRangeMax(mRangeSlider.getMaxValue());
}


void AudioPluginAudioProcessorEditor::tuningSliderChanged()
{
    const double tuning = mTuningSlider.getValue();
    processorRef.setTuning(tuning);
    mMagnitudesComponent.setTuning(tuning);
}

void AudioPluginAudioProcessorEditor::smoothingSliderChanged()
{
    mMagnitudesComponent.setSmoothing(mSmoothingSlider.getMinValue(), mSmoothingSlider.getMaxValue());
}