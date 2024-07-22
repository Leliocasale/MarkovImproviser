/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../MarkovModelCPP/src/MarkovChain.h"
#include <algorithm>
#include <array>
#include <random>

//==============================================================================
MidiMarkovProcessor::MidiMarkovProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         )
#endif
      ,
      pitchModel{}, iOIModel{}, velocityModel{}, lastNoteOnTime{0}, elapsedSamples{0}, modelPlayNoteTime{0}, noMidiYet{true}, chordDetect{0}
{
  // set all note off times to zero 

  for (auto i=0;i<127;++i){
    noteOffTimes[i] = 0;
    noteOnTimes[i] = 0;
}   
   
    for (int i = 0; i < 75; i++) {
        CMinor[i] = DSharpMajor[i];
    }
    for (int i = 0; i < std::size(EMajor)-1; i++) {
        CSharpMinor[i] = EMajor[i];
    }
    for (int i = 0; i < std::size(FMajor)-1; i++) {
        DMinor[i] = FMajor[i];
    }
    for (int i = 0; i < std::size(FSharpMajor)-1; i++) {
        DSharpMinor[i] = FSharpMajor[i];
    }
    for (int i = 0; i < std::size(GMajor)-1; i++) {
        EMinor[i] = GMajor[i];
    }
    for (int i = 0; i < std::size(GSharpMajor)-1; i++) {
        FMinor[i] = GSharpMajor[i];
    }
    for (int i = 0; i < std::size(AMajor)-1; i++) {
        FSharpMinor[i] = AMajor[i];
    }
    for (int i = 0; i < std::size(ASharpMajor)-1; i++) {
        GMinor[i] = ASharpMajor[i];
    }
    for (int i = 0; i < std::size(BMajor)-1; i++) {
        GSharpMinor[i] = BMajor[i];
    }
    for (int i = 0; i < std::size(CMajor)-1; i++) {
        AMinor[i] = CMajor[i];
    }
    for (int i = 0; i < std::size(CSharpMajor)-1; i++) {
        ASharpMinor[i] = CSharpMajor[i];
    }
    for (int i = 0; i < std::size(DMajor)-1; i++) {
        BMinor[i] = DMajor[i];
    }
}

MidiMarkovProcessor::~MidiMarkovProcessor()
{
}

//==============================================================================
const juce::String MidiMarkovProcessor::getName() const
{
  return JucePlugin_Name;
}

bool MidiMarkovProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool MidiMarkovProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool MidiMarkovProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double MidiMarkovProcessor::getTailLengthSeconds() const
{
  return 0.0;
}

int MidiMarkovProcessor::getNumPrograms()
{
  return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
            // so this should be at least 1, even if you're not really implementing programs.
}

int MidiMarkovProcessor::getCurrentProgram()
{
  return 0;
}

void MidiMarkovProcessor::setCurrentProgram(int index)
{
}

const juce::String MidiMarkovProcessor::getProgramName(int index)
{
  return {};
}

void MidiMarkovProcessor::changeProgramName(int index, const juce::String &newName)
{
}

//==============================================================================
void MidiMarkovProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  double maxIntervalInSamples = sampleRate * 0.05; // 50ms
  chordDetect = ChordDetector((unsigned long) maxIntervalInSamples); 
}

void MidiMarkovProcessor::releaseResources()
{
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MidiMarkovProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void MidiMarkovProcessor::updateEditorDisplay(juce::String& key)
{
    if (auto* editor = dynamic_cast<MidiMarkovEditor*>(getActiveEditor()))
    {
        editor->updateDynamicText(key);
    }
}

void MidiMarkovProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
  ////////////
  // deal with MIDI

  // transfer any pending notes into the midi messages and
  // clear pending - these messages come from the addMidi function
  // which the UI might call to send notes from the piano widget
  if (!keyLoaded)
    { 
      analyzeKey(200);
      keyLoaded = true;
    }
  
  if (midiToProcess.getNumEvents() > 0)
  {
    midiMessages.addEvents(midiToProcess, midiToProcess.getFirstEventTime(), midiToProcess.getLastEventTime() + 1, 0);
    midiToProcess.clear();
  }
  
    
  if (learnOn){
    for (const auto metadata : midiMessages)
      {
          auto message = metadata.getMessage();
          if (message.isNoteOn())
          {
              int noteNumber = message.getNoteNumber();
              analyzeKey(noteNumber);
              // Use noteNumber as needed
          }
      }
    analysePitches(midiMessages);
    analyseDuration(midiMessages);
    analyseIoI(midiMessages);
    analyseVelocity(midiMessages);
  }
  juce::MidiBuffer generatedMessages;
  if (canGenerateNotes){
    generatedMessages = generateNotesFromModel(midiMessages);
  }
  // send note offs if needed  
  for (auto i = 0; i < 127; ++i)
  {
    if (noteOffTimes[i] > 0 &&
        noteOffTimes[i] < elapsedSamples)
    {
      juce::MidiMessage nOff = juce::MidiMessage::noteOff(1, i, 0.0f);
      generatedMessages.addEvent(nOff, 0);
      noteOffTimes[i] = 0;
    }
  }
  // now you can clear the outgoing buffer if you want
  midiMessages.clear();
  // then add your generated messages
  midiMessages.addEvents(generatedMessages, generatedMessages.getFirstEventTime(), -1, 0);

  elapsedSamples += buffer.getNumSamples();
}

