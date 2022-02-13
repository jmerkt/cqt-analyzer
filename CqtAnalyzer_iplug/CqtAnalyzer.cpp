#include "CqtAnalyzer.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "../../include/gui/ICqtMagnitudes.h"
#include "../../include/gui/IChromaFeature.h"
#include "../../include/gui/IOctaveMagnitudes.h"

CqtAnalyzer::CqtAnalyzer(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // controls
  GetParam(kTuning)->InitDouble("Tuning", 440., 415., 465.0, 0.01, "Hz");
  GetParam(kChannel)->InitEnum("Channel", 0, 1, "", IParam::kFlagsNone, "");
  GetParam(kMagMin)->InitDouble("MagMin", -50., -120., 20., 1., "dB");
  GetParam(kMagMax)->InitDouble("MagMax", 10., -120., 40., 1., "dB");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() 
  {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) 
  {
    const IVStyle style
    {
        true, // Show label
        true, // Show value
    {
    DEFAULT_BGCOLOR, // Background
    DEFAULT_FGCOLOR, // Foreground
    DEFAULT_PRCOLOR, // Pressed
    COLOR_BLACK, // Frame
    DEFAULT_HLCOLOR, // Highlight
    DEFAULT_SHCOLOR, // Shadow
    COLOR_BLACK, // Extra 1
    DEFAULT_X2COLOR, // Extra 2
    DEFAULT_X3COLOR  // Extra 3
    }, // Colors
    IText(14.f, EAlign::Center, COLOR_WHITE), // Label text
    IText(14.f, EVAlign::Bottom, COLOR_WHITE) // Value text
    };
    IText labelTexts{ 20.f, COLOR_WHITE, "Ariblk", EAlign::Center };
    IColor backgroundColor{ 255, 13, 7, 63 };

    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(backgroundColor);

    pGraphics->LoadFont("Roboto-Regular", ARIALBD_FN);
    pGraphics->LoadFont("Arialbd", ARIALBD_FN);
    pGraphics->LoadFont("Ariblk", ARIBLK_FN);
    IRECT b = pGraphics->GetBounds();

    const float controlYFrac = 0.04;
    const float headingYFrac = 0.08;
    const float magXFrac = 0.8; 

    auto controlRect = b.GetReducedFromBottom((1.f - controlYFrac) * b.H());
    controlRect.ReduceFromRight((1.f - magXFrac) * b.W());

    auto spectrumRect = b.GetReducedFromTop((controlYFrac) * b.H());
    spectrumRect.ReduceFromBottom(headingYFrac * b.H());
    spectrumRect.ReduceFromRight((1.f - magXFrac) * b.W());

    auto featureRect = b.GetReducedFromLeft(magXFrac * b.W());
    featureRect.ReduceFromBottom(headingYFrac * b.H());

    auto headingRect = b.GetReducedFromTop((1.f - headingYFrac) * b.H());

    // controls
    const float numControls = 4.f;
    const float numLabels = 3.f;
    const float controlWidth = 1.f / (numControls + numLabels);
    const float controlFill = 0.8f;
    IText labelText{ 15.f, COLOR_WHITE, "Ariblk", EAlign::Center };

    controlRect.ReduceFromRight((1.f - controlWidth) * controlRect.W());
    pGraphics->AttachControl(new ITextControl(controlRect, "Channel: ", labelText));
    controlRect.Translate(controlRect.W(), 0.f);
    pGraphics->AttachControl(new ICaptionControl(controlRect.GetScaledAboutCentre(controlFill), kChannel, IText(15.f), DEFAULT_FGCOLOR, true), kNoTag, "");
    controlRect.Translate(controlRect.W(), 0.f);
    pGraphics->AttachControl(new ITextControl(controlRect, "Range: ", labelText));
    controlRect.Translate(controlRect.W(), 0.f);
    pGraphics->AttachControl(new ICaptionControl(controlRect.GetScaledAboutCentre(controlFill), kMagMin, IText(15.f), DEFAULT_FGCOLOR, true), kNoTag, "");
    controlRect.Translate(controlRect.W(), 0.f);
    pGraphics->AttachControl(new ICaptionControl(controlRect.GetScaledAboutCentre(controlFill), kMagMax, IText(15.f), DEFAULT_FGCOLOR, true), kNoTag, "");
    controlRect.Translate(controlRect.W(), 0.f);
    pGraphics->AttachControl(new ITextControl(controlRect, "Tuning: ", labelText));
    controlRect.Translate(controlRect.W(), 0.f);
    pGraphics->AttachControl(new ICaptionControl(controlRect.GetScaledAboutCentre(controlFill), kTuning, IText(15.f), DEFAULT_FGCOLOR, true), kNoTag, "");

    // spectrum
    pGraphics->AttachControl(new ICqtMagnitudes<BinsPerOctave, OctaveNumber>(spectrumRect, style), kCtrlTagCqtVis, "");

    //features
    const float midGap = 0.01f;
    auto chromaRect = featureRect.GetReducedFromBottom(featureRect.H() / 2.f + featureRect.H() * midGap);
    pGraphics->AttachControl(new IChromaFeature<BinsPerOctave>(chromaRect, style), kCtrlTagChromaVis, "");
    auto magRect = featureRect.GetReducedFromTop(featureRect.H() / 2.f + featureRect.H() * midGap);
    pGraphics->AttachControl(new IOctaveMagnitudes<OctaveNumber>(magRect, style), kCtrlTagOctaveMagnitudesVis, "");

    // heading
    const float sideGap = 0.02f;
    IText headingText{ 30.f, COLOR_WHITE, "Ariblk", EAlign::Center };
    pGraphics->AttachControl(new ITextControl(headingRect, "", headingText, IColor{ 255, 0, 0, 0 }));
    pGraphics->AttachControl(new ITextControl(headingRect, "CqtAnalyzer", headingText));
    IText versionText{ 18.f, COLOR_WHITE, "Ariblk", EAlign::Near };
    std::string version = std::string("Version ") + PLUG_VERSION_STR;
    pGraphics->AttachControl(new ITextControl(headingRect.GetReducedFromLeft(sideGap * headingRect.W()), version.c_str(), versionText));
    IText webpageText{ 18.f, COLOR_WHITE, "Ariblk", EAlign::Far };
    pGraphics->AttachControl(new ITextControl(headingRect.GetReducedFromRight(sideGap * headingRect.W()), "www.ChromaDSP.com", webpageText));

  };
#endif

#if IPLUG_DSP
  for (int i = 0; i < OctaveNumber; i++)
  {
      Cqt::ScheduleElement schedule;
      schedule.octave = i;
      mCqtTimers.push_back(std::make_unique<TimerMt>(std::bind(&CqtAnalyzer::threadedCqtCall, this, schedule)));
  }
  mChromaFeatureTimer = std::make_unique<TimerMt>(std::bind(&CqtAnalyzer::updateChromaFeature, this));
  mOctaveMagnitudesTimer = std::make_unique<TimerMt>(std::bind(&CqtAnalyzer::updateOctaveMagnitudes, this));
#endif
}

