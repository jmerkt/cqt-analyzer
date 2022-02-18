#pragma once



class MagnitudeMeter : public juce::Component, public juce::TooltipClient
{
public:
    MagnitudeMeter() = default;

    void paint (juce::Graphics& g) override
    {
		auto valueRect = getBounds().toFloat();
		valueRect = valueRect.withTrimmedTop(valueRect.getHeight() - mValue * valueRect.getHeight());
		g.setColour(mColour);
		g.fillRect(valueRect);
    };

    void setColour(const juce::Colour colour){mColour = colour;};

    void setValue(const double value) 
	{
		if(value > mValue)
		{
			mValue = (1. - mSmoothingUp) * value + mSmoothingUp * mValue;
		}
		else
		{
			mValue = (1. - mSmoothingDown) * value + mSmoothingDown * mValue;
		}	
	};

	void setValueHard(const double value)
	{
		mValue = value;
	}

	const double& getValue()
	{
		return mValue;
	}

	void setSmoothing(const double smoothingUp, const double smoothingDown)
	{
		const double smoothingUpClipped = Cqt::Clip<double>(smoothingUp, 0., 0.999999);
		const double smoothingDownClipped = Cqt::Clip<double>(smoothingDown, 0., 0.999999);
		mSmoothingUp = smoothingUpClipped;
		mSmoothingDown = smoothingDownClipped;
	}

	juce::String getTooltip() override
	{
		return mFrequencyString;
	}

	void setFrequency(const double frequency)
	{
		mFrequency = frequency;
		if (frequency < 1000.)
		{
			mFrequencyString = std::to_string(static_cast<int>(std::round(frequency))) + " Hz";
		}
		else
		{
			std::ostringstream streamObj;
			streamObj << std::fixed;
			streamObj << std::setprecision(2);
			streamObj << frequency / 1000.;
			mFrequencyString = streamObj.str() + " kHz";
		}
	}

private:
    double mValue{ 0. };
	double mSmoothingUp{ 0.7 };
	double mSmoothingDown{ 0.85 };
	double mFrequency{ 50. };
	juce::String mFrequencyString{"50 Hz"};
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
                mMagnitudeMeters[octave][tone].setColour(juce::Colour{static_cast<float>(octave * B + tone) * colorFadeIncr, 0.98, 0.725, 1.f});
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

        auto bounds = getLocalBounds().toFloat();

        g.setFont(14.f / static_cast<float>(PLUGIN_HEIGHT) * bounds.getHeight());

		// x-axis labels
		const float octaveNumFloat = static_cast<float>(OctaveNumber);
		auto labelRect = bounds.withTrimmedTop((1.f - mXAxisMargin) * bounds.getHeight());
		labelRect = labelRect.withTrimmedLeft(mYAxisMargin * labelRect.getWidth());
		labelRect = labelRect.withTrimmedRight((octaveNumFloat - 1.f) / octaveNumFloat * labelRect.getWidth());
		labelRect = labelRect.withTrimmedTop(2.f / static_cast<float>(PLUGIN_HEIGHT) * bounds.getHeight());

