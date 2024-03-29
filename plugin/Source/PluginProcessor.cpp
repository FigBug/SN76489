#include "PluginProcessor.h"
#include "PluginEditor.h"

const char* SN76489AudioProcessor::paramPulse1Level      = "pulse1Level";
const char* SN76489AudioProcessor::paramPulse2Level      = "pulse2Level";
const char* SN76489AudioProcessor::paramPulse3Level      = "pulse3Level";
const char* SN76489AudioProcessor::paramNoiseLevel       = "noiseLevel";
const char* SN76489AudioProcessor::paramNoiseWhite       = "noiseWhite";
const char* SN76489AudioProcessor::paramNoiseShift       = "noiseShift";

//==============================================================================
static juce::String percentTextFunction (const gin::Parameter& p, float v)
{
    return juce::String::formatted ("%.0f%%", v / p.getUserRangeEnd() * 100);
}

static juce::String typeTextFunction (const gin::Parameter&, float v)
{
    return v > 0.0f ? "White" : "Periodic";
}

static juce::String speedTextFunction (const gin::Parameter&, float v)
{
    switch (int (v))
    {
        case 0: return "Fast";
        case 1: return "Medium";
        case 2: return "Slow";
        case 3: return "Tone 2";
    }
    return "";
}

//==============================================================================
SN76489AudioProcessor::SN76489AudioProcessor()
    : gin::Processor (false, gin::ProcessorOptions().withAdditionalCredits({"Shay Green"}))
{
    addExtParam (paramPulse1Level, "Pulse 1 Level", "Pulse 1", "", { 0.0f, 1.0f,  0.0f, 1.0f }, 1.0f, 0.0f, percentTextFunction);
    addExtParam (paramPulse2Level, "Pulse 2 Level", "Pulse 2", "", { 0.0f, 1.0f,  0.0f, 1.0f }, 0.0f, 0.0f, percentTextFunction);
    addExtParam (paramPulse3Level, "Pulse 3 Level", "Pulse 3", "", { 0.0f, 1.0f,  0.0f, 1.0f }, 0.0f, 0.0f, percentTextFunction);
    addExtParam (paramNoiseLevel,  "Noise Level",   "Noise",   "", { 0.0f, 1.0f,  0.0f, 1.0f }, 0.0f, 0.0f, percentTextFunction);
    addExtParam (paramNoiseWhite,  "Noise Type",    "Type",    "", { 0.0f, 1.0f,  1.0f, 1.0f }, 0.0f, 0.0f, typeTextFunction);
    addExtParam (paramNoiseShift,  "Noise Speed",   "Speed",   "", { 0.0f, 3.0f,  1.0f, 1.0f }, 0.0f, 0.0f, speedTextFunction);
    
    init();
}

SN76489AudioProcessor::~SN76489AudioProcessor()
{
}

//==============================================================================
void SN76489AudioProcessor::prepareToPlay (double sampleRate, int)
{
    outputSmoothed.reset (sampleRate, 0.05);
    
    buf.sample_rate (long (sampleRate));
    buf.clock_rate (clocks_per_sec);
    
    apu.output (buf.center(), buf.left(), buf.right());
}

void SN76489AudioProcessor::releaseResources()
{
}

void SN76489AudioProcessor::runUntil (int& done, juce::AudioSampleBuffer& buffer, int pos)
{
    int todo = std::min (pos, buffer.getNumSamples()) - done;
    
    while (todo > 0)
    {
        if (buf.samples_avail() > 0)
        {
            blip_sample_t out[1024];
            
			int count = int (buf.read_samples (out, (size_t) std::min ({todo, 1024 / 2, (int) buf.samples_avail()})));
        
            auto data = buffer.getWritePointer (0, done);
            for (int i = 0; i < count; i++)
                data[i] = (out[i * 2] + out[i * 2 + 1]) / 2.0f / 32768.0f;
        
            done += count;
            todo -= count;
        }
        else
        {
            apu.end_frame (1024);
            buf.end_frame (1024);
        }
    }
}

