#include "CqtAnalyzer.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "../../include/gui/CqtVisual.h"

CqtAnalyzer::CqtAnalyzer(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    /*pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "Hello iPlug 2!", IText(50)));
    pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));*/

    pGraphics->AttachControl(new CqtVisual(b), kCtrlTagCqtVis, "");
  };
#endif
}

#if IPLUG_DSP
void CqtAnalyzer::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nChans = NOutChansConnected();
 
  for (int i = 0; i < nFrames; i++)
  {
      mCqtSampleBuffer[i] = inputs[0][i];
  }
  // analyze data
  mCqt.inputBlock(mCqtSampleBuffer.data(), nFrames);
  auto cqtSchedule = mCqt.getCqtSchedule();
  for (auto schedule : cqtSchedule)
  {
      mCqt.cqt(schedule);
      auto cqtData = mCqt.getOctaveCqtBuffer(schedule.octave);
      // send data to gui
      mSenderBuffer.vals[0][0] = static_cast<double>(schedule.octave);
      for (size_t tone = 0; tone < BinsPerOctave; tone++) 
      {
          const double realD = (*cqtData)[tone].real();
          const double imagD = (*cqtData)[tone].imag();
          mSenderBuffer.vals[0][tone + 1] = std::sqrt(std::pow(realD, 2) + std::pow(imagD, 2));
      }
      //push octave data
      mCqtSender.PushData(mSenderBuffer);
  }

  // bypass audio
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s];
    }
  }
}

void CqtAnalyzer::OnReset()
{
    WDL_MutexLock lock(&mMutex);

    const double samplerate = GetSampleRate();
    const int blockSize = GetBlockSize();

    mCqt.init();
    mCqt.initFs(samplerate, blockSize);
    mCqtSampleBuffer.resize(blockSize, 0.);

    mSenderBuffer.chanOffset = 0;
    mSenderBuffer.ctrlTag = kCtrlTagCqtVis;
    mSenderBuffer.nChans = 1;
}

void CqtAnalyzer::OnParamChange(int paramIdx)
{
}
#endif