		const float colorFadeIncr = 1.f / (static_cast<float>(OctaveNumber * B));
		for (int o = 0; o < OctaveNumber; o++)
		{
            const int toneOffset = static_cast<int>(std::round(9.f / 12.f * static_cast<float>(B)));
			const double freq = processorRef.mKernelFreqs[OctaveNumber - o - 1][toneOffset];
			
			std::string freqStr = "A" + std::to_string(o) + ": ";
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

            g.setColour(juce::Colour::fromHSV(static_cast<float>(o * B + toneOffset) * colorFadeIncr, 0.98, 0.725, 1.f));
			g.drawRect(labelRect.withSizeKeepingCentre(labelRect.getWidth() - 3.f, labelRect.getHeight()), 6.f);
            g.setColour(juce::Colours::white);
            float dashPattern[2];
            dashPattern[0] = 4.0;
            dashPattern[1] = 4.0;
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
		auto meterRect = getLocalBounds().toFloat();
		meterRect = meterRect.withTrimmedLeft(mYAxisMargin * meterRect.getWidth());
		const float barWidth = meterRect.getWidth() / static_cast<float>(OctaveNumber * B);
		meterRect = meterRect.withTrimmedRight(meterRect.getWidth() - barWidth);
		meterRect = meterRect.withTrimmedBottom(mXAxisMargin * meterRect.getHeight());
		// create level meters
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
                mMagnitudeMeters[octave][tone].setBounds(meterRect.toNearestIntEdges());
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
				const double magLogMapped = 1. - ((mMagMax - magLog) * mOneDivMaxMin);
				mMagnitudeMeters[OctaveNumber - octave - 1][tone].setValue(magLogMapped);
			}
		}
		if(processorRef.mNewKernelFreqs)
		{
			for (int octave = 0; octave < OctaveNumber; octave++) 
			{
				for (int tone = 0; tone < B; tone++) 
				{
					mMagnitudeMeters[OctaveNumber - octave - 1][tone].setFrequency(processorRef.mKernelFreqs[octave][tone]);
				}
			}
			processorRef.mNewKernelFreqs = false;
		}
		repaint();
	}

	void remapValues()
	{
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
				const double magLogMapped = mMagnitudeMeters[OctaveNumber - octave - 1][tone].getValue();
				const double magLog = magLogMapped * (mMagMaxPrev - mMagMinPrev) + mMagMinPrev;
				if((magLog > mMagMin) && (magLog < mMagMax) && (magLog > mMagMinPrev))
				{
					const double magLogRemapped = 1. - ((mMagMax - magLog) * mOneDivMaxMin);
					mMagnitudeMeters[OctaveNumber - octave - 1][tone].setValueHard(magLogRemapped);
				}
				else if(magLog > mMagMax)
				{
					mMagnitudeMeters[OctaveNumber - octave - 1][tone].setValueHard(1.);
				}
				else
				{
					mMagnitudeMeters[OctaveNumber - octave - 1][tone].setValueHard(0.);
				}	
			}
		}
	}

	void setRangeMin(const double rangeMin)
	{
		if(static_cast<int>(rangeMin) != static_cast<int>(mMagMax))
		{
			mMagMinPrev = mMagMin;
			mMagMin = rangeMin;
			mOneDivMaxMin = 1. / (mMagMax - mMagMin);
			remapValues();
			repaint();
		}	
	}

	void setRangeMax(const double rangeMax)
	{
		if(static_cast<int>(rangeMax) != static_cast<int>(mMagMin))
		{
			mMagMaxPrev = mMagMax;
			mMagMax = rangeMax;
			mOneDivMaxMin = 1. / (mMagMax - mMagMin);
			remapValues();
			repaint();
		}
	}

	void setTuning(const double tuning)
	{
		mTuning = tuning;
		repaint();
	}

	void setSmoothing(const double smoothing)
	{
		const double smoothingClipped = Cqt::Clip<double>(smoothing, 0., 0.999999);
		const double smoothingUp = smoothingClipped;
		const double smoothingDown = (1. - (1. - smoothingClipped) * 0.5);
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
				mMagnitudeMeters[octave][tone].setSmoothing(smoothingUp, smoothingDown);
			}
		}
	}

	void setSmoothing(const double smoothingUp, const double smoothingDown)
	{
		for (int octave = 0; octave < OctaveNumber; octave++) 
			{
				for (int tone = 0; tone < B; tone++) 
				{
					mMagnitudeMeters[octave][tone].setSmoothing(smoothingUp, smoothingDown);
				}
			}
	}
private:
	AudioPluginAudioProcessor& processorRef;

    juce::Colour mBackgroundColor{juce::Colours::black};
    juce::Colour mMeterColour{juce::Colours::blue};
	MagnitudeMeter mMagnitudeMeters[OctaveNumber][B];
	double mMagMin{ -50. };
	double mMagMax{ 0. };
	double mMagMinPrev{ -50. };
	double mMagMaxPrev{ 0. };
	double mTuning{ 440. };
	double mOneDivMaxMin{ 1. };
	const float mXAxisMargin{ 0.08f };
	const float mYAxisMargin{ 0.06f };
	const float mYAxisLabelSpacing{ 5.f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagnitudesComponent)
};