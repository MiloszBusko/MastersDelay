/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
MastersDelayAudioProcessor::MastersDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

MastersDelayAudioProcessor::~MastersDelayAudioProcessor()
{
}

//==============================================================================
const juce::String MastersDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MastersDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MastersDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MastersDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MastersDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MastersDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MastersDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MastersDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MastersDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void MastersDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MastersDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{   
    auto begin = std::chrono::high_resolution_clock::now();

    int totalNumInputChannels = getTotalNumInputChannels();

    delay.prepare(sampleRate, totalNumInputChannels, 3.f);

    flanger.prepare(sampleRate, totalNumInputChannels, 0.0200f + 0.0200f);
    flanger.lfoPhase = 0.f;
    flanger.inverseSampleRate = 1.f / (float)sampleRate;

    vibrato.prepare(sampleRate, totalNumInputChannels, 0.040f);
    vibrato.lfoPhase = 0.f;
    vibrato.inverseSampleRate = 1.f / (float)sampleRate;

    chorus.prepare(sampleRate, totalNumInputChannels, 0.080f);
    chorus.lfoPhase = 0.f;
    chorus.inverseSampleRate = 1.f / (float)sampleRate;

    dryReverb.setSampleRate(sampleRate);
    dryReverb.reset();

    wetReverb.setSampleRate(sampleRate);
    wetReverb.reset();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    DBG("Processing time: " << duration << " microseconds");
}

void MastersDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MastersDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MastersDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();
    auto sampleRate = getSampleRate();

    auto chainSettings = getChainSettings(apvts);
    int caseOfProcessing = 0;

    auto delayTime = chainSettings.delayTime;
    auto feedback = chainSettings.feedback;
    auto dryLevel = chainSettings.dryLevel;
    auto wetLevel = chainSettings.wetLevel;
    delay.prepareSmoothing(delayTime, sampleRate);

    auto flangDelay = chainSettings.flangDelay;
    auto flangWidth = chainSettings.flangWidth;
    auto flangDepth = chainSettings.flangDepth;
    auto flangFeedback = chainSettings.flangFeedback;
    auto flangLfoFreq = chainSettings.flangLfoFreq;
    flanger.prepareSmoothing(flangDelay, sampleRate, flangWidth);
    
    auto vibWidth = chainSettings.vibWidth;
    auto vibLfoFreq = chainSettings.vibLfoFreq;
    auto vibDepth = chainSettings.vibDepth;
    vibrato.prepareSmoothing(vibWidth, sampleRate);

    auto chorDelay = chainSettings.chorDelay;
    auto chorWidth = chainSettings.chorWidth;
    auto chorDepth = chainSettings.chorDepth;
    auto chorLfoFreq = chainSettings.chorLfoFreq;
    auto numOfVoices = chainSettings.numOfVoices;
    chorus.prepareSmoothing(chorDelay, sampleRate, chorWidth);

    auto dryRev = chainSettings.dryReverb;
    auto wetRev = chainSettings.wetReverb;
    auto roomSize = chainSettings.roomSize;
    auto damping = chainSettings.damping;
    auto revWidth = chainSettings.revWidth;

    bool flangerIsOn = chainSettings.flangerOn;
    bool vibratoIsOn = chainSettings.vibratoOn;
    bool chorusIsOn = chainSettings.chorusOn;

    auto dryReverbOn = chainSettings.dryReverbOn;
    auto wetReverbOn = chainSettings.wetReverbOn;

    dryRevParams.wetLevel = dryRev;
    dryRevParams.roomSize = roomSize;
    dryRevParams.damping = damping;
    dryRevParams.width = revWidth;
    dryRevParams.dryLevel = 0.5f;
    dryReverb.setParameters(dryRevParams);
    dryRevBufferCopy.setSize(totalNumInputChannels, numSamples);
    dryRevBufferCopy.clear();

    wetRevParams.wetLevel = wetRev;
    wetRevParams.roomSize = roomSize;
    wetRevParams.damping = damping;
    wetRevParams.width = revWidth;
    wetRevParams.dryLevel = 0.5f;
    wetReverb.setParameters(wetRevParams);
    wetRevBufferCopy.setSize(totalNumInputChannels, numSamples);
    wetRevBufferCopy.clear();

    //==============================================================================
    //PROCESSING

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);

        delay.prepareDelayBuffer(channel);
        flanger.prepareDelayBuffer(channel, true);
        vibrato.prepareDelayBuffer(channel, true);
        chorus.prepareDelayBuffer(channel, true);

        for (int sample = 0; sample < numSamples; ++sample) {
            const float in = channelData[sample];

            dryRevBufferCopy.addSample(channel, sample, in);        

            if (!flangerIsOn) {
                caseOfProcessing = 1;
                
            }
            else if (!vibratoIsOn) {
                caseOfProcessing = 2;
                
            }
            else if (!chorusIsOn) {
                caseOfProcessing = 3;
                
            }
            else {
                caseOfProcessing = 0;
            }

            switch (caseOfProcessing) {
                case 0: {
                    delay.process(delay.currentDelayTime);

                    delay.delayData[delay.localWritePosition] = in + delay.out * feedback;
                    
                    if (wetReverbOn) {
                        channelData[sample] = channelData[sample] * dryLevel + delay.out *  wetLevel;
                    }

                    wetRevBufferCopy.addSample(channel, sample, delay.out);

                    break;
                }
                case 1: {
                    delay.process(delay.currentDelayTime);

                    float localFlangerDelayTime = flanger.currentDelayTime + flanger.currentWidth * flanger.lfo();
                    flanger.process(localFlangerDelayTime);

                    delay.delayData[delay.localWritePosition] = delay.out + flanger.out * flangDepth;
                    flanger.delayData[flanger.localWritePosition] = delay.out + flanger.out * flangFeedback;

                    delay.delayData[delay.localWritePosition] = in + delay.out * feedback;
                    

                    if (wetReverbOn) {
                        channelData[sample] = channelData[sample] * dryLevel + (flanger.out * flangDepth + delay.out) *  wetLevel;
                    }

                    wetRevBufferCopy.addSample(channel, sample, delay.out + flanger.out * flangDepth);


                    break;
                }
                case 2: {
                    delay.process(delay.currentDelayTime);

                    float localVibratoDelayTime = vibrato.currentDelayTime * vibrato.lfo(true);
                    vibrato.process(localVibratoDelayTime);

                    vibrato.delayData[vibrato.localWritePosition] = delay.out;

                    delay.delayData[delay.localWritePosition] = in + delay.out * feedback;

                    if (wetReverbOn) {
                        channelData[sample] = channelData[sample] * dryLevel + (vibDepth * vibrato.out) *  wetLevel;
                    }

                    wetRevBufferCopy.addSample(channel, sample, vibDepth * vibrato.out);

                    break;
                }
                case 3: {
                    delay.process(delay.currentDelayTime);
                    chorus.phaseOffset = 0.0f;

                    for (int voice = 0; voice < numOfVoices + 1; ++voice) {
                        if ((numOfVoices + 2) > 2) {
                            chorus.weight = (float)(voice) / (float)(numOfVoices);
                            if (channel != 0) {
                                chorus.weight = 1.0f - chorus.weight;
                            }
                        }
                        else {
                            chorus.weight = 1.0f;
                        }

                        float localChorusDelayTime = chorus.currentDelayTime + chorus.currentWidth * chorus.lfo();

                        chorus.process(localChorusDelayTime);

                        if ((numOfVoices + 2) == 2) {
                            delay.delayData[delay.localWritePosition] = (channel == 0) ? delay.out : chorus.out * chorDepth;
                        }
                        else {
                            delay.delayData[delay.localWritePosition] = chorus.out * chorDepth * chorus.weight;
                        }

                        if ((numOfVoices + 2) == 3) {
                            chorus.phaseOffset += 0.25f;
                        }
                        else if ((numOfVoices + 2) > 3) {
                            chorus.phaseOffset += 1.0f / (float)(numOfVoices + 1);
                        }
                    }

                    chorus.delayData[chorus.localWritePosition] = delay.out;

                    delay.delayData[delay.localWritePosition] = in + delay.out * feedback;

                    if (wetReverbOn) {
                        channelData[sample] = channelData[sample] * dryLevel + (chorDepth * chorus.out + delay.out) *  wetLevel;
                    }

                    wetRevBufferCopy.addSample(channel, sample, delay.out + chorDepth * chorus.out);


                    break;
                }
            }

            delay.calculatePositionAndPhase();
            flanger.calculatePositionAndPhase(flangLfoFreq);
            vibrato.calculatePositionAndPhase(vibLfoFreq);
            chorus.calculatePositionAndPhase(chorLfoFreq);
        }
    }

    delay.updatePositionAndPhase();
    flanger.updatePositionAndPhase(true);
    vibrato.updatePositionAndPhase(true);
    chorus.updatePositionAndPhase(true);

    if (!dryReverbOn) {
        if (totalNumInputChannels == 1) {
            dryReverb.processMono(dryRevBufferCopy.getWritePointer(0), numSamples);
        }
        else if (totalNumInputChannels == 2) {
            dryReverb.processStereo(dryRevBufferCopy.getWritePointer(0), dryRevBufferCopy.getWritePointer(1), numSamples);
        }
    }

    if (!wetReverbOn) {
        if (totalNumInputChannels == 1) {
            wetReverb.processMono(wetRevBufferCopy.getWritePointer(0), numSamples);
        }
        else if (totalNumInputChannels == 2) {
            wetReverb.processStereo(wetRevBufferCopy.getWritePointer(0), wetRevBufferCopy.getWritePointer(1), numSamples);
        }
    }

    if (!dryReverbOn || !wetReverbOn) {
        for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            float* channelData = buffer.getWritePointer(channel);
            float* directCopyData = dryRevBufferCopy.getWritePointer(channel);
            float* delayCopyData = wetRevBufferCopy.getWritePointer(channel);

            for (int sample = 0; sample < numSamples; ++sample) {
                channelData[sample] = directCopyData[sample] * dryLevel + delayCopyData[sample] * wetLevel;
            }
        }
    }
    for (int channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear(channel, 0, numSamples);
    
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds > (end - begin).count();
    DBG(duration);
    
}
//==============================================================================
// HELP FUNCTIONS