#if IPLUG_DSP
void CqtAnalyzer::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nOutChans = NOutChansConnected();
  const int nInChans = NInChansConnected();
  if (nInChans != mNumChans)
  {
      mNumChans = nInChans;
      numChansChanged.store(true);
  }

  // send data into cqt filter bank
  for (int i = 0; i < nFrames; i++)
  {
      mCqtSampleBuffer[i] = inputs[mChannel][i];
  }
  mCqt.inputBlock(mCqtSampleBuffer.data(), nFrames);

  // bypass audio
  for (int s = 0; s < nFrames; s++) 
  {
    for (int c = 0; c < nOutChans; c++)
    {
      outputs[c][s] = inputs[c][s];
    }
  }
}

void CqtAnalyzer::threadedCqtCall(const Cqt::ScheduleElement schedule)
{
    mCqt.cqt(schedule);
    auto cqtData = mCqt.getOctaveCqtBuffer(schedule.octave);
    // send data to gui
    mSenderBuffer[schedule.octave].vals[0][0] = static_cast<double>(schedule.octave);
    mSenderBuffer[schedule.octave].vals[0][1] = mMagMin;
    mSenderBuffer[schedule.octave].vals[0][2] = mMagMax;
    mSenderBuffer[schedule.octave].vals[0][3] = mTuning;
    for (size_t tone = 0; tone < BinsPerOctave; tone++)
    {
        const double realD = (*cqtData)[tone].real();
        const double imagD = (*cqtData)[tone].imag();
        const double magnitude = std::sqrt(std::pow(realD, 2) + std::pow(imagD, 2));
        mSenderBuffer[schedule.octave].vals[0][tone + 4] = magnitude;
        mCqtDataStorage[schedule.octave][tone] = magnitude;
    }
    // push octave data
    mCqtSender[schedule.octave].PushData(mSenderBuffer[schedule.octave]);
}