//==============================================================================
bool MidiMarkovProcessor::hasEditor() const
{
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *MidiMarkovProcessor::createEditor()
{
  return new MidiMarkovEditor(*this);
}

//==============================================================================
void MidiMarkovProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries to make it easy to save and load complex data.
}

void MidiMarkovProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  // You should use this method to restore your parameters from this memory block,
  // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new MidiMarkovProcessor();
}

void MidiMarkovProcessor::addMidi(const juce::MidiMessage& msg, int sampleOffset)
{
  midiToProcess.addEvent(msg, sampleOffset);
}

void MidiMarkovProcessor::resetMarkovModel()
{
  pitchModel.reset();
  iOIModel.reset();
  noteDurationModel.reset();
  velocityModel.reset();
  key = "";
  newKey = "";
  for (int i =0; i<24; i++){
    keyProbs[i] = 0;
  }
  updateEditorDisplay(key);
}

void MidiMarkovProcessor::analyseIoI(const juce::MidiBuffer& midiMessages)
{
  // compute the IOI 
  for (const auto metadata : midiMessages){
      auto message = metadata.getMessage();
      if (message.isNoteOn()){   
          unsigned long exactNoteOnTime = elapsedSamples + message.getTimeStamp();
          unsigned long iOI = exactNoteOnTime - lastNoteOnTime;
          if (iOI < getSampleRate() * 2 && 
              iOI > getSampleRate() * 0.05){
            iOIModel.putEvent(std::to_string(iOI));
            // DBG("Note on at: " << exactNoteOnTime << " IOI " << iOI);

          }
          lastNoteOnTime = exactNoteOnTime; 
      }
  }
}
    
void MidiMarkovProcessor::analysePitches(const juce::MidiBuffer& midiMessages)
{
  for (const auto metadata : midiMessages)
  {
    auto message = metadata.getMessage();
    if (message.isNoteOn()){
      chordDetect.addNote(
            message.getNoteNumber(), 
            // add the offset within this buffer
            elapsedSamples + message.getTimeStamp()
        );
      if (chordDetect.hasChord()){
          std::string notes = 
              MidiMarkovProcessor::notesToMarkovState(
                  chordDetect.getChord()
              );
          DBG("Got notes from detector " << notes);
          pitchModel.putEvent(notes);
      }     
      noMidiYet = false;// bootstrap code
    }
  }
}

void MidiMarkovProcessor::analyseDuration(const juce::MidiBuffer& midiMessages)
{
  for (const auto metadata : midiMessages)
  {
    auto message = metadata.getMessage();
    if (message.isNoteOn())
    {
      noteOnTimes[message.getNoteNumber()] = elapsedSamples + message.getTimeStamp();
    }
    if (message.isNoteOff()){
      unsigned long noteOffTime = elapsedSamples + message.getTimeStamp();
      unsigned long noteLength = noteOffTime - 
                                  noteOnTimes[message.getNoteNumber()];
      noteDurationModel.putEvent(std::to_string(noteLength));
    }
  }
}


void MidiMarkovProcessor::analyseVelocity(const juce::MidiBuffer& midiMessages)
{
  // compute the IOI 
  for (const auto metadata : midiMessages){
      auto message = metadata.getMessage();
      if (message.isNoteOn()){   
          auto velocity = message.getVelocity();
          
          velocityModel.putEvent(std::to_string(velocity));
      }
  }
}

