#include "CqtAnalyzer.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "../../include/gui/CqtVisual.h"

CqtAnalyzer::CqtAnalyzer(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() 
  {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) 
  {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();

    pGraphics->AttachControl(new CqtVisual(b), kCtrlTagCqtVis, "");
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
  const int nChans = NOutChansConnected();

  // send data into cqt filter bank
  for (int i = 0; i < nFrames; i++)
  {
      mCqtSampleBuffer[i] = inputs[0][i];
  }
  mCqt.inputBlock(mCqtSampleBuffer.data(), nFrames);

  // bypass audio
  for (int s = 0; s < nFrames; s++) 
  {
    for (int c = 0; c < nChans; c++) 
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
    for (size_t tone = 0; tone < BinsPerOctave; tone++)
    {
        const double realD = (*cqtData)[tone].real();
        const double imagD = (*cqtData)[tone].imag();
        const double magnitude = std::sqrt(std::pow(realD, 2) + std::pow(imagD, 2));
        mSenderBuffer[schedule.octave].vals[0][tone + 1] = magnitude;
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
    mCqt.init(0.5, 100, {std::begin(mOctaveOverlaps), std::end(mOctaveOverlaps)}, {std::begin(mOctaveEqualization), std::end(mOctaveEqualization)});
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
}

void CqtAnalyzer::OnIdle() 
{ 
    for (int i = 0; i < OctaveNumber; i++)
    {
        mCqtSender[i].TransmitData(*this);
    } 
    mChromaFeatureSender.TransmitData(*this); 
    mOctaveMagnitudesSender.TransmitData(*this);
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
