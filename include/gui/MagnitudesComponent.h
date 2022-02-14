#pragma once



class MagnitudeMeter : public juce::Component
{
public:
    MagnitudeMeter() = default;

    void paint (juce::Graphics& g) override
    {
        if (mValue > 1.e-3)
		{
			auto valueRect = getBounds();
			valueRect = valueRect.withTrimmedTop(valueRect.getHeight() - mValue * valueRect.getHeight());
            g.setColour(mColour);
			g.fillRect(valueRect);
		}
    };

    void setColour(const juce::Colour colour){mColour = colour;};

    void setValue(const double value) 
	{
		if(value > mValue)
		{
			mValue = 0.3 * value + 0.7 * mValue;
		}
		else
		{
			mValue = 0.15 * value + 0.85 * mValue;
		}	
	};

private:
    double mValue{ 0. };
    juce::Colour mColour;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagnitudeMeter)
};

template <int B, int OctaveNumber>
class MagnitudesComponent    : public juce::Component, public juce::Timer
{
public:
    MagnitudesComponent(AudioPluginAudioProcessor& p):
	processorRef (p)
    {
		// create level meters
		const float colorFadeIncr = 1.f / (static_cast<float>(OctaveNumber * B));
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
                addAndMakeVisible(mMagnitudeMeters[octave][tone]);
                mMagnitudeMeters[octave][tone].setColour(juce::Colour{static_cast<float>(octave * B + tone) * colorFadeIncr, 0.9, 0.5, 1.f});
			}
		}

		// timer
		startTimer(15);
    }

	~MagnitudesComponent()
	{
		//stopTimer();
	}

    void paint (juce::Graphics& g) override
    {
        g.fillAll (mBackgroundColor);

        auto bounds = getBounds();

        g.setFont(12.f / 550.f * static_cast<float>(bounds.getHeight()));

		// x-axis labels
		const float octaveNumFloat = static_cast<float>(OctaveNumber);
		auto labelRect = bounds.withTrimmedTop((1.f - mXAxisMargin) * bounds.getHeight());
		labelRect = labelRect.withTrimmedLeft(mYAxisMargin * labelRect.getWidth());
		labelRect = labelRect.withTrimmedRight((octaveNumFloat - 1.f) / octaveNumFloat * labelRect.getWidth());
		const float colorFadeIncr = 1.f / (static_cast<float>(OctaveNumber));
		for (int o = 0; o < OctaveNumber; o++)
		{
			const double fRef = std::pow(2., ((100. - 49.) / 12.)) * mTuning;
            const int toneOffset = static_cast<int>(std::round(9.f / 12.f * static_cast<float>(B)));
			const double freq = (fRef / std::pow(2., (OctaveNumber - o))) * std::pow(2., static_cast<double>(B + toneOffset) / static_cast<double>(B));
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
            g.setColour(juce::Colours::white);
			g.drawText(juce::String(freqStr), labelRect, juce::Justification::centred);

            g.setColour(juce::Colour{static_cast<float>(o) * colorFadeIncr, 0.9, 0.5, 1.f});
			g.drawRect(labelRect.withSizeKeepingCentre(labelRect.getWidth() * 0.925f, labelRect.getHeight() * 0.925f), 6.f);
            g.setColour(juce::Colours::white);
            float dashPattern[2];
            dashPattern[0] = 8.0;
            dashPattern[1] = 8.0;
			g.drawDashedLine({labelRect.getRight(), bounds.getY(), labelRect.getRight(), bounds.getBottom()}, dashPattern, 2, 1.f);

			labelRect.translate(labelRect.getWidth(), 0.f);
		}

		// y-axis line and labels
		const float labelHeight = bounds.getHeight() - mXAxisMargin * bounds.getHeight();
		const double range = mMagMax - mMagMin;
		const int numLines = static_cast<int>(std::ceil(range / mYAxisLabelSpacing));
		float yValLine = mMagMax;
		for (int i = 0; i < numLines; i++)
		{
			const float yPos = bounds.getY() + ((mMagMax - yValLine) / (mMagMax - mMagMin)) * labelHeight;
            g.setColour(juce::Colours::white);
			g.drawLine({mYAxisMargin * bounds.getWidth(), yPos, bounds.getWidth(), yPos}, 1.f);
			yValLine -= mYAxisLabelSpacing;
		}

		const int numLabels = static_cast<int>(std::floor(range / mYAxisLabelSpacing));
		labelRect = bounds.withTrimmedRight((1.f - mYAxisMargin) * bounds.getWidth());
		float yValLabel = mMagMax - mYAxisLabelSpacing;
		for (int i = 0; i < numLabels; i++)
		{
			labelRect.setTop(bounds.getY() + ((mMagMax - (yValLabel + mYAxisLabelSpacing)) / (mMagMax - mMagMin)) * labelHeight);
			labelRect.setBottom(bounds.getY() + ((mMagMax - (yValLabel - mYAxisLabelSpacing)) / (mMagMax - mMagMin)) * labelHeight);
			std::string label = std::to_string(static_cast<int>(mMagMax) - (i + 1) * static_cast<int>(mYAxisLabelSpacing)) + " dB";
            g.setColour(juce::Colours::white);
			g.drawText(juce::String(label), labelRect, juce::Justification::centred);
			yValLabel -= mYAxisLabelSpacing;
		}

		// meters
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
                mMagnitudeMeters[octave][tone].paint(g);
			}
		}
    }

    void resized() override
    {
		auto meterRect = getBounds();
		meterRect = meterRect.withTrimmedLeft(mYAxisMargin * meterRect.getWidth());
		const float barWidth = meterRect.getWidth() / static_cast<float>(OctaveNumber * B);
		meterRect = meterRect.withTrimmedRight(meterRect.getWidth() - barWidth);
		meterRect = meterRect.withTrimmedBottom(mXAxisMargin * meterRect.getHeight());
		// create level meters
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
                mMagnitudeMeters[octave][tone].setBounds(meterRect);
				meterRect.translate(meterRect.getWidth(), 0.f);
			}
		}
    }

	void timerCallback() override
	{
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
				const double value = processorRef.mCqtDataStorage[octave][tone];
				double magLog = juce::Decibels::gainToDecibels(value);
				magLog = Cqt::Clip<double>(magLog, mMagMin, mMagMax);
				const double magLogMapped = 1. - ((mMagMax - magLog) / (mMagMax - mMagMin));
				mMagnitudeMeters[OctaveNumber - octave - 1][tone].setValue(magLogMapped);
			}
		}
		repaint();
	}

	void setRangeMin(const double rangeMin)
	{
		if(static_cast<int>(rangeMin) != static_cast<int>(mMagMax))
		{
			mMagMin = rangeMin;
			repaint();
		}	
	}

	void setRangeMax(const double rangeMax)
	{
		if(static_cast<int>(rangeMax) != static_cast<int>(mMagMin))
		{
			mMagMax = rangeMax;
			repaint();
		}
	}

	void setTuning(const double tuning)
	{
		mTuning = tuning;
		repaint();
	}
private:
	AudioPluginAudioProcessor& processorRef;

    juce::Colour mBackgroundColor{juce::Colours::black};
    juce::Colour mMeterColour{juce::Colours::blue};
	MagnitudeMeter mMagnitudeMeters[OctaveNumber][B];
	double mMagMin{ -50. };
	double mMagMax{ 0. };
	double mTuning{ 440. };
	const float mXAxisMargin{ 0.1f };
	const float mYAxisMargin{ 0.06f };
	const float mYAxisLabelSpacing{ 5.f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagnitudesComponent)
};