void MastersDelayAudioProcessor::turnOnFlangerAndEffects()
{
    apvts.getParameter("Flanger On")->setValueNotifyingHost(1.0f);  // Turn on flanger
    apvts.getParameter("Vibrato On")->setValueNotifyingHost(1.0f);  // Turn on vibrato
    apvts.getParameter("Chorus On")->setValueNotifyingHost(1.0f);   // Turn on chorus
}

void MastersDelayAudioProcessor::setDelayTimeFromTapTempo(float delayTime)
{
    apvts.getParameter("Delay Time")->setValueNotifyingHost(delayTime);
}

//void MastersDelayAudioProcessor::setParameterNotifyingHost(float newValue)
//{
//    // This is where you update the parameter and notify the host
//    apvts.getParameter("Delay Time")->setValueNotifyingHost(newValue);
//}

//==============================================================================
bool MastersDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MastersDelayAudioProcessor::createEditor()
{
    return new MastersDelayAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void MastersDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void MastersDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.delayTime = apvts.getRawParameterValue("Delay Time")->load();
    settings.feedback = apvts.getRawParameterValue("Feedback")->load();
    settings.dryLevel = apvts.getRawParameterValue("Dry Level")->load();
    settings.wetLevel = apvts.getRawParameterValue("Wet Level")->load();

    settings.flangDelay = apvts.getRawParameterValue("Flanger Delay")->load();
    settings.flangWidth = apvts.getRawParameterValue("Flanger Width")->load();
    settings.flangDepth = apvts.getRawParameterValue("Flanger Depth")->load();
    settings.flangFeedback = apvts.getRawParameterValue("Flanger Feedback")->load();
    settings.flangLfoFreq = apvts.getRawParameterValue("Flanger LFO Frequency")->load();

    settings.vibWidth = apvts.getRawParameterValue("Vibrato Width")->load();
    settings.vibDepth = apvts.getRawParameterValue("Vibrato Depth")->load();
    settings.vibLfoFreq = apvts.getRawParameterValue("Vibrato LFO Frequency")->load();

    settings.chorDelay = apvts.getRawParameterValue("Chorus Delay")->load();
    settings.chorWidth = apvts.getRawParameterValue("Chorus Width")->load();
    settings.chorDepth = apvts.getRawParameterValue("Chorus Depth")->load();
    settings.chorLfoFreq = apvts.getRawParameterValue("Chorus LFO Frequency")->load();
    settings.numOfVoices = static_cast<NumOfVoices>(apvts.getRawParameterValue("Number of Voices")->load());

    settings.dryReverb = apvts.getRawParameterValue("Dry Reverb")->load();
    settings.wetReverb = apvts.getRawParameterValue("Wet Reverb")->load();
    settings.roomSize = apvts.getRawParameterValue("Room Size")->load();
    settings.damping = apvts.getRawParameterValue("Damping")->load();
    settings.revWidth = apvts.getRawParameterValue("Reverb Width")->load();

    settings.flangerOn = apvts.getRawParameterValue("Flanger On")->load() > 0.5f;
    settings.vibratoOn = apvts.getRawParameterValue("Vibrato On")->load() > 0.5f;
    settings.chorusOn = apvts.getRawParameterValue("Chorus On")->load() > 0.5f;
    settings.dryReverbOn = apvts.getRawParameterValue("Dry Reverb On")->load() > 0.5f;
    settings.wetReverbOn = apvts.getRawParameterValue("Wet Reverb On")->load() > 0.5f;

    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout MastersDelayAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay Time", "Delay Time", juce::NormalisableRange<float>(0.1f, 3.0f, 0.001f, 1.f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Feedback", "Feedback", juce::NormalisableRange<float>(0.0f, 0.90f, 0.001f, 1.f), 0.45f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Dry Level", "Dry Level", juce::NormalisableRange<float>(0.0f, 1.00f, 0.01f, 1.f), 1.00f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Wet Level", "Wet Level", juce::NormalisableRange<float>(0.0f, 1.00f, 0.01f, 1.f), 0.50f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Flanger Delay", "Flanger Delay", juce::NormalisableRange<float>(0.0010f, 0.0200f, 0.0005f, 1.f), 0.005f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Flanger Width", "Flanger Width", juce::NormalisableRange<float>(0.001f, 0.020f, 0.001f, 1.f), 0.010f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Flanger Depth", "Flanger Depth", juce::NormalisableRange<float>(0.00f, 1.0f, 0.01f, 1.f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Flanger Feedback", "Flanger Feedback", juce::NormalisableRange<float>(0.00f, 0.50f, 0.005f, 1.f), 0.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Flanger LFO Frequency", "Flanger LFO Frequency", juce::NormalisableRange<float>(0.100f, 2.000f, 0.001f, 1.f), 0.500f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Vibrato Width", "Vibrato Width", juce::NormalisableRange<float>(0.001f, 0.04f, 0.001f, 1.f), 0.02f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Vibrato Depth", "Vibrato Depth", juce::NormalisableRange<float>(0.00f, 1.0f, 0.01f, 1.f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Vibrato LFO Frequency", "Vibrato LFO Frequency", juce::NormalisableRange<float>(0.400f, 8.000f, 0.001f, 1.f), 2.000f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Chorus Delay", "Chorus Delay", juce::NormalisableRange<float>(0.01f, 0.05f, 0.001f, 1.f), 0.03f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Chorus Width", "Chorus Width", juce::NormalisableRange<float>(0.001f, 0.03f, 0.001f, 1.f), 0.01f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Chorus Depth", "Chorus Depth", juce::NormalisableRange<float>(0.00f, 1.0f, 0.01f, 1.f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Chorus LFO Frequency", "Chorus LFO Frequency", juce::NormalisableRange<float>(0.100f, 2.000f, 0.001f, 1.f), 0.500f));

    juce::StringArray numOfVoicesArray;
    numOfVoicesArray.add("2");
    numOfVoicesArray.add("3");
    numOfVoicesArray.add("4");
    numOfVoicesArray.add("5");
    layout.add(std::make_unique <juce::AudioParameterChoice>("Number of Voices", "Number Of Voices", numOfVoicesArray, 1));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Dry Reverb", "Dry Reverb", juce::NormalisableRange<float>(0.00f, 1.00f, 0.01f, 1.f), 0.50f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Wet Reverb", "Wet Reverb", juce::NormalisableRange<float>(0.00f, 1.00f, 0.01f, 1.f), 0.50f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Room Size", "Room Size", juce::NormalisableRange<float>(0.00f, 0.50f, 0.005f, 1.f), 0.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Damping", "Damping", juce::NormalisableRange<float>(0.00f, 1.00f, 0.01f, 1.f), 0.80f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Reverb Width", "Reverb Width", juce::NormalisableRange<float>(0.00f, 1.00f, 0.01f, 1.f), 0.50f));

    layout.add(std::make_unique<juce::AudioParameterBool>("Flanger On", "Flanger On", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("Vibrato On", "Vibrato On", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("Chorus On", "Chorus On", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("Dry Reverb On", "Dry Reverb On", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("Wet Reverb On", "Wet Reverb On", true));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MastersDelayAudioProcessor();
}


