#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "../../../lib/FFTDomain/ConstantQTransform.h"

constexpr int BinsPerOctave{ 12 };
constexpr int OctaveNumber{ 8 };

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class CqtAnalyzer final : public Plugin
{
public:
  CqtAnalyzer(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;

  Cqt::ConstantQTransform<BinsPerOctave, OctaveNumber> mCqt;

  WDL_Mutex mMutex;
#endif
};
