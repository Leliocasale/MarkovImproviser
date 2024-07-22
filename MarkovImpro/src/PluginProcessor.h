/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

// #include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../MarkovModelCPP/src/MarkovManager.h"

#include "ChordDetector.h"

//==============================================================================
/**
*/
class MidiMarkovProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    MarkovManager pitchModel;
    MarkovManager iOIModel;
    MarkovManager noteDurationModel;    
    MarkovManager velocityModel;
    bool learnOn = false;
    bool canGenerateNotes = false;
    int CMajor[75] = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24, 26, 28, 29, 31, 33, 35, 36, 38, 40, 41, 43, 45, 47, 48, 
                    50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84, 86, 88, 89, 91, 93, 95, 96, 98, 100,
                    101, 103, 105, 107, 108, 110, 112, 113, 115, 117, 119, 120, 122, 124, 125, 127};
    int CMajorIntervals[33] = {0, 4, 7, 12, 16, 19, 24, 28, 31, 36, 40, 43, 48, 52, 55, 60, 64, 67, 72, 76, 79, 84, 88, 91, 96,
                             100, 103, 108, 112, 115, 120, 124, 127};

    int CSharpMajor[74] = {1, 3, 5, 6, 8, 10, 12, 13, 15, 17, 18, 20, 22, 24, 25, 27, 29, 30, 32, 34, 36, 37, 39, 41, 42, 44, 46, 48, 49, 
                     51, 53, 54, 56, 58, 60, 61, 63, 65, 66, 68, 70, 72, 73, 75, 77, 78, 80, 82, 84, 85, 87, 89, 90, 92, 94, 96, 97, 99, 101,
                     102, 104, 106, 108, 109, 111, 113, 114, 116, 118, 120, 121, 123, 125, 126};
    int CSharpMajorIntervals[32] = {1, 5, 8, 13, 17, 20, 25, 29, 32, 37, 41, 44, 49, 53, 56, 61, 65, 68, 73, 77, 80, 85, 89, 92, 97,
                              101, 104, 109, 113, 116, 121, 125};

    int DMajor[75] = {1, 2, 4, 6, 7, 9, 11, 13, 14, 16, 18, 19, 21, 23, 25, 26, 28, 30, 31, 33, 35, 37, 38, 40, 42, 43, 45, 47, 49, 50, 
                52, 54, 55, 57, 59, 61, 62, 64, 66, 67, 69, 71, 73, 74, 76, 78, 79, 81, 83, 85, 86, 88, 90, 91, 93, 95, 97, 98, 100, 102,
                103, 105, 107, 109, 110, 112, 114, 115, 117, 119, 121, 122, 124, 126, 127};
    int DMajorIntervals[32] = {2, 6, 9, 14, 18, 21, 26, 30, 33, 38, 42, 45, 50, 54, 57, 62, 66, 69, 74, 78, 81, 86, 90, 93, 98,
                         102, 105, 110, 114, 117, 122, 126};

    int DSharpMajor[75] = {0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 20, 22, 24, 26, 27, 29, 31, 32, 34, 36, 38, 39, 41, 43, 44, 46, 48, 50, 51, 
                     53, 55, 56, 58, 60, 62, 63, 65, 67, 68, 70, 72, 74, 75, 77, 79, 80, 82, 84, 86, 87, 89, 91, 92, 94, 96, 98, 99, 101, 103,
                     104, 106, 108, 110, 111, 113, 115, 116, 118, 120, 122, 123, 125, 127};
    int DSharpMajorIntervals[32] = {3, 7, 10, 15, 19, 22, 27, 31, 34, 39, 43, 46, 51, 55, 58, 63, 67, 70, 75, 79, 82, 87, 91, 94, 99,
                              103, 106, 111, 115, 118, 123, 127};

    int EMajor[74] = {1, 3, 4, 6, 8, 9, 11, 13, 15, 16, 18, 20, 21, 23, 25, 27, 28, 30, 32, 33, 35, 37, 39, 40, 42, 44, 45, 47, 49, 51, 52, 
                54, 56, 57, 59, 61, 63, 64, 66, 68, 69, 71, 73, 75, 76, 78, 80, 81, 83, 85, 87, 88, 90, 92, 93, 95, 97, 99, 100, 102, 104,
                105, 107, 109, 111, 112, 114, 116, 117, 119, 121, 123, 124, 126};
    int EMajorIntervals[31] = {4, 8, 11, 16, 20, 23, 28, 32, 35, 40, 44, 47, 52, 56, 59, 64, 68, 71, 76, 80, 83, 88, 92, 95, 100,
                         104, 107, 112, 116, 119, 124};

    int FMajor[75] = {0, 2, 4, 5, 7, 9, 10, 12, 14, 16, 17, 19, 21, 22, 24, 26, 28, 29, 31, 33, 34, 36, 38, 40, 41, 43, 45, 46, 48, 50, 52, 53, 
                55, 57, 58, 60, 62, 64, 65, 67, 69, 70, 72, 74, 76, 77, 79, 81, 82, 84, 86, 88, 89, 91, 93, 94, 96, 98, 100, 101, 103, 105,
                106, 108, 110, 112, 113, 115, 117, 118, 120, 122, 124, 125, 127};
    int FMajorIntervals[32] = {0, 5, 9, 12, 17, 21, 24, 29, 33, 36, 41, 45, 48, 53, 57, 60, 65, 69, 72, 77, 81, 84, 89, 93, 96, 101,
                         105, 108, 113, 117, 120, 125};

    int FSharpMajor[74] = {1, 3, 5, 6, 8, 10, 11, 13, 15, 17, 18, 20, 22, 23, 25, 27, 29, 30, 32, 34, 35, 37, 39, 41, 42, 44, 46, 47, 49, 51, 53, 54, 
                     56, 58, 59, 61, 63, 65, 66, 68, 70, 71, 73, 75, 77, 78, 80, 82, 83, 85, 87, 89, 90, 92, 94, 95, 97, 99, 101, 102, 104, 106,
                     107, 109, 111, 113, 114, 116, 118, 119, 121, 123, 125, 126};
    int FSharpMajorIntervals[32] = {1, 6, 10, 13, 18, 22, 25, 30, 34, 37, 42, 46, 49, 54, 58, 61, 66, 70, 73, 78, 82, 85, 90, 94, 97, 102,
                              106, 109, 114, 118, 121, 126};  

    int GMajor[75] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26, 28, 30, 31, 33, 35, 36, 38, 40, 42, 43, 45, 47, 48, 50, 52, 54, 55, 
                57, 59, 60, 62, 64, 66, 67, 69, 71, 72, 74, 76, 78, 79, 81, 83, 84, 86, 88, 90, 91, 93, 95, 96, 98, 100, 102, 103, 105, 107,
                108, 110, 112, 114, 115, 117, 119, 120, 122, 124, 126, 127};
    int GMajorIntervals[32] = {2, 7, 11, 14, 19, 23, 26, 31, 35, 38, 43, 47, 50, 55, 59, 62, 67, 71, 74, 79, 83, 86, 91, 95, 98, 103,
                         107, 110, 115, 119, 122, 127}; 

    int GSharpMajor[75] = {0, 1, 3, 5, 7, 8, 10, 12, 13, 15, 17, 19, 20, 22, 24, 25, 27, 29, 31, 32, 34, 36, 37, 39, 41, 43, 44, 46, 48, 49, 51, 53, 55, 56, 
                     58, 60, 61, 63, 65, 67, 68, 70, 72, 73, 75, 77, 79, 80, 82, 84, 85, 87, 89, 91, 92, 94, 96, 97, 99, 101, 103, 104, 106, 108,
                     109, 111, 113, 115, 116, 118, 120, 121, 123, 125, 127};
    int GSharpMajorIntervals[33] = {0, 3, 8, 12, 15, 20, 24, 27, 32, 36, 39, 44, 48, 51, 56, 60, 63, 68, 72, 75, 80, 84, 87, 92, 96, 99, 104,
                              108, 111, 116, 120, 123, 127};

    int AMajor[74] = {1, 2, 4, 6, 8, 9, 11, 13, 14, 16, 18, 20, 21, 23, 25, 26, 28, 30, 32, 33, 35, 37, 38, 40, 42, 44, 45, 47, 49, 50, 52, 54, 56, 57, 
                59, 61, 62, 64, 66, 68, 69, 71, 73, 74, 76, 78, 80, 81, 83, 85, 86, 88, 90, 92, 93, 95, 97, 98, 100, 102, 104, 105, 107, 109,
                110, 112, 114, 116, 117, 119, 121, 122, 124, 126};
    int AMajorIntervals[32] = {1, 4, 9, 13, 16, 21, 25, 28, 33, 37, 40, 45, 49, 52, 57, 61, 64, 69, 73, 76, 81, 85, 88, 93, 97, 100, 105,
                         109, 112, 117, 121, 124};

    int ASharpMajor[75] = {0, 2, 3, 5, 7, 9, 10, 12, 14, 15, 17, 19, 21, 22, 24, 26, 27, 29, 31, 33, 34, 36, 38, 39, 41, 43, 45, 46, 48, 50, 51, 53, 55, 57, 58, 
                     60, 62, 63, 65, 67, 69, 70, 72, 74, 75, 77, 79, 81, 82, 84, 86, 87, 89, 91, 93, 94, 96, 98, 99, 101, 103, 105, 106, 108, 110,
                     111, 113, 115, 117, 118, 120, 122, 123, 125, 127};
    int ASharpMajorIntervals[32] = {2, 5, 10, 14, 17, 22, 26, 29, 34, 38, 41, 46, 50, 53, 58, 62, 65, 70, 74, 77, 82, 86, 89, 94, 98, 101, 106,
                              110, 113, 118, 122, 125};

    int BMajor[74] = {1, 3, 4, 6, 8, 10, 11, 13, 15, 16, 18, 20, 22, 23, 25, 27, 28, 30, 32, 34, 35, 37, 39, 40, 42, 44, 46, 47, 49, 51, 52, 54, 56, 58, 59, 
                61, 63, 64, 66, 68, 70, 71, 73, 75, 76, 78, 80, 82, 83, 85, 87, 88, 90, 92, 94, 95, 97, 99, 100, 102, 104, 106, 107, 109, 111,
                112, 114, 116, 118, 119, 121, 123, 124, 126};
    int BMajorIntervals[32] = {3, 6, 11, 15, 18, 23, 27, 30, 35, 39, 42, 47, 51, 54, 59, 63, 66, 71, 75, 78, 83, 87, 90, 95, 99, 102, 107,
                         111, 114, 119, 123, 126};

    int CMinor[75];
    int CSharpMinor[73];
    int DMinor[75];
    int DSharpMinor[74];
    int EMinor[75];
    int FMinor[75];
    int FSharpMinor[74];
    int GMinor[75];
    int GSharpMinor[73];
    int AMinor[75];
    int ASharpMinor[74];
    int BMinor[75];
    
    int CMinorIntervals[33] = {0, 3, 7, 12, 15, 19, 24, 27, 31, 36, 39, 43, 48, 51, 55, 60, 63, 67, 72,
                            75, 79, 84, 87, 91, 96, 99, 103, 108, 111, 115, 120, 123, 127};

    int CSharpMinorIntervals[32] = {1, 4, 8, 13, 16, 20, 25, 28, 32, 37, 40, 44, 49, 52, 56, 61, 64, 68, 73,
                              76, 80, 85, 88, 92, 97, 100, 104, 109, 112, 116, 121, 124};   

    int DMinorIntervals[32] = {2, 5, 9, 14, 17, 21, 26, 29, 33, 38, 41, 45, 50, 53, 57, 62, 65, 69, 74,
                         77, 81, 86, 89, 93, 98, 101, 105, 110, 113, 117, 122, 125};

    int DSharpMinorIntervals[32] = {3, 6, 10, 15, 18, 22, 27, 30, 34, 39, 42, 46, 51, 54, 58, 63, 66, 70, 75,
                              78, 82, 87, 90, 94, 99, 102, 106, 111, 114, 118, 123, 126};

    int EMinorIntervals[32] = {4, 7, 11, 16, 19, 23, 28, 31, 35, 40, 43, 47, 52, 55, 59, 64, 67, 71, 76,
                         79, 83, 88, 91, 95, 100, 103, 107, 112, 115, 119, 124, 127};

    int FMinorIntervals[32] = {0, 5, 8, 12, 17, 20, 24, 29, 32, 36, 41, 44, 48, 53, 56, 60, 65, 68, 72, 77,
                         80, 84, 89, 92, 96, 101, 104, 108, 113, 116, 120, 125};

    int FSharpMinorIntervals[32] = {1, 6, 9, 13, 18, 21, 25, 30, 33, 37, 42, 45, 49, 54, 57, 61, 66, 69, 73, 78,
                              81, 85, 90, 93, 97, 102, 105, 109, 114, 117, 121, 126};

    int GMinorIntervals[32] = {2, 7, 10, 14, 19, 22, 26, 31, 34, 38, 43, 46, 50, 55, 58, 62, 67, 70, 74, 79,
                         82, 86, 91, 94, 98, 103, 106, 110, 115, 118, 122, 127};

    int GSharpMinorIntervals[31] = {3, 8, 11, 15, 20, 23, 27, 32, 35, 39, 44, 47, 51, 56, 59, 63, 68, 71, 75, 80,
                              83, 87, 92, 95, 99, 104, 107, 111, 116, 119, 123};

    int AMinorIntervals[32] = {0, 4, 9, 12, 16, 21, 24, 28, 33, 36, 40, 45, 48, 52, 57, 60, 64, 69, 72, 76, 81,
                         84, 88, 93, 96, 100, 105, 108, 112, 117, 120, 124};

    int ASharpMinorIntervals[32] = {1, 5, 10, 13, 17, 22, 25, 29, 34, 37, 41, 46, 49, 53, 58, 61, 65, 70, 73, 77, 82,
                              85, 89, 94, 97, 101, 106, 109, 113, 118, 121, 125};

    int BMinorIntervals[32] = {2, 6, 11, 14, 18, 23, 26, 30, 35, 38, 42, 47, 50, 54, 59, 62, 66, 71, 74, 78, 83,
                         86, 90, 95, 98, 102, 107, 110, 114, 119, 122, 126};

    int keyProbs[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int maxIndex = -1;

    //==============================================================================
    MidiMarkovProcessor();
    ~MidiMarkovProcessor() override;

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
    /** add some midi to be played at the sent sample offset*/
    void addMidi(const juce::MidiMessage& msg, int sampleOffset);
    void resetMarkovModel();

    void saveMarkovModel(const juce::File& file);
    void loadMarkovModel(const juce::File& file);

    void updateEditorDisplay(juce::String& newText);
private:

    void analysePitches(const juce::MidiBuffer& midiMessages);
    void analyseIoI(const juce::MidiBuffer& midiMessages);
    void analyseDuration(const juce::MidiBuffer& midiMessages);
    void analyseVelocity(const juce::MidiBuffer& midiMessages);
    void analyzeKey(int noteNumber);
    

    std::string notesToMarkovState (const std::vector<int>& notesVec);
    std::vector<int> markovStateToNotes (const std::string& notesStr);

    juce::MidiBuffer generateNotesFromModel(const juce::MidiBuffer& incomingMessages);
    // return true if time to play a note
    bool isTimeToPlayNote(unsigned long currentTime);
    // call after playing a note 
    void updateTimeForNextPlay();

    
    

    /** stores messages added from the addMidi function*/
    juce::MidiBuffer midiToProcess;
        

    unsigned long lastNoteOnTime; 
    bool noMidiYet; 
    unsigned long noteOffTimes[127];
    unsigned long noteOnTimes[127];

    unsigned long elapsedSamples; 
    unsigned long modelPlayNoteTime;
    ChordDetector chordDetect;

    juce::String key = "";
    juce::String newKey = "";
    bool keyLoaded = true;
      //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiMarkovProcessor)
};
