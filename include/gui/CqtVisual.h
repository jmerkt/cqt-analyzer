#pragma once

#include "IControl.h"


class CqtBar : public IControl
	{
	public:
		CqtBar(const IRECT& bounds)
			: IControl(bounds)
			, mColor(COLOR_GREEN) {
		}

		void Draw(IGraphics& g) override {
			g.FillRect(COLOR_WHITE, mRECT);
			//Draw value bar
			IRECT valueRect = mRECT;
			valueRect.ReduceFromTop(valueRect.H() - mValue * valueRect.H());
			g.FillRect(mColor, valueRect);
			//Frame
			g.DrawRect(COLOR_BLACK, mRECT);
		}
		void changeValue(double value) {
			if (value > 1.) {
				value = 1.;
			}
			if (value < 0.) {
				value = 0.;
			}
			mValue = value * 0.25 + mValue * 0.75;
		};
		double getValue() {
			return mValue;
		}

		void setColor(IColor color) {
			mColor = color;
		};

	private:
		IColor mColor;
		double mValue = 0.5;
	};

/*
init Parameters has to get calles after creation and redraw then
*/
class CqtVisual : public IControl
	{
	public:
		CqtVisual(const IRECT& bounds)
			: IControl(bounds)
			, mBackgroundColor(COLOR_BLACK)
		{
			initParameters();
		}

		~CqtVisual() {
			for (auto bar : mToneBars) {
				delete bar;
			}
		}

		void Draw(IGraphics& g) override {
			//g.FillRect(mBackgroundColor, mRECT);
			for (auto toneBar : mToneBars) {
				toneBar->Draw(g);
			}
		}
		void initParameters(size_t octaveNumber = OctaveNumber, size_t B = BinsPerOctave, double f1 = 440., double fs = 48000.) {
			mOctaveNumber = octaveNumber;
			mB = BinsPerOctave;
			mF1 = f1;
			mFs = fs;
			//redraw all stuff
			mColorFadeIncr = (int)((double)255) / ((double)mB);
			mToneBars.clear();
			IRECT barRect = mRECT;
			float barWidth = barRect.W() / (mOctaveNumber * mB);
			barRect.ReduceFromRight(barRect.W() - barWidth);
			//allocate the bar's space
			int count = 0;
			for (int octave = 0; octave < mOctaveNumber; octave++) {
				for (int tone = 0; tone < mB; tone++) {
					mToneBars.push_back(new CqtBar(barRect));
					barRect.Translate(barRect.W(), 0.f);
					//Color management
					IColor barColor{255, 0, 255 - mColorFadeIncr * tone, 0 + mColorFadeIncr * tone};
					mToneBars[count]->setColor(barColor);
					count++;
				}
			}
		}

		void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override {
			IByteStream stream(pData, dataSize);
			int pos = 0;
			ISenderData<1, std::array<double, OctaveBufferSize>> d;
			pos = stream.Get(&d, pos);
			int octave = (int)d.vals[0][0];
			for (size_t tone = 0; tone < mB; tone++) 
			{
				mToneBars[(mOctaveNumber - octave - 1) * mB + tone]->changeValue(d.vals[0][tone + 1]);
			}
			SetDirty(false);
		};

	private:
		IColor mBackgroundColor;
		std::vector<CqtBar*> mToneBars;
		size_t mOctaveNumber = 9;
		size_t mB = 12;
		double mF1 = 41.2034;
		double mFs = 48000.;
		int mColorFadeIncr = 0;
	};
