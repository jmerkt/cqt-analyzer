#pragma once

#include "IControl.h"

template<int OctaveNumber>
class IOctaveMagnitudes : public IControl
{
public:
	IOctaveMagnitudes(const IRECT& bounds, const IVStyle& style) : IControl(bounds)
	{
		for (int octave = 0; octave < OctaveNumber; octave++)
		{
			mOctaveEnvelopeFeature[octave] = 0.;
			mOctaveEnvelopeFeatureDb[octave] = -80.;
		}
		mLabelText = style.valueText;
	};
	~IOctaveMagnitudes() = default;

	void Draw(IGraphics& g) override
	{
		//Areas for circle and label
		IRECT circleArea = mRECT;
		IRECT labelArea = mRECT;
		circleArea.ReduceFromBottom(0.08f * mRECT.H());
		labelArea.ReduceFromTop(0.92f * mRECT.H());
		//Write label
		g.DrawText(mLabelText, "Octave Magnitudes", labelArea);
		//Main empty white Circle
		float xc = circleArea.MW();
		float yc = circleArea.MH();
		float r = circleArea.H() < circleArea.W() ? circleArea.H() / 2.f : circleArea.W() / 2.f;
		g.FillCircle(COLOR_BLACK, xc, yc, r);
		//Arc for each octave, cut at 0. value like processing does
		float arcAngleIncr = 360.f / static_cast<float>(OctaveNumber);
		float angle = 0.f;
		const float colorFadeIncr = 1.f / (static_cast<float>(OctaveNumber));
		for (int octave = OctaveNumber - 1; octave >= 0; octave--)
		{
			const float rMapping = 1. / 80.f * (mOctaveEnvelopeFeatureDb[octave] + 80.f);
			float toneR = rMapping * r;
			if (toneR < 0.f) toneR = 0.f;
			g.FillArc(IColor::FromHSLA(static_cast<float>(OctaveNumber - octave - 1) * colorFadeIncr, 0.7, 0.3, 1.f), xc, yc, toneR, angle, angle + arcAngleIncr);
			angle += arcAngleIncr;
		}
		//Octave Strings
		IText text;
		text.mSize = 22.;
		text.mFGColor = COLOR_WHITE;
		auto textRect = circleArea;
		float rFactor = 0.7f;
		textRect.GetScaledAboutCentre(0.01);
		textRect.Translate(0, r * -rFactor);
		angle = 90.f;
		float rText = rFactor * r;
		float xOrig = rText * std::cos(DegToRad(angle));
		float yOrig = rText * std::sin(DegToRad(angle));
		angle -= arcAngleIncr / 2.f;
		float xNew = rText * std::cos(DegToRad(angle));
		float yNew = rText * std::sin(DegToRad(angle));
		textRect.Translate(xNew - xOrig, -(yNew - yOrig));
		g.DrawText(text, mOctaves[0].c_str(), textRect);
		for (int octave = 1; octave < OctaveNumber; octave++)
		{
			xOrig = xNew;
			yOrig = yNew;
			angle -= arcAngleIncr;
			xNew = rText * std::cos(DegToRad(angle));
			yNew = rText * std::sin(DegToRad(angle));
			textRect.Translate(xNew - xOrig, -(yNew - yOrig));
			g.DrawText(text, mOctaves[octave].c_str(), textRect);
		}
		//Separation Line
		g.DrawLine(COLOR_WHITE, xc, yc, xc, yc - r, 0, 3.f);
		g.FillCircle(COLOR_WHITE, xc, yc, 1.5f);
	}

	void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
	{
		IByteStream stream(pData, dataSize);
		int pos = 0;
		ISenderData<1, std::array<double, OctaveNumber>> d;
		pos = stream.Get(&d, pos);
		for (int octave = 0; octave < OctaveNumber; octave++)
		{
			mOctaveEnvelopeFeature[octave] = mOctaveEnvelopeFeature[octave] * 0.9 + 0.1 * d.vals[0][octave];
		}
		// Normalize feature
		double max = -1.;
		for (int octave = 0; octave < OctaveNumber; octave++)
		{
			mOctaveEnvelopeFeatureDb[octave] = AmpToDB(mOctaveEnvelopeFeature[octave]);
		}
		SetDirty(false);
	};
private:
	double mOctaveEnvelopeFeature[OctaveNumber];
	double mOctaveEnvelopeFeatureDb[OctaveNumber];
	IColor mBackgroundColor{ COLOR_BLACK };
	const std::string mOctaves[OctaveNumber] = { "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9"};
	IText mLabelText;
};