#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"

#if IPLUG_DSP
#include "../../../lib/FFTDomain/ConstantQTransform.h"
#endif

constexpr int BinsPerOctave{ 12 };
constexpr int OctaveNumber{ 8 };
constexpr int OctaveBufferSize{ BinsPerOctave + 1 };

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum EControlTags
{
	kCtrlTagCqtVis = 0,
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
  void OnIdle()  override { mCqtSender.TransmitData(*this); };

  std::vector<double> mCqtSampleBuffer;
  Cqt::ConstantQTransform<BinsPerOctave, OctaveNumber> mCqt;

  ISender<1, 64, std::array<double, OctaveBufferSize> > mCqtSender;
  ISenderData<1, std::array<double, OctaveBufferSize> > mSenderBuffer;

  WDL_Mutex mMutex;
#endif
};
