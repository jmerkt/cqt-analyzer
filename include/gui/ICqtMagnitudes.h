#pragma once

#include "IControl.h"
#include <sstream>
#include <iomanip>


class IMagnitudeMeter : public IControl
{
public:
	IMagnitudeMeter(const IRECT& bounds, const IColor& color): IControl(bounds), mColor(color)
	{
	}

	void Draw(IGraphics& g) override 
	{
		// draw value bar
		if (mValue > 1.e-3)
		{
			IRECT valueRect = mRECT;
			valueRect.ReduceFromTop(valueRect.H() - mValue * valueRect.H());
			g.FillRect(mColor, valueRect);
		}
	}
	void setValue(double value) 
	{
		mValue = value * 0.3 + mValue * 0.7;
	};
	double getValue() 
	{
		return mValue;
	}

private:
	IColor mColor;
	double mValue{ 0. };
};


template <int B, int OctaveNumber>
class ICqtMagnitudes : public IControl
{
public:
	ICqtMagnitudes(const IRECT& bounds, const IVStyle& style)
		: IControl(bounds)
		, mBackgroundColor(COLOR_BLACK)
	{
		mColorFadeIncr = 1.f / (static_cast<float>(B));
		mMagnitudeMeters.clear();
		IRECT meterRect = mRECT;
		meterRect.ReduceFromLeft(mYAxisMargin * meterRect.W());
		const float barWidth = meterRect.W() / (OctaveNumber * B);
		meterRect.ReduceFromRight(meterRect.W() - barWidth);
		meterRect.ReduceFromBottom(mXAxisMargin * meterRect.H());
		// create level meters
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
				IColor meterColor = IColor::FromHSLA(static_cast<float>(tone) * mColorFadeIncr, 0.7, 0.3, 1.f);
				mMagnitudeMeters.push_back(std::make_unique<IMagnitudeMeter>(meterRect, meterColor));
				meterRect.Translate(meterRect.W(), 0.f);
			}
		}
	}

	~ICqtMagnitudes() = default;

	void Draw(IGraphics& g) override 
	{
		g.FillRect(mBackgroundColor, mRECT);
		// x-axis labels
		IText text;
		text.mSize = 12.;
		text.mFGColor = COLOR_WHITE;
		const float octaveNumFloat = static_cast<float>(OctaveNumber);
		auto labelRect = mRECT.GetReducedFromTop((1.f - mXAxisMargin) * mRECT.H());
		labelRect.ReduceFromLeft(mYAxisMargin * labelRect.W());
		labelRect.ReduceFromRight((octaveNumFloat - 1.f) / octaveNumFloat * labelRect.W());
		const float colorFadeIncr = 1.f / (static_cast<float>(OctaveNumber));
		for (int o = 0; o < OctaveNumber; o++)
		{
			const double fRef = std::pow(2., ((100. - 49.) / 12.)) * mTuning;
			const double freq = (fRef / std::pow(2., (OctaveNumber - o))) * std::pow(2., static_cast<double>(B + 9) / static_cast<double>(B));
			std::string freqStr = "A" + std::to_string(o + 1) + ": ";
			if (freq < 1000.)
			{
				freqStr += std::to_string(static_cast<int>(std::round(freq))) + " Hz";
			}
			else
			{
				std::ostringstream streamObj;
				streamObj << std::fixed;
				streamObj << std::setprecision(2);
				streamObj << freq / 1000.;
				freqStr += streamObj.str() + " kHz";
			}
			g.DrawText(text, freqStr.c_str(), labelRect);

			g.DrawRect(IColor::FromHSLA(static_cast<float>(o) * colorFadeIncr, 0.7, 0.3, 1.f), labelRect.GetScaledAboutCentre(0.925f), 0, 6.f);
			g.DrawDottedLine(COLOR_WHITE, labelRect.R, mRECT.T, labelRect.R, mRECT.B);

			labelRect.Translate(labelRect.W(), 0.f);
		}

			

		// TODO: still an offset for numbers != % 5 == 0 

		// y-axis labels
		const double range = mMagMax - mMagMin;
		const float numLines = std::floor(range / mYAxisLabelSpacing);
		float yPos = mRECT.T;
		for (int i = 0; i < numLines; i++)
		{
			g.DrawLine(COLOR_WHITE, mYAxisMargin * mRECT.W(), yPos, mRECT.W(), yPos);
			yPos += (1.f - mXAxisMargin) * mRECT.H() / numLines;
		}
		const float numLabels = std::floor(range / mYAxisLabelSpacing);
		labelRect = mRECT.GetReducedFromRight((1.f - mYAxisMargin) * mRECT.W()).GetReducedFromBottom(mXAxisMargin * mRECT.H());
		labelRect.ReduceFromBottom((numLabels - 2.f) / numLabels * labelRect.H());
		for (int i = 0; i < numLabels; i++)
		{
			std::string label = std::to_string(static_cast<int>(mMagMax) - (i + 1) * static_cast<int>(mYAxisLabelSpacing)) + " dB";
			g.DrawText(text, label.c_str(), labelRect);
			labelRect.Translate(0.f, labelRect.H() / 2.f);
		}

		// meters
		for (const auto& meter : mMagnitudeMeters) 
		{
			meter.get()->Draw(g);
		}
	}

	void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override 
	{
		IByteStream stream(pData, dataSize);
		int pos = 0;
		ISenderData<1, std::array<double, OctaveBufferSize>> d;
		pos = stream.Get(&d, pos);
		const int octave = static_cast<int>(d.vals[0][0]);
		mMagMin = d.vals[0][1];
		mMagMax = d.vals[0][2];
		mTuning = d.vals[0][3];
		for (size_t tone = 0; tone < B; tone++) 
		{
			double magLog = AmpToDB(d.vals[0][tone + 4]);
			magLog = Clip<double>(magLog, mMagMin, mMagMax);
			const double magLogMapped = 1. - ((mMagMax - magLog) / (mMagMax - mMagMin));
			mMagnitudeMeters[(OctaveNumber - octave - 1) * B + tone].get()->setValue(magLogMapped);
		}
		SetDirty(false);
	};

private:
	IColor mBackgroundColor;
	std::vector<std::unique_ptr<IMagnitudeMeter>> mMagnitudeMeters;
	float mColorFadeIncr{ 0.f };
	double mMagMin{ -120. };
	double mMagMax{ 0. };
	double mTuning{ 440. };
	const float mXAxisMargin{ 0.1f };
	const float mYAxisMargin{ 0.06f };
	const float mYAxisLabelSpacing{ 5.f };
};
