#pragma once

#include "IControl.h"

template<int ChannelNumber, int B>
class ChromaVisual : public IControl
{
public:
	ChromaVisual(const IRECT& bounds, const IVStyle& style) : IControl(bounds)
	{
		for (int ch = 0; ch < ChannelNumber; ch++)
		{
			for (int tone = 0; tone < B; tone++)
			{
				mChromaFeature[ch][tone] = 0.;
			}
		}
		mLabelText = style.valueText;
	};
	~ChromaVisual() = default;

	void Draw(IGraphics& g) override
	{
		//Areas for circle and label
		IRECT circleArea = mRECT;
		IRECT labelArea = mRECT;
		circleArea.ReduceFromBottom(0.08f * mRECT.H());
		labelArea.ReduceFromTop(0.92f * mRECT.H());
		//Write label
		g.DrawText(mLabelText, "Note Distribution", labelArea);
		//Main empty white Circle
		float xc = circleArea.MW();
		float yc = circleArea.MH();
		float r = circleArea.H() < circleArea.W() ? circleArea.H() / 2.f : circleArea.W() / 2.f;
		//g.DrawCircle(COLOR_WHITE, xc, yc, r);
		g.FillCircle(COLOR_BLACK, xc, yc, r);
		//Arc for each tone, cut at 0. value like processing does
		float arcAngleIncr = 360.f / static_cast<float>(B);
		float angle = 0.f;
		for (int ch = 0; ch < ChannelNumber; ch++)
		{
			for (int tone = 0; tone < B; tone++)
			{
				float toneR = mChromaFeature[ch][tone] * r;
				if (toneR < 0.f) toneR = 0.f;
				g.FillArc(mToneColors[ch], xc, yc, toneR, angle, angle + arcAngleIncr);
				angle += arcAngleIncr;
			}
		}
		//Töne Buchstaben
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
		//Little circle in middle
		//g.FillCircle(COLOR_WHITE, xc, yc, r * 0.01f);
	}

	void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
	{
		IByteStream stream(pData, dataSize);
		int pos = 0;
		ISenderData<ChannelNumber, std::array<double, B>> d;
		pos = stream.Get(&d, pos);
		for(int ch = 0; ch < ChannelNumber; ch++)
		{ 
			for (int tone = 0; tone < B; tone++)
			{
				mChromaFeature[ch][tone] = mChromaFeature[ch][tone] * 0.9 + 0.1 * d.vals[ch][tone];
			}
		}
		SetDirty(false);
	};
private:
	// TODO: COLORS MATCHING (SIMILAR) FOR F0 and its quint / quart! - Take the colors of circle of fifth inspiration maybe and a bit tranparency
	double mChromaFeature[ChannelNumber][B];
	IColor mBackgroundColor{ COLOR_BLACK };
	const IColor mToneColors[ChannelNumber] = { {204,0,90,190}, {125,255,230,0} };
	const std::string mNotes[B] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

	IText mLabelText;
};