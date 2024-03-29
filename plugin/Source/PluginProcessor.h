#pragma once

#include <JuceHeader.h>
#include "sms/Sms_Apu.h"
#include "sms/Stereo_Buffer.h"

//==============================================================================
/**
*/
class SN76489AudioProcessorEditor;
class SN76489AudioProcessor : public gin::Processor
{
public:
    //==============================================================================
    SN76489AudioProcessor();
    ~SN76489AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioSampleBuffer&, juce::MidiBuffer&) override;

    //==============================================================================
	juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    
    static const char* paramPulse1Level;
    static const char* paramPulse2Level;
    static const char* paramPulse3Level;
    static const char* paramNoiseLevel;
    static const char* paramNoiseWhite;
    static const char* paramNoiseShift;
    
    gin::AudioFifo fifo {1, 44100};

private:
    void runUntil (int& done, juce::AudioSampleBuffer& buffer, int pos);
    
    int lastNote = -1;
    int velocity = 0;
	juce::Array<int> noteQueue;
    
	juce::LinearSmoothedValue<float> outputSmoothed;
    
    Sms_Apu apu;
    Stereo_Buffer buf;
    const long clocks_per_sec = 3579545;
    
    struct ChannelInfo
    {
        int note = -1;
        int velocity = 0;
        bool dirty = false;
    };
    
    ChannelInfo channelInfo[3];
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SN76489AudioProcessor)
};
