/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <chrono>


using SmoothedValue = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

using DelayBuffer = juce::AudioBuffer<float>;

struct DelayLineEffect
{
    DelayBuffer delayBuffer;
    SmoothedValue smoothedDelay;
    SmoothedValue smoothedWidth;

    int bufferChannels;
    int bufferSize;
    int writePosition;

    float* delayData;
    float currentDelayTime;
    float currentWidth;

    int localWritePosition;
    float readPosition;
    int localReadPosition;

    float out;
    float phase;
    float phaseOffset = 0.0f;;
    float lfoPhase;
    float inverseSampleRate;
    const float twoPi = juce::MathConstants<float>::twoPi;
    float weight;

    void prepare(int sampleRate, int totalNumInputChannels, float maxDelayTime)
    {
        smoothedDelay.reset(sampleRate, 1e-3);
        smoothedWidth.reset(sampleRate, 1e-3);

        bufferSize = (int)(maxDelayTime * sampleRate) + 1;
        if (bufferSize < 1)
            bufferSize = 1;
        bufferChannels = totalNumInputChannels;
        delayBuffer.setSize(bufferChannels, bufferSize);
        delayBuffer.clear();

        writePosition = 0;
    }

    void prepareSmoothing(float delayTime, int sampleRate, float width = 0) 
    {
        smoothedDelay.setTargetValue((float)delayTime);
        currentDelayTime = smoothedDelay.getTargetValue() * (float)sampleRate;

        if (width != 0) {
            smoothedWidth.setTargetValue((float)width);
            currentWidth = smoothedWidth.getTargetValue() * (float)sampleRate;
        }
    }

    void prepareDelayBuffer(int channel, bool useLfo = false)
    {
        delayData = delayBuffer.getWritePointer(channel);
        localWritePosition = writePosition;

        if (useLfo) {
            phase = lfoPhase;
        }
    }

    void process(float currentDelayTime)
    {
        out = 0.0f;
        readPosition = fmodf((float)localWritePosition - currentDelayTime + (float)bufferSize, bufferSize);
        localReadPosition = floorf(readPosition);

        if (localReadPosition != localWritePosition) {
            out = cubicInterpolation();
        }
    }

    float cubicInterpolation()
    {
        float fraction = readPosition - (float)localReadPosition;
        float fractionSqrt = fraction * fraction;
        float fractionCube = fractionSqrt * fraction;

        float sample0 = delayData[(localReadPosition - 1 + bufferSize) % bufferSize];
        float sample1 = delayData[(localReadPosition + 0)];
        float sample2 = delayData[(localReadPosition + 1) % bufferSize];
        float sample3 = delayData[(localReadPosition + 2) % bufferSize];

        float a0 = -0.5f * sample0 + 1.5f * sample1 - 1.5f * sample2 + 0.5f * sample3;
        float a1 = sample0 - 2.5f * sample1 + 2.0f * sample2 - 0.5f * sample3;
        float a2 = -0.5f * sample0 + 0.5f * sample2;
        float a3 = sample1;
        float out = a0 * fractionCube + a1 * fractionSqrt + a2 * fraction + a3;

        return out;
    }

    float lfo(bool vibrato = false)
    {
        float out = 0.0f;

        float factor = (vibrato) ? 0.1f : 0.25f;

        out = 0.5f + factor * sinf(twoPi * (phase + phaseOffset));
        return out;
    }

    void calculatePositionAndPhase(float lfoFreq = 0)
    {
        if (++localWritePosition >= bufferSize)
            localWritePosition -= bufferSize;

        phase += lfoFreq * inverseSampleRate;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }

    void updatePositionAndPhase(bool useLfo = false)
    {
        writePosition = localWritePosition;
        if (useLfo) {
            lfoPhase = phase;
        }
    }
};

enum NumOfVoices
{
    Two,
    Three,
    Four,
    Five
};

struct ChainSettings
{
    float delayTime{ 0.5f }, feedback{ 0.5f }, dryLevel{ 1.0f }, wetLevel{ 0.5f };
    float flangDelay{ 0.0025f }, flangWidth{ 0.005f }, flangDepth{ 1.0f },
        flangFeedback{ 0.25f }, flangLfoFreq{ 0.5f };
    float vibWidth{ 1.0f }, vibDepth{ 1.0f }, vibLfoFreq{ 4.f };
    float chorDelay{ 0.030f }, chorWidth{ 0.020f }, chorDepth{ 1.0f },
        chorLfoFreq{ 1.0f };
    NumOfVoices numOfVoices{ NumOfVoices::Two };
    float dryReverb{ 0.5f }, wetReverb{ 0.5f }, roomSize{ 0.25f },
        damping{ 0.8f }, revWidth{ 0.5f };
    bool flangerOn{ true }, vibratoOn{ true }, chorusOn{ true },
        dryReverbOn{ true }, wetReverbOn{ true };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

class MastersDelayAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    MastersDelayAudioProcessor();
    ~MastersDelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    void turnOnFlangerAndEffects();
    void setDelayTimeFromTapTempo(float delayTime);


private:

    DelayLineEffect delay;
    DelayLineEffect flanger;
    DelayLineEffect vibrato;
    DelayLineEffect chorus;

    juce::Reverb dryReverb;
    juce::Reverb::Parameters dryRevParams;
    juce::AudioBuffer<float> dryRevBufferCopy;

    juce::Reverb wetReverb;
    juce::Reverb::Parameters wetRevParams;
    juce::AudioBuffer<float> wetRevBufferCopy;

    std::vector<double> tapTimes;
    std::vector<int> durationVec;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MastersDelayAudioProcessor)
};