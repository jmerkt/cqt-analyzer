#pragma once

#include "IControl.h"

template<int B>
class IChromaFeature : public IControl
{
public:
	IChromaFeature(const IRECT& bounds, const IVStyle& style) : IControl(bounds)
	{
		for (int tone = 0; tone < B; tone++)
		{
			mChromaFeature[tone] = 0.;
		}
		mLabelText = style.valueText;
	};
	~IChromaFeature() = default;

	void Draw(IGraphics& g) override
	{
		// areas for circle and label
		IRECT circleArea = mRECT;
		IRECT labelArea = mRECT;
		circleArea.ReduceFromBottom(0.08f * mRECT.H());
		labelArea.ReduceFromTop(0.92f * mRECT.H());
		// label
		g.DrawText(mLabelText, "Note Distribution", labelArea);
		// main empty black circle
		float xc = circleArea.MW();
		float yc = circleArea.MH();
		float r = circleArea.H() < circleArea.W() ? circleArea.H() / 2.f : circleArea.W() / 2.f;
		g.FillCircle(COLOR_BLACK, xc, yc, r);
		// arc for each tone, cut value at 0. 
		float arcAngleIncr = 360.f / static_cast<float>(B);
		float angle = 0.f;
		const float colorFadeIncr = 1.f / (static_cast<float>(B));
		for (int tone = 0; tone < B; tone++)
		{
			float toneR = mChromaFeature[tone] * r;
			if (toneR < 0.f) toneR = 0.f;
			g.FillArc(IColor::FromHSLA(static_cast<float>(tone) * colorFadeIncr, 0.7, 0.3, 1.f), xc, yc, toneR, angle, angle + arcAngleIncr);
			angle += arcAngleIncr;
		}
		// note letters
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
		g.DrawText(text, mNotes[0].c_str(), textRect);
		for (int tone = 1; tone < B; tone++)
		{
			xOrig = xNew;
			yOrig = yNew;
			angle -= arcAngleIncr;
			xNew = rText * std::cos(DegToRad(angle));
			yNew = rText * std::sin(DegToRad(angle));
			textRect.Translate(xNew - xOrig, -(yNew - yOrig));
			g.DrawText(text, mNotes[tone].c_str(), textRect);
		}
	}

	void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
	{
		IByteStream stream(pData, dataSize);
		int pos = 0;
		ISenderData<1, std::array<double, B>> d;
		pos = stream.Get(&d, pos);
		for (int tone = 0; tone < B; tone++)
		{
			mChromaFeature[tone] = mChromaFeature[tone] * 0.9 + 0.1 * d.vals[0][tone];
		}
		SetDirty(false);
	};
private:
	double mChromaFeature[B];
	IColor mBackgroundColor{ COLOR_BLACK };
	const std::string mNotes[B] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	IText mLabelText;
};