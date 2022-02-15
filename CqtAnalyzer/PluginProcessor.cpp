#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    for (int i = 0; i < OctaveNumber; i++)
    {
        Cqt::ScheduleElement schedule;
        schedule.octave = i;
        mCqtTimers.push_back(std::make_unique<TimerMt>(std::bind(&AudioPluginAudioProcessor::threadedCqtCall, this, schedule)));
    }

    // reset feature buffers
    for (int o = 0; o < OctaveNumber; o++)
    {
        for (int tone = 0; tone < BinsPerOctave; tone++)
        {
            mCqtDataStorage[o][tone] = 0.;
            mKernelFreqs[o][tone] = 0.;
        }
    }
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // mutex

    // initialize the cqt
    std::vector<int> hopSizes(OctaveNumber);
    for (int o = 0; o < OctaveNumber; o++)
    {
        hopSizes[o] = Cqt::Fft_Size / std::pow(2, o);
    }
    mCqt.init(hopSizes);
    mCqt.initFs(sampleRate, samplesPerBlock);
    mCqtSampleBuffer.resize(samplesPerBlock, 0.);

    // reset feature buffers
    for (int o = 0; o < OctaveNumber; o++)
    {
        for (int tone = 0; tone < BinsPerOctave; tone++)
        {
            mCqtDataStorage[o][tone] = 0.;
            mKernelFreqs[o][tone] = 0.;
        }
    }

    // configure timers
    for (int i = 0; i < OctaveNumber; i++)
    {
        mCqtTimers[i]->stop();
        mCqtTimers[i]->setSingleShot(false);
        mCqtTimers[i]->setInterval(std::chrono::milliseconds(static_cast<size_t>(mCqt.getLatencyMs(i))));
        mCqtTimers[i]->start(true);
    }

    const auto kernelFreqs = mCqt.getKernelFreqs();
    for (int o = 0; o < OctaveNumber; o++)
    {
        for (int tone = 0; tone < BinsPerOctave; tone++)
        {
            mKernelFreqs[o][tone] = kernelFreqs[o][tone];
        }
    }
    mNewKernelFreqs = true;
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    switch(mChannel)
    {
        case 0:
        {
            auto* channelDataL = buffer.getReadPointer (0);
            for(int s = 0; s < buffer.getNumSamples(); s++)
            {
                mCqtSampleBuffer[s] = static_cast<double>(channelDataL[s]);
            }
            break;
        }
        case 1:
        {
            auto* channelDataR = buffer.getReadPointer (1);
            for(int s = 0; s < buffer.getNumSamples(); s++)
            {
                mCqtSampleBuffer[s] = static_cast<double>(channelDataR[s]);
            }
            break;
        }
        case 2:
        {
            auto* channelDataL = buffer.getReadPointer (0);
            auto* channelDataR = buffer.getReadPointer (1);
            for(int s = 0; s < buffer.getNumSamples(); s++)
            {
                mCqtSampleBuffer[s] = static_cast<double>(channelDataL[s] + channelDataR[s]);
            }
            break;
        }
        case 3:
        {
            auto* channelDataL = buffer.getReadPointer (0);
            auto* channelDataR = buffer.getReadPointer (1);
            for(int s = 0; s < buffer.getNumSamples(); s++)
            {
                mCqtSampleBuffer[s] = static_cast<double>(channelDataL[s] - channelDataR[s]);
            }
            break;
        }
        default:
        break;
    }
    mCqt.inputBlock(mCqtSampleBuffer.data(), buffer.getNumSamples());
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    switch(mChannel)
    {
        case 0:
        {
            auto* channelDataL = buffer.getReadPointer (0);
            for(int s = 0; s < buffer.getNumSamples(); s++)
            {
                mCqtSampleBuffer[s] = static_cast<double>(channelDataL[s]);
            }
            break;
        }
        case 1:
        {
            auto* channelDataR = buffer.getReadPointer (1);
            for(int s = 0; s < buffer.getNumSamples(); s++)
            {
                mCqtSampleBuffer[s] = static_cast<double>(channelDataR[s]);
            }
            break;
        }
        case 2:
        {
            auto* channelDataL = buffer.getReadPointer (0);
            auto* channelDataR = buffer.getReadPointer (1);
            for(int s = 0; s < buffer.getNumSamples(); s++)
            {
                mCqtSampleBuffer[s] = static_cast<double>(channelDataL[s] + channelDataR[s]);
            }
            break;
        }
        case 3:
        {
            auto* channelDataL = buffer.getReadPointer (0);
            auto* channelDataR = buffer.getReadPointer (1);
            for(int s = 0; s < buffer.getNumSamples(); s++)
            {
                mCqtSampleBuffer[s] = static_cast<double>(channelDataL[s] - channelDataR[s]);
            }
            break;
        }
        default:
        break;
    }
    mCqt.inputBlock(mCqtSampleBuffer.data(), buffer.getNumSamples());
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

//==============================================================================

void AudioPluginAudioProcessor::setTuning(const double tuning)
{ 
    mCqt.setConcertPitch(tuning); 
    const auto kernelFreqs = mCqt.getKernelFreqs();
    for (int o = 0; o < OctaveNumber; o++)
    {
        for (int tone = 0; tone < BinsPerOctave; tone++)
        {
            mKernelFreqs[o][tone] = kernelFreqs[o][tone];
        }
    }
    mNewKernelFreqs = true;
};

void AudioPluginAudioProcessor::threadedCqtCall(const Cqt::ScheduleElement schedule)
{
    mCqt.cqt(schedule);
    auto cqtData = mCqt.getOctaveCqtBuffer(schedule.octave);
    for (size_t tone = 0; tone < BinsPerOctave; tone++)
    {
        const double realD = (*cqtData)[tone].real();
        const double imagD = (*cqtData)[tone].imag();
        const double magnitude = std::sqrt(std::pow(realD, 2) + std::pow(imagD, 2));
        mCqtDataStorage[schedule.octave][tone] = magnitude;
    }
}