void SN76489AudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midi)
{
	buffer.clear();
	
    const float p1Level = getParameter (paramPulse1Level)->getUserValue();
    const float p2Level = getParameter (paramPulse2Level)->getUserValue();
    const float p3Level = getParameter (paramPulse3Level)->getUserValue();
    const float nLevel  = getParameter (paramNoiseLevel)->getUserValue();
    const bool nWhite   = getParameter (paramNoiseWhite)->getUserValue() > 0.0f;
    const int nType     = getParameter (paramNoiseShift)->getUserValueInt();

    int done = 0;
    runUntil (done, buffer, 0);
    
    for (auto itr : midi)
    {
        auto msg = itr.getMessage();
        int pos = itr.samplePosition;

        runUntil (done, buffer, pos);
        
        if (msg.isNoteOn())
        {
            noteQueue.add (msg.getNoteNumber());
            velocity = msg.getVelocity();
            
            for (int i = 0; i < 3; i++)
            {
                if (channelInfo[i].note == -1)
                {
                    channelInfo[i].note = msg.getNoteNumber();
                    channelInfo[i].velocity = msg.getVelocity();
                    channelInfo[i].dirty = true;
                    break;
                }
            }
        }
        else if (msg.isNoteOff())
        {
            noteQueue.removeFirstMatchingValue (msg.getNoteNumber());
            
            for (int i = 0; i < 3; i++)
            {
                if (channelInfo[i].note == msg.getNoteNumber())
                {
                    channelInfo[i].note = -1;
                    channelInfo[i].velocity = 0;
                    channelInfo[i].dirty = true;
                }
            }
        }
        else if (msg.isAllNotesOff())
        {
            noteQueue.clear();
            
            for (int i = 0; i < 3; i++)
            {
                if (channelInfo[i].note != -1)
                {
                    channelInfo[i].note = -1;
                    channelInfo[i].velocity = 0;
                    channelInfo[i].dirty = true;
                }
            }
        }
                
        blip_time_t time = 0;
        
         if (channelInfo[0].dirty)
         {
             int v = channelInfo[0].velocity;
             int curNote = channelInfo[0].note;
             
            // Tone 1
            apu.write_data (time, 0x80 | (0 << 5) | (1 << 4) | 0xF - int (p1Level * v / 127.0 * 0xF));
            
            if (curNote != -1)
            {
                int period = int (3579545.0 / (juce::MidiMessage::getMidiNoteInHertz (curNote) * 2 * 16));
                period = juce::jlimit (1, 0x3ff, period);
                
                apu.write_data (time, 0x80 | (0 << 5) | (0 << 4) | (period & 0xF));
                apu.write_data (time, period >> 4);
            }
         }

        if (channelInfo[1].dirty)
        {
            int v = channelInfo[1].velocity;
            int curNote = channelInfo[1].note;

            // Tone 2
            apu.write_data (time, 0x80 | (1 << 5) | (1 << 4) | 0xF - int (p2Level * v / 127.0 * 0xF));
            
            if (curNote != -1)
            {
                int period = int (3579545.0 / (juce::MidiMessage::getMidiNoteInHertz (curNote) * 2 * 16));
                period = juce::jlimit (1, 0x3ff, period);
                
                apu.write_data (time, 0x80 | (1 << 5) | (0 << 4) | (period & 0xF));
                apu.write_data (time, period >> 4);
            }
        }

        if (channelInfo[2].dirty)
        {
            int v = channelInfo[2].velocity;
            int curNote = channelInfo[2].note;

            // Tone 3
            apu.write_data (time, 0x80 | (2 << 5) | (1 << 4) | 0xF - int (p3Level * v / 127.0 * 0xF));
            
            if (curNote != -1)
            {
                int period = int (3579545.0 / (juce::MidiMessage::getMidiNoteInHertz (curNote) * 2 * 16));
                period = juce::jlimit (1, 0x3ff, period);
                
                apu.write_data (time, 0x80 | (2 << 5) | (0 << 4) | (period & 0xF));
                apu.write_data (time, period >> 4);
            }
        }
        
        const int curNote = noteQueue.size() > 0 ? noteQueue.getFirst() : -1;

        if (curNote != lastNote)
        {
            int v = curNote == -1 ? 0 : velocity;
            
            // Noise
            apu.write_data (time, 0x80 | (3 << 5) | (1 << 4) | 0xF - int (nLevel * v / 127.0 * 0xF));
            
            if (curNote != -1)
            {
                apu.write_data (time, 0x80 | (3 << 5) | (0 << 4) | ((nWhite ? 1 : 0) << 2) | nType);
            }
            
            lastNote = curNote;
        }
    }
    
    int numSamples = buffer.getNumSamples();
    runUntil (done, buffer, numSamples);
    
    if (fifo.getFreeSpace() >= numSamples)
    {
        auto data = buffer.getReadPointer (0);
        fifo.writeMono (data, numSamples);
    }
}

//==============================================================================
bool SN76489AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SN76489AudioProcessor::createEditor()
{
    return new SN76489AudioProcessorEditor (*this);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SN76489AudioProcessor();
}