void CqtAnalyzer::OnReset()
{
    WDL_MutexLock lock(&mMutex);

    const double samplerate = GetSampleRate();
    const int blockSize = GetBlockSize();

    // initialize the cqt
    std::vector<int> hopSizes(OctaveNumber);
    for (int o = 0; o < OctaveNumber; o++)
    {
        hopSizes[o] = Cqt::Fft_Size / std::pow(2, o);
    }
    mCqt.init(hopSizes);
    mCqt.initFs(samplerate, blockSize);
    mCqtSampleBuffer.resize(blockSize, 0.);

    // reset feature buffers
    for (int o = 0; o < OctaveNumber; o++)
    {
        for (int tone = 0; tone < BinsPerOctave; tone++)
        {
            mCqtDataStorage[o][tone] = 0.;
        }
    }
    for (int tone = 0; tone < BinsPerOctave; tone++)
    {
        mChromaFeature[tone] = 0.;
    }
    for (int o = 0; o < OctaveNumber; o++)
    {
        mOctaveMagnitudes[o] = 0.;
    }

    // fifo for transporting data from timer threads to gui
    for (int i = 0; i < OctaveNumber; i++)
    {
        mSenderBuffer[i].chanOffset = 0;
        mSenderBuffer[i].ctrlTag = kCtrlTagCqtVis;
        mSenderBuffer[i].nChans = 1;
    }
    mChromaFeatureBuffer.chanOffset = 0;
    mChromaFeatureBuffer.ctrlTag = kCtrlTagChromaVis;
    mChromaFeatureBuffer.nChans = 1;

    mOctaveMagnitudesBuffer.chanOffset = 0;
    mOctaveMagnitudesBuffer.ctrlTag = kCtrlTagOctaveMagnitudesVis;
    mOctaveMagnitudesBuffer.nChans = 1;

    // configure timers
    for (int i = 0; i < OctaveNumber; i++)
    {
        mCqtTimers[i]->stop();
        mCqtTimers[i]->setSingleShot(false);
        mCqtTimers[i]->setInterval(std::chrono::milliseconds(static_cast<size_t>(mCqt.getLatencyMs(i))));
        mCqtTimers[i]->start(true);
    }

    mChromaFeatureTimer->stop();
    mChromaFeatureTimer->setSingleShot(false);
    mChromaFeatureTimer->setInterval(std::chrono::milliseconds(FeatureUpdateRate));
    mChromaFeatureTimer->start(true);

    mOctaveMagnitudesTimer->stop();
    mOctaveMagnitudesTimer->setSingleShot(false);
    mOctaveMagnitudesTimer->setInterval(std::chrono::milliseconds(FeatureUpdateRate));
    mOctaveMagnitudesTimer->start(true);
}

