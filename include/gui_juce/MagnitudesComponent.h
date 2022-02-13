#pragma once

class MagnitudesComponent    : public juce::Component
{
public:
    MagnitudesComponent()
    {
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::lightblue);
    }

    void resized() override
    {
    }

private:


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagnitudesComponent)
};