juce::MidiBuffer MidiMarkovProcessor::generateNotesFromModel(const juce::MidiBuffer& incomingNotes)
{

  juce::MidiBuffer generatedMessages{};
  if (isTimeToPlayNote(elapsedSamples)){
    if (!noMidiYet){ // not in bootstrapping phase 
      std::string notes = pitchModel.getEvent();
      unsigned long duration = std::stoul(noteDurationModel.getEvent(true));
      juce::uint8 velocity = std::stoi(velocityModel.getEvent(true));
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(0.0, 1.0);
      for (int& note : markovStateToNotes(notes)){
          float randChoice = dis(gen);
          float positiveChoice = dis(gen);
          float intervalChoice = dis(gen);
          int chosenNote = note;
          float randNess = pitchModel.getRandomness();
          std::array<int, 33> keyIntervals;
          std::array<int, 75> keyNotes;
          if (maxIndex != -1){
          if (randChoice < randNess)
            {
              if (positiveChoice > 0.5)
              {
                if (intervalChoice >= 0 && intervalChoice < 0.5)
                  {
                    int skips = rand() % 4;
                    switch (maxIndex) {
                      case 0: std::copy(std::begin(CMajorIntervals), std::end(CMajorIntervals), std::begin(keyIntervals)); break;
                      case 1: std::copy(std::begin(CSharpMajorIntervals), std::end(CSharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 2: std::copy(std::begin(DMajorIntervals), std::end(DMajorIntervals), std::begin(keyIntervals)); break;
                      case 3: std::copy(std::begin(DSharpMajorIntervals), std::end(DSharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 4: std::copy(std::begin(EMajorIntervals), std::end(EMajorIntervals), std::begin(keyIntervals)); break;
                      case 5: std::copy(std::begin(FMajorIntervals), std::end(FMajorIntervals), std::begin(keyIntervals)); break;
                      case 6: std::copy(std::begin(FSharpMajorIntervals), std::end(FSharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 7: std::copy(std::begin(GMajorIntervals), std::end(GMajorIntervals), std::begin(keyIntervals)); break;
                      case 8: std::copy(std::begin(GSharpMajorIntervals), std::end(GSharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 9: std::copy(std::begin(AMajorIntervals), std::end(AMajorIntervals), std::begin(keyIntervals)); break;
                      case 10: std::copy(std::begin(ASharpMajorIntervals), std::end(ASharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 11: std::copy(std::begin(BMajorIntervals), std::end(BMajorIntervals), std::begin(keyIntervals)); break;
                      case 12: std::copy(std::begin(CMinorIntervals), std::end(CMinorIntervals), std::begin(keyIntervals)); break;
                      case 13: std::copy(std::begin(CSharpMinorIntervals), std::end(CSharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 14: std::copy(std::begin(DMinorIntervals), std::end(DMinorIntervals), std::begin(keyIntervals)); break;
                      case 15: std::copy(std::begin(DSharpMinorIntervals), std::end(DSharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 16: std::copy(std::begin(EMinorIntervals), std::end(EMinorIntervals), std::begin(keyIntervals)); break;
                      case 17: std::copy(std::begin(FMinorIntervals), std::end(FMinorIntervals), std::begin(keyIntervals)); break;
                      case 18: std::copy(std::begin(FSharpMinorIntervals), std::end(FSharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 19: std::copy(std::begin(GMinorIntervals), std::end(GMinorIntervals), std::begin(keyIntervals)); break;
                      case 20: std::copy(std::begin(GSharpMinorIntervals), std::end(GSharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 21: std::copy(std::begin(AMinorIntervals), std::end(AMinorIntervals), std::begin(keyIntervals)); break;
                      case 22: std::copy(std::begin(ASharpMinorIntervals), std::end(ASharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 23: std::copy(std::begin(BMinorIntervals), std::end(BMinorIntervals), std::begin(keyIntervals)); break;
                      default: std::copy(std::begin(CMajorIntervals), std::end(CMajorIntervals), std::begin(keyIntervals)); break;
                  }
                    
                    while (skips != -1){
                      note = note+1;
                      if (std::find(keyIntervals.begin(), keyIntervals.end(), note) != keyIntervals.end()){
                        if (skips == 0){
                          chosenNote = note;
                        }
                        skips = skips - 1;
                      }
                    }
                  }
                else if (intervalChoice >= 0.5)
                  {
                    int skipNotes = rand() % 4;
                    

                    switch (maxIndex) {
                      case 0:
                          std::copy(std::begin(CMajor), std::end(CMajor), std::begin(keyNotes));
                          break;
                      case 1:
                          std::copy(std::begin(CSharpMajor), std::end(CSharpMajor), std::begin(keyNotes));
                          break;
                      case 2:
                          std::copy(std::begin(DMajor), std::end(DMajor), std::begin(keyNotes));
                          break;
                      case 3:
                          std::copy(std::begin(DSharpMajor), std::end(DSharpMajor), std::begin(keyNotes));
                          break;
                      case 4:
                          std::copy(std::begin(EMajor), std::end(EMajor), std::begin(keyNotes));
                          break;
                      case 5:
                          std::copy(std::begin(FMajor), std::end(FMajor), std::begin(keyNotes));
                          break;
                      case 6:
                          std::copy(std::begin(FSharpMajor), std::end(FSharpMajor), std::begin(keyNotes));
                          break;
                      case 7:
                          std::copy(std::begin(GMajor), std::end(GMajor), std::begin(keyNotes));
                          break;
                      case 8:
                          std::copy(std::begin(GSharpMajor), std::end(GSharpMajor), std::begin(keyNotes));
                          break;
                      case 9:
                          std::copy(std::begin(AMajor), std::end(AMajor), std::begin(keyNotes));
                          break;
                      case 10:
                          std::copy(std::begin(ASharpMajor), std::end(ASharpMajor), std::begin(keyNotes));
                          break;
                      case 11:
                          std::copy(std::begin(BMajor), std::end(BMajor), std::begin(keyNotes));
                          break;
                      case 12:
                          std::copy(std::begin(CMinor), std::end(CMinor), std::begin(keyNotes));
                          break;
                      case 13:
                          std::copy(std::begin(CSharpMinor), std::end(CSharpMinor), std::begin(keyNotes));
                          break;
                      case 14:
                          std::copy(std::begin(DMinor), std::end(DMinor), std::begin(keyNotes));
                          break;
                      case 15:
                          std::copy(std::begin(DSharpMinor), std::end(DSharpMinor), std::begin(keyNotes));
                          break;
                      case 16:
                          std::copy(std::begin(EMinor), std::end(EMinor), std::begin(keyNotes));
                          break;
                      case 17:
                          std::copy(std::begin(FMinor), std::end(FMinor), std::begin(keyNotes));
                          break;
                      case 18:
                          std::copy(std::begin(FSharpMinor), std::end(FSharpMinor), std::begin(keyNotes));
                          break;
                      case 19:
                          std::copy(std::begin(GMinor), std::end(GMinor), std::begin(keyNotes));
                          break;
                      case 20:
                          std::copy(std::begin(GSharpMinor), std::end(GSharpMinor), std::begin(keyNotes));
                          break;
                      case 21:
                          std::copy(std::begin(AMinor), std::end(AMinor), std::begin(keyNotes));
                          break;
                      case 22:
                          std::copy(std::begin(ASharpMinor), std::end(ASharpMinor), std::begin(keyNotes));
                          break;
                      case 23:
                          std::copy(std::begin(BMinor), std::end(BMinor), std::begin(keyNotes));
                          break;
                      default:
                          std::copy(std::begin(CMajor), std::end(CMajor), std::begin(keyNotes));
                          break;
                  }

                    while (skipNotes != -1){
                      note = note+1;
                      if (std::find(keyNotes.begin(), keyNotes.end(), note) != keyNotes.end()){
                        if (skipNotes == 0){
                          chosenNote = note;
                        }
                        skipNotes = skipNotes - 1;
                      }
                    }
                  }
              }

              else if (positiveChoice <= 0.5)
              {
                if (intervalChoice >= 0 && intervalChoice < 0.5)
                  {
                    int skips = rand() % 4;
                    
                    switch (maxIndex) {
                      case 0: std::copy(std::begin(CMajorIntervals), std::end(CMajorIntervals), std::begin(keyIntervals)); break;
                      case 1: std::copy(std::begin(CSharpMajorIntervals), std::end(CSharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 2: std::copy(std::begin(DMajorIntervals), std::end(DMajorIntervals), std::begin(keyIntervals)); break;
                      case 3: std::copy(std::begin(DSharpMajorIntervals), std::end(DSharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 4: std::copy(std::begin(EMajorIntervals), std::end(EMajorIntervals), std::begin(keyIntervals)); break;
                      case 5: std::copy(std::begin(FMajorIntervals), std::end(FMajorIntervals), std::begin(keyIntervals)); break;
                      case 6: std::copy(std::begin(FSharpMajorIntervals), std::end(FSharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 7: std::copy(std::begin(GMajorIntervals), std::end(GMajorIntervals), std::begin(keyIntervals)); break;
                      case 8: std::copy(std::begin(GSharpMajorIntervals), std::end(GSharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 9: std::copy(std::begin(AMajorIntervals), std::end(AMajorIntervals), std::begin(keyIntervals)); break;
                      case 10: std::copy(std::begin(ASharpMajorIntervals), std::end(ASharpMajorIntervals), std::begin(keyIntervals)); break;
                      case 11: std::copy(std::begin(BMajorIntervals), std::end(BMajorIntervals), std::begin(keyIntervals)); break;
                      case 12: std::copy(std::begin(CMinorIntervals), std::end(CMinorIntervals), std::begin(keyIntervals)); break;
                      case 13: std::copy(std::begin(CSharpMinorIntervals), std::end(CSharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 14: std::copy(std::begin(DMinorIntervals), std::end(DMinorIntervals), std::begin(keyIntervals)); break;
                      case 15: std::copy(std::begin(DSharpMinorIntervals), std::end(DSharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 16: std::copy(std::begin(EMinorIntervals), std::end(EMinorIntervals), std::begin(keyIntervals)); break;
                      case 17: std::copy(std::begin(FMinorIntervals), std::end(FMinorIntervals), std::begin(keyIntervals)); break;
                      case 18: std::copy(std::begin(FSharpMinorIntervals), std::end(FSharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 19: std::copy(std::begin(GMinorIntervals), std::end(GMinorIntervals), std::begin(keyIntervals)); break;
                      case 20: std::copy(std::begin(GSharpMinorIntervals), std::end(GSharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 21: std::copy(std::begin(AMinorIntervals), std::end(AMinorIntervals), std::begin(keyIntervals)); break;
                      case 22: std::copy(std::begin(ASharpMinorIntervals), std::end(ASharpMinorIntervals), std::begin(keyIntervals)); break;
                      case 23: std::copy(std::begin(BMinorIntervals), std::end(BMinorIntervals), std::begin(keyIntervals)); break;
                      default: std::copy(std::begin(CMajorIntervals), std::end(CMajorIntervals), std::begin(keyIntervals)); break;
                  }
                    
                    while (skips != -1){
                      note = note-1;
                      if (std::find(keyIntervals.begin(), keyIntervals.end(), note) != keyIntervals.end()){
                        if (skips == 0){
                          chosenNote = note;
                        }
                        skips = skips - 1;
                      }
                    }
                  }
                else if (intervalChoice >= 0.5)
                  {
                    int skipNotes = rand() % 4;
                    
                    switch (maxIndex) {
                      case 0:
                          std::copy(std::begin(CMajor), std::end(CMajor), std::begin(keyNotes));
                          break;
                      case 1:
                          std::copy(std::begin(CSharpMajor), std::end(CSharpMajor), std::begin(keyNotes));
                          break;
                      case 2:
                          std::copy(std::begin(DMajor), std::end(DMajor), std::begin(keyNotes));
                          break;
                      case 3:
                          std::copy(std::begin(DSharpMajor), std::end(DSharpMajor), std::begin(keyNotes));
                          break;
                      case 4:
                          std::copy(std::begin(EMajor), std::end(EMajor), std::begin(keyNotes));
                          break;
                      case 5:
                          std::copy(std::begin(FMajor), std::end(FMajor), std::begin(keyNotes));
                          break;
                      case 6:
                          std::copy(std::begin(FSharpMajor), std::end(FSharpMajor), std::begin(keyNotes));
                          break;
                      case 7:
                          std::copy(std::begin(GMajor), std::end(GMajor), std::begin(keyNotes));
                          break;
                      case 8:
                          std::copy(std::begin(GSharpMajor), std::end(GSharpMajor), std::begin(keyNotes));
                          break;
                      case 9:
                          std::copy(std::begin(AMajor), std::end(AMajor), std::begin(keyNotes));
                          break;
                      case 10:
                          std::copy(std::begin(ASharpMajor), std::end(ASharpMajor), std::begin(keyNotes));
                          break;
                      case 11:
                          std::copy(std::begin(BMajor), std::end(BMajor), std::begin(keyNotes));
                          break;
                      case 12:
                          std::copy(std::begin(CMinor), std::end(CMinor), std::begin(keyNotes));
                          break;
                      case 13:
                          std::copy(std::begin(CSharpMinor), std::end(CSharpMinor), std::begin(keyNotes));
                          break;
                      case 14:
                          std::copy(std::begin(DMinor), std::end(DMinor), std::begin(keyNotes));
                          break;
                      case 15:
                          std::copy(std::begin(DSharpMinor), std::end(DSharpMinor), std::begin(keyNotes));
                          break;
                      case 16:
                          std::copy(std::begin(EMinor), std::end(EMinor), std::begin(keyNotes));
                          break;
                      case 17:
                          std::copy(std::begin(FMinor), std::end(FMinor), std::begin(keyNotes));
                          break;
                      case 18:
                          std::copy(std::begin(FSharpMinor), std::end(FSharpMinor), std::begin(keyNotes));
                          break;
                      case 19:
                          std::copy(std::begin(GMinor), std::end(GMinor), std::begin(keyNotes));
                          break;
                      case 20:
                          std::copy(std::begin(GSharpMinor), std::end(GSharpMinor), std::begin(keyNotes));
                          break;
                      case 21:
                          std::copy(std::begin(AMinor), std::end(AMinor), std::begin(keyNotes));
                          break;
                      case 22:
                          std::copy(std::begin(ASharpMinor), std::end(ASharpMinor), std::begin(keyNotes));
                          break;
                      case 23:
                          std::copy(std::begin(BMinor), std::end(BMinor), std::begin(keyNotes));
                          break;
                      default:
                          std::copy(std::begin(CMajor), std::end(CMajor), std::begin(keyNotes));
                          break;
                  }

                    while (skipNotes != -1){
                      note = note-1;
                      if (std::find(keyNotes.begin(), keyNotes.end(), note) != keyNotes.end()){
                        if (skipNotes == 0){
                          chosenNote = note;
                        }
                        skipNotes = skipNotes - 1;
                      }
                    }
                  }
              }
            }
          }
          juce::MidiMessage nOn = juce::MidiMessage::noteOn(1, chosenNote, velocity);
          generatedMessages.addEvent(nOn, 0);
          noteOffTimes[chosenNote] = elapsedSamples + duration; 
      }
    }
    unsigned long nextIoI = std::stoi(iOIModel.getEvent());

    
    if (nextIoI > 0){
      modelPlayNoteTime = elapsedSamples + nextIoI;
      
    } 
  }
  return generatedMessages;
}

bool MidiMarkovProcessor::isTimeToPlayNote(unsigned long currentTime)
{
  // if (modelPlayNoteTime == 0){
  //   return false; 
  // }
  if (currentTime >= modelPlayNoteTime){
    return true;
  }
  else {
    return false; 
  }
}

// call after playing a note 
void MidiMarkovProcessor::updateTimeForNextPlay()
{

}

std::string MidiMarkovProcessor::notesToMarkovState(
               const std::vector<int>& notesVec)
{
std::string state{""};
for (const int& note : notesVec){
  state += std::to_string(note) + "-";
}
return state; 
}

std::vector<int> MidiMarkovProcessor::markovStateToNotes(
              const std::string& notesStr)
{
  std::vector<int> notes{};
  if (notesStr == "0") return notes;
  for (const std::string& note : 
           MarkovChain::tokenise(notesStr, '-')){
    notes.push_back(std::stoi(note));
  }
  return notes; 
}

void MidiMarkovProcessor::saveMarkovModel(const juce::File& file)
{
    juce::String combinedModel = "#PITCH#" + juce::String(pitchModel.getModelAsString()) +
                                 "#IOI#" + juce::String(iOIModel.getModelAsString()) +
                                 "#DURATION#" + juce::String(noteDurationModel.getModelAsString()) +
                                 "#VELOCITY#" + juce::String(velocityModel.getModelAsString())
                                 + "#KEYARRAY#";
    
    for (int i=0; i<24; i++){
      combinedModel = combinedModel + juce::String(keyProbs[i]) + "-";
    }
    
    file.replaceWithText(combinedModel);
}

void MidiMarkovProcessor::loadMarkovModel(const juce::File& file)
{
    if (file.existsAsFile())
    {
        juce::String combinedModel = file.loadFileAsString();
        //combinedModel = combinedModel.replace("\r", "");
        juce::String pitchString = combinedModel.fromFirstOccurrenceOf("#PITCH#", false, false)
                                      .upToFirstOccurrenceOf("#IOI#", false, false);
        //file.replaceWithText(pitchString);
        juce::String IOIString = combinedModel.fromFirstOccurrenceOf("#IOI#", false, false)
                                    .upToFirstOccurrenceOf("#DURATION#", false, false);
        juce::String durationString = combinedModel.fromFirstOccurrenceOf("#DURATION#", false, false)
                                         .upToFirstOccurrenceOf("#VELOCITY#", false, false);
        juce::String velocityString = combinedModel.fromFirstOccurrenceOf("#VELOCITY#", false, false)
                                          .upToFirstOccurrenceOf("#KEYARRAY#", false, false);   
        juce::String keyString = combinedModel.fromFirstOccurrenceOf("#KEYARRAY#", false, false);  
        
        int maxSize = 24;
        int count = 0;

        juce::StringArray tokens;
        tokens.addTokens(keyString, "-", "");

        for (int i = 0; i < tokens.size() && i < maxSize; ++i) {
            keyProbs[i] = tokens[i].getIntValue();
            count++;
        }

        keyLoaded = false;
        
        std::string pitchStringStd = pitchString.toStdString();
        
        pitchModel.setupModelFromString(pitchStringStd);
        iOIModel.setupModelFromString(IOIString.toStdString());
        noteDurationModel.setupModelFromString(durationString.toStdString());
        velocityModel.setupModelFromString(velocityString.toStdString());

        
    }
}

void MidiMarkovProcessor::analyzeKey(int noteNumber){

    
    //C Major
    if (std::find(std::begin(CMajor), std::end(CMajor), noteNumber) 
                                  != std::end(CMajor))
    {
      keyProbs[0] = keyProbs[0] + 1;
    }
    if (std::find(std::begin(CMajorIntervals), std::end(CMajorIntervals), noteNumber) 
                                  != std::end(CMajorIntervals))
    {
      keyProbs[0] = keyProbs[0] + 1;
    }
    if (noteNumber % 12 == 0){
        keyProbs[0] = keyProbs[0] + 1;
    }

    //C# Major
    if (std::find(std::begin(CSharpMajor), std::end(CSharpMajor), noteNumber) 
                                  != std::end(CSharpMajor))
    {
      keyProbs[1] = keyProbs[1] + 1;
    }
    if (std::find(std::begin(CSharpMajorIntervals), std::end(CSharpMajorIntervals), noteNumber) 
                                  != std::end(CSharpMajorIntervals))
    {
      keyProbs[1] = keyProbs[1] + 1;
    }
    if (noteNumber % 12 == 1){
        keyProbs[1] = keyProbs[1] + 1;
    }

    //D Major
    if (std::find(std::begin(DMajor), std::end(DMajor), noteNumber) 
                                  != std::end(DMajor))
    {
      keyProbs[2] = keyProbs[2] + 1;
    }
    if (std::find(std::begin(DMajorIntervals), std::end(DMajorIntervals), noteNumber) 
                                  != std::end(DMajorIntervals))
    {
      keyProbs[2] = keyProbs[2] + 1;
    }
    if (noteNumber % 12 == 2){
        keyProbs[2] = keyProbs[2] + 1;
    }

    //D# Major
    if (std::find(std::begin(DSharpMajor), std::end(DSharpMajor), noteNumber) 
                                  != std::end(DSharpMajor))
    {
      keyProbs[3] = keyProbs[3] + 1;
    }
    if (std::find(std::begin(DSharpMajorIntervals), std::end(DSharpMajorIntervals), noteNumber) 
                                  != std::end(DSharpMajorIntervals))
    {
      keyProbs[3] = keyProbs[3] + 1;
    }
    if (noteNumber % 12 == 3){
        keyProbs[3] = keyProbs[3] + 1;
    }

    //E Major
    if (std::find(std::begin(EMajor), std::end(EMajor), noteNumber) 
                                  != std::end(EMajor))
    {
      keyProbs[4] = keyProbs[4] + 1;
    }
    if (std::find(std::begin(EMajorIntervals), std::end(EMajorIntervals), noteNumber) 
                                  != std::end(EMajorIntervals))
    {
      keyProbs[4] = keyProbs[4] + 1;
    }
    if (noteNumber % 12 == 4){
        keyProbs[4] = keyProbs[4] + 1;
    }

    //F Major
    if (std::find(std::begin(FMajor), std::end(FMajor), noteNumber) 
                                  != std::end(FMajor))
    {
      keyProbs[5] = keyProbs[5] + 1;
    }
    if (std::find(std::begin(FMajorIntervals), std::end(FMajorIntervals), noteNumber) 
                                  != std::end(FMajorIntervals))
    {
      keyProbs[5] = keyProbs[5] + 1;
    }
    if (noteNumber % 12 == 5){
        keyProbs[5] = keyProbs[5] + 1;
    }

    //F# Major
    if (std::find(std::begin(FSharpMajor), std::end(FSharpMajor), noteNumber) 
                                  != std::end(FSharpMajor))
    {
      keyProbs[6] = keyProbs[6] + 1;
    }
    if (std::find(std::begin(FSharpMajorIntervals), std::end(FSharpMajorIntervals), noteNumber) 
                                  != std::end(FSharpMajorIntervals))
    {
      keyProbs[6] = keyProbs[6] + 1;
    }
    if (noteNumber % 12 == 6){
        keyProbs[6] = keyProbs[6] + 1;
    }

    //G Major
    if (std::find(std::begin(GMajor), std::end(GMajor), noteNumber) 
                                  != std::end(GMajor))
    {
      keyProbs[7] = keyProbs[7] + 1;
    }
    if (std::find(std::begin(GMajorIntervals), std::end(GMajorIntervals), noteNumber) 
                                  != std::end(GMajorIntervals))
    {
      keyProbs[7] = keyProbs[7] + 1;
    }
    if (noteNumber % 12 == 7){
        keyProbs[7] = keyProbs[7] + 1;
    }

    //G# Major
    if (std::find(std::begin(GSharpMajor), std::end(GSharpMajor), noteNumber) 
                                  != std::end(GSharpMajor))
    {
      keyProbs[8] = keyProbs[8] + 1;
    }
    if (std::find(std::begin(GSharpMajorIntervals), std::end(GSharpMajorIntervals), noteNumber) 
                                  != std::end(GSharpMajorIntervals))
    {
      keyProbs[8] = keyProbs[8] + 1;
    }
    if (noteNumber % 12 == 8){
        keyProbs[8] = keyProbs[8] + 1;
    }

    //A Major
    if (std::find(std::begin(AMajor), std::end(AMajor), noteNumber) 
                                  != std::end(AMajor))
    {
      keyProbs[9] = keyProbs[9] + 1;
    }
    if (std::find(std::begin(AMajorIntervals), std::end(AMajorIntervals), noteNumber) 
                                  != std::end(AMajorIntervals))
    {
      keyProbs[9] = keyProbs[9] + 1;
    }
    if (noteNumber % 12 == 9){
        keyProbs[9] = keyProbs[9] + 1;
    }

    //A# Major
    if (std::find(std::begin(ASharpMajor), std::end(ASharpMajor), noteNumber) 
                                  != std::end(ASharpMajor))
    {
      keyProbs[10] = keyProbs[10] + 1;
    }
    if (std::find(std::begin(ASharpMajorIntervals), std::end(ASharpMajorIntervals), noteNumber) 
                                  != std::end(ASharpMajorIntervals))
    {
      keyProbs[10] = keyProbs[10] + 1;
    }
    if (noteNumber % 12 == 10){
        keyProbs[10] = keyProbs[10] + 1;
    }

    //B Major
    if (std::find(std::begin(BMajor), std::end(BMajor), noteNumber) 
                                  != std::end(BMajor))
    {
      keyProbs[11] = keyProbs[11] + 1;
    }
    if (std::find(std::begin(BMajorIntervals), std::end(BMajorIntervals), noteNumber) 
                                  != std::end(BMajorIntervals))
    {
      keyProbs[11] = keyProbs[11] + 1;
    }
    if (noteNumber % 12 == 11){
        keyProbs[11] = keyProbs[11] + 1;
    }

    //C Minor
    if (std::find(std::begin(CMinor), std::end(CMinor), noteNumber) != std::end(CMinor))
    {
      keyProbs[12] = keyProbs[12] + 1;
    }
    if (std::find(std::begin(CMinorIntervals), std::end(CMinorIntervals), noteNumber) != std::end(CMinorIntervals))
    {
      keyProbs[12] = keyProbs[12] + 1;
    }
    if (noteNumber % 12 == 0){
        keyProbs[12] = keyProbs[12] + 1;
    }

    //C# Minor
    if (std::find(std::begin(CSharpMinor), std::end(CSharpMinor), noteNumber) != std::end(CSharpMinor))
    {
      keyProbs[13] = keyProbs[13] + 1;
    }
    if (std::find(std::begin(CSharpMinorIntervals), std::end(CSharpMinorIntervals), noteNumber) != std::end(CSharpMinorIntervals))
    {
      keyProbs[13] = keyProbs[13] + 1;
    }
    if (noteNumber % 12 == 1){
        keyProbs[13] = keyProbs[13] + 1;
    }

    //D Minor
    if (std::find(std::begin(DMinor), std::end(DMinor), noteNumber) != std::end(DMinor))
    {
      keyProbs[14] = keyProbs[14] + 1;
    }
    if (std::find(std::begin(DMinorIntervals), std::end(DMinorIntervals), noteNumber) != std::end(DMinorIntervals))
    {
      keyProbs[14] = keyProbs[14] + 1;
    }
    if (noteNumber % 12 == 2){
        keyProbs[14] = keyProbs[14] + 1;
    }

    //D# Minor
    if (std::find(std::begin(DSharpMinor), std::end(DSharpMinor), noteNumber) != std::end(DSharpMinor))
    {
      keyProbs[15] = keyProbs[15] + 1;
    }
    if (std::find(std::begin(DSharpMinorIntervals), std::end(DSharpMinorIntervals), noteNumber) != std::end(DSharpMinorIntervals))
    {
      keyProbs[15] = keyProbs[15] + 1;
    }
    if (noteNumber % 12 == 3){
        keyProbs[15] = keyProbs[15] + 1;
    }

    //E Minor
    if (std::find(std::begin(EMinor), std::end(EMinor), noteNumber) != std::end(EMinor))
    {
      keyProbs[16] = keyProbs[16] + 1;
    }
    if (std::find(std::begin(EMinorIntervals), std::end(EMinorIntervals), noteNumber) != std::end(EMinorIntervals))
    {
      keyProbs[16] = keyProbs[16] + 1;
    }
    if (noteNumber % 12 == 4){
        keyProbs[16] = keyProbs[16] + 1;
    }

    //F Minor
    if (std::find(std::begin(FMinor), std::end(FMinor), noteNumber) != std::end(FMinor))
    {
      keyProbs[17] = keyProbs[17] + 1;
    }
    if (std::find(std::begin(FMinorIntervals), std::end(FMinorIntervals), noteNumber) != std::end(FMinorIntervals))
    {
      keyProbs[17] = keyProbs[17] + 1;
    }
    if (noteNumber % 12 == 5){
        keyProbs[17] = keyProbs[17] + 1;
    }

    //F# Minor
    if (std::find(std::begin(FSharpMinor), std::end(FSharpMinor), noteNumber) != std::end(FSharpMinor))
    {
      keyProbs[18] = keyProbs[18] + 1;
    }
    if (std::find(std::begin(FSharpMinorIntervals), std::end(FSharpMinorIntervals), noteNumber) != std::end(FSharpMinorIntervals))
    {
      keyProbs[18] = keyProbs[18] + 1;
    }
    if (noteNumber % 12 == 6){
        keyProbs[18] = keyProbs[18] + 1;
    }

    //G Minor
    if (std::find(std::begin(GMinor), std::end(GMinor), noteNumber) != std::end(GMinor))
    {
      keyProbs[19] = keyProbs[19] + 1;
    }
    if (std::find(std::begin(GMinorIntervals), std::end(GMinorIntervals), noteNumber) != std::end(GMinorIntervals))
    {
      keyProbs[19] = keyProbs[19] + 1;
    }
    if (noteNumber % 12 == 7){
        keyProbs[19] = keyProbs[19] + 1;
    }

    //G# Minor
    if (std::find(std::begin(GSharpMinor), std::end(GSharpMinor), noteNumber) != std::end(GSharpMinor))
    {
      keyProbs[20] = keyProbs[20] + 1;
    }
    if (std::find(std::begin(GSharpMinorIntervals), std::end(GSharpMinorIntervals), noteNumber) != std::end(GSharpMinorIntervals))
    {
      keyProbs[20] = keyProbs[20] + 1;
    }
    if (noteNumber % 12 == 8){
        keyProbs[20] = keyProbs[20] + 1;
    }

    //A Minor
    if (std::find(std::begin(AMinor), std::end(AMinor), noteNumber) != std::end(AMinor))
    {
      keyProbs[21] = keyProbs[21] + 1;
    }
    if (std::find(std::begin(AMinorIntervals), std::end(AMinorIntervals), noteNumber) != std::end(AMinorIntervals))
    {
      keyProbs[21] = keyProbs[21] + 1;
    }
    if (noteNumber % 12 == 9){
        keyProbs[21] = keyProbs[21] + 1;
    }

    //A# Minor
    if (std::find(std::begin(ASharpMinor), std::end(ASharpMinor), noteNumber) != std::end(ASharpMinor))
    {
      keyProbs[22] = keyProbs[22] + 1;
    }
    if (std::find(std::begin(ASharpMinorIntervals), std::end(ASharpMinorIntervals), noteNumber) != std::end(ASharpMinorIntervals))
    {
      keyProbs[22] = keyProbs[22] + 1;
    }
    if (noteNumber % 12 == 10){
        keyProbs[22] = keyProbs[22] + 1;
    }

    //B Minor
    if (std::find(std::begin(BMinor), std::end(BMinor), noteNumber) != std::end(BMinor))
    {
      keyProbs[23] = keyProbs[23] + 1;
    }
    if (std::find(std::begin(BMinorIntervals), std::end(BMinorIntervals), noteNumber) != std::end(BMinorIntervals))
    {
      keyProbs[23] = keyProbs[23] + 1;
    }
    if (noteNumber % 12 == 11){
        keyProbs[23] = keyProbs[23] + 1;
    }

    
    
    int max = 0;
    for (int i = 0; i < 24; i++){
        if (keyProbs[i] > max) {
          max = keyProbs[i];
          maxIndex = i;
        }
    }
    switch (maxIndex) {
    case 0:
        newKey = "C Major";
        break;
    case 1:
        newKey = "C# Major";
        break;
    case 2:
        newKey = "D Major";
        break;
    case 3:
        newKey = "D# Major";
        break;
    case 4:
        newKey = "E Major";
        break;
    case 5:
        newKey = "F Major"; 
        break;
    case 6:
        newKey = "F# Major";
        break;
    case 7:
        newKey = "G Major";
        break;
    case 8:
        newKey = "G# Major";
        break;
    case 9:
        newKey = "A Major";
        break;
    case 10:
        newKey = "A# Major";
        break;
    case 11:
        newKey = "B Major";
        break;
    case 12:
        newKey = "C Minor";
        break;
    case 13:
        newKey = "C# Minor";
        break;
    case 14:
        newKey = "D Minor";
        break;
    case 15:
        newKey = "D# Minor";
        break;
    case 16:
        newKey = "E Minor";
        break;
    case 17:
        newKey = "F Minor";
        break;
    case 18:
        newKey = "F# Minor";
        break;
    case 19:
        newKey = "G Minor";
        break;
    case 20:
        newKey = "G# Minor";
        break;
    case 21:
        newKey = "A Minor";
        break;
    case 22:
        newKey = "A# Minor";
        break;
    case 23:
        newKey = "B Minor";
        break;
    default:
        newKey = "";
    }
    if (newKey != key){
      key = newKey;
      updateEditorDisplay(key);
    }
    
}