void CqtAnalyzer::OnParamChange(int paramIdx)
{
    switch (paramIdx)
    {
        case kTuning:
        {
            mCqt.setConcertPitch(GetParam(paramIdx)->Value());
            mTuning = GetParam(paramIdx)->Value();
            break;
        }
        case kChannel:
        {
            mChannel = GetParam(paramIdx)->Int();
            break;
        }
        case kMagMin:
        {
            mMagMin = GetParam(paramIdx)->Value();
            break;
        }
        case kMagMax:
        {
            mMagMax = GetParam(paramIdx)->Value();
            break;
        }
    }
}

void CqtAnalyzer::OnIdle() 
{ 
    for (int i = 0; i < OctaveNumber; i++)
    {
        mCqtSender[i].TransmitData(*this);
    } 
    mChromaFeatureSender.TransmitData(*this); 
    mOctaveMagnitudesSender.TransmitData(*this);

    if (numChansChanged.load())
    {
        numChansChanged.store(false);
        GetParam(kChannel)->InitEnum("Channel", 0, mNumChans, "", IParam::kFlagsNone, "");
        for (int i = 0; i < mNumChans; i++)
        {
            GetParam(kChannel)->SetDisplayText(i, std::to_string(i).c_str());
        }
        setDirty(false);
    }
};

void CqtAnalyzer::updateChromaFeature()
{
    for (int tone = 0; tone < BinsPerOctave; tone++)
    {
        mChromaFeature[tone] = 0.;
    }
    for (int octave = 0; octave < OctaveNumber; octave++)
    {
        for (int tone = 0; tone < BinsPerOctave; tone++)
        {
            mChromaFeature[tone] += mCqtDataStorage[octave][tone];
        }
    }
    // calculate mean magnitude
    double mean = 0.;
    for (int tone = 0; tone < BinsPerOctave; tone++)
    {
        mean += mChromaFeature[tone];
    }
    mean *= 1. / static_cast<double>(BinsPerOctave);
    // subtract mean
    for (int tone = 0; tone < BinsPerOctave; tone++)
    {
        mChromaFeature[tone] -= mean;
    }
    // normalize to max of 1
    double max = -1.;
    for (int tone = 0; tone < BinsPerOctave; tone++)
    {
        if (std::abs(mChromaFeature[tone]) > max)
        {
            max = std::abs(mChromaFeature[tone]);
        }
    }
    const double max1Div = max > 1e-8 ? 1. / max : 1.;
    for (int tone = 0; tone < BinsPerOctave; tone++)
    {
        mChromaFeature[tone] *= max1Div;
    }

    // send data to gui
    for (int tone = 0; tone < BinsPerOctave; tone++)
    {
        mChromaFeatureBuffer.vals[0][tone] = mChromaFeature[tone];
    }
    mChromaFeatureSender.PushData(mChromaFeatureBuffer);
}

void CqtAnalyzer::updateOctaveMagnitudes()
{
    for (int octave = 0; octave < OctaveNumber; octave++)
    {
        mOctaveMagnitudes[octave] = 0.;
        for (int tone = 0; tone < BinsPerOctave; tone++)
        {
            mOctaveMagnitudes[octave] += mCqtDataStorage[octave][tone];
        }
    }
    // scale
    for (int octave = 0; octave < OctaveNumber; octave++)
    {
        mOctaveMagnitudes[octave] *= 1. / static_cast<double>(BinsPerOctave);
    }

    // send data to gui
    for (int octave = 0; octave < OctaveNumber; octave++)
    {
        mOctaveMagnitudesBuffer.vals[0][octave] = mOctaveMagnitudes[octave];
    }
    mOctaveMagnitudesSender.PushData(mOctaveMagnitudesBuffer);
}
#endif
