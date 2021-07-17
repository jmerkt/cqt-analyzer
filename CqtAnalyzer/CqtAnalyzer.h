#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"

#if IPLUG_DSP
#include "../include/TimerMt.h"
#include "../../../lib/FFTDomain/ConstantQTransform.h"
#endif

/*
* TODO: Multichannel - channel selection
* 
* (Leaky integration for GUI)
*/


constexpr int BinsPerOctave{ 12 };
constexpr int OctaveNumber{ 9 };
constexpr int OctaveBufferSize{ BinsPerOctave + 4 };
constexpr size_t FeatureUpdateRate{ 20 };

const int kNumPresets = 1;

enum EParams
{
  kTuning = 0,
  kChannel,
  kMagMin,
  kMagMax,
  kNumParams
};

enum EControlTags
{
	kCtrlTagCqtVis = 0,
	kCtrlTagChromaVis,
	kCtrlTagOctaveMagnitudesVis,
	kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class CqtAnalyzer final : public Plugin
{
public:
  CqtAnalyzer(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx)  override;
  void OnIdle()  override;

  std::vector<double> mCqtSampleBuffer;
  Cqt::ConstantQTransform<BinsPerOctave, OctaveNumber> mCqt;

  ISender<1, 64, std::array<double, OctaveBufferSize> > mCqtSender[OctaveNumber];
  ISenderData<1, std::array<double, OctaveBufferSize> > mSenderBuffer[OctaveNumber];

  ISender<1, 64, std::array<double, BinsPerOctave> > mChromaFeatureSender;
  ISenderData<1, std::array<double, BinsPerOctave> > mChromaFeatureBuffer;

  ISender<1, 64, std::array<double, OctaveNumber> > mOctaveMagnitudesSender;
  ISenderData<1, std::array<double, OctaveNumber> > mOctaveMagnitudesBuffer;

  WDL_Mutex mMutex;
  
  void threadedCqtCall(const Cqt::ScheduleElement schedule);
  void updateChromaFeature();
  void updateOctaveMagnitudes();

  std::vector<std::unique_ptr<TimerMt>> mCqtTimers;
  std::unique_ptr<TimerMt> mChromaFeatureTimer;
  std::unique_ptr<TimerMt> mOctaveMagnitudesTimer;

  double mCqtDataStorage[OctaveNumber][BinsPerOctave];
  double mChromaFeature[BinsPerOctave];
  double mOctaveMagnitudes[OctaveNumber];

  int mNumChans{ 1 };
  std::atomic<bool> numChansChanged{ true };
  int mChannel{ 0 };
  double mMagMin{ -120. };
  double mMagMax{ 0. };
  double mTuning{ 440. };

  const double mOctaveOverlaps[OctaveNumber] = {0., 0.1, 0.2, 0.4, 0.6, 0.8, 0.9, 0.95, 0.975};

#endif
};
