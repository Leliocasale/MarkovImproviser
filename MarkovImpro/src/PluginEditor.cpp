/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MidiMarkovEditor::MidiMarkovEditor (MidiMarkovProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), 
    miniPianoKbd{kbdState, juce::MidiKeyboardComponent::horizontalKeyboard} 

{    


    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (700, 350);

    // listen to the mini piano
    kbdState.addListener(this);  
    addAndMakeVisible(miniPianoKbd);
    addAndMakeVisible(resetButton);
    //resetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkblue);
    resetButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    //resetButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::lightblue);
    resetButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    resetButton.setButtonText("Reset Markov Model");
    resetButton.addListener(this);

    

    addAndMakeVisible(onOffButton);
    onOffButton.setButtonText("Learning On/Off");
    onOffButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::lightgreen);
    onOffButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    onOffButton.addListener(this);

    addAndMakeVisible(genButton);
    genButton.setButtonText("Generation On/Off");
    genButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::lightgreen);
    genButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    genButton.addListener(this);

    addAndMakeVisible(saveButton);
    //saveButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkblue);
    saveButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    //saveButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::lightblue);
    saveButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    
    saveButton.setButtonText("Save Model");
    saveButton.addListener(this);

    addAndMakeVisible(loadButton);
    //loadButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkblue);
    loadButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    //loadButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::lightblue);
    loadButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    loadButton.setButtonText("Load Model");
    loadButton.addListener(this);

    
    randomnessSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 200, 20);
    randomnessSlider.setColour(juce::Slider::thumbColourId, juce::Colours::lightblue);
    randomnessSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkblue);
    randomnessSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    randomnessSlider.setRange(0.0, 100.0, 0.1);
    randomnessSlider.setTextValueSuffix("% Randomness");
    randomnessSlider.addListener(this);
    addAndMakeVisible(randomnessSlider);    

    dynamicTextLabel.setText("Detected Key: ", juce::dontSendNotification);
    dynamicTextLabel.setFont(juce::Font(24.0f).boldened());
    dynamicTextLabel.setJustificationType(juce::Justification::left);
    addAndMakeVisible(dynamicTextLabel);
}

MidiMarkovEditor::~MidiMarkovEditor()
{
}

//==============================================================================
void MidiMarkovEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

   // g.setColour (juce::Colours::white);
   // g.setFont (15.0f);
   // g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void MidiMarkovEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    float rowHeight = getHeight()/6; 
    float colWidth = getWidth() / 3;
    float row = 0;


    miniPianoKbd.setBounds(0, rowHeight*row, getWidth(), rowHeight);
    row ++ ; 
    resetButton.setBounds(0, rowHeight*row, getWidth(), rowHeight);
    row ++;
    saveButton.setBounds(0, rowHeight*row, getWidth()/2 - 1, rowHeight);
    loadButton.setBounds(getWidth()/2, rowHeight*row, getWidth()/2, rowHeight);
    row ++;
    randomnessSlider.setBounds(0, rowHeight*row+5, getWidth(), rowHeight);
    row++;
    onOffButton.setBounds(0, rowHeight*row, getWidth()/5, rowHeight-1);
    dynamicTextLabel.setBounds(getWidth()/3, rowHeight*row, 2*colWidth, 2*rowHeight);
    row ++;
    genButton.setBounds(0, rowHeight*row-10, getWidth()/5, rowHeight);
    
    
}

 void MidiMarkovEditor::sliderValueChanged (juce::Slider *slider)
{
    
    if (slider == &randomnessSlider)
    {
        audioProcessor.pitchModel.chain.randomness = (slider->getValue()) / 100;
        audioProcessor.iOIModel.chain.randomness = (slider->getValue()) / 100;
        audioProcessor.noteDurationModel.chain.randomness = (slider->getValue()) / 100;
        audioProcessor.velocityModel.chain.randomness = (slider->getValue()) / 100;
    }
    
}

void MidiMarkovEditor::buttonClicked(juce::Button* btn)
{
    if (btn == &resetButton){
        audioProcessor.resetMarkovModel();
    }
    else if (btn == &onOffButton) {
        audioProcessor.learnOn = onOffButton.getToggleState();
    }
    else if (btn == &genButton) {
        audioProcessor.canGenerateNotes = genButton.getToggleState();
    }
    else if (btn == &saveButton)
    {
        juce::FileChooser chooser("Save Markov Model", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "*.txt");
        if (chooser.browseForFileToOpen()) {
            juce::File file = chooser.getResult();
            audioProcessor.saveMarkovModel(file);
        }
    }
    else if (btn == &loadButton)
    {
        juce::FileChooser chooser("Load Markov Model", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "*.txt");
        if (chooser.browseForFileToOpen()) {
            juce::File file = chooser.getResult();
            audioProcessor.loadMarkovModel(file);
        }
    }
}

void MidiMarkovEditor::updateDynamicText(juce::String& key)
{
    dynamicTextLabel.setText("Detected Key: " + key, juce::dontSendNotification);
}

void MidiMarkovEditor::handleNoteOn(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity)
{
    juce::MidiMessage msg1 = juce::MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);
    // DBG(msg1.getNoteNumber());
    audioProcessor.addMidi(msg1, 0);
    
}

void MidiMarkovEditor::handleNoteOff(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity)
{
    juce::MidiMessage msg2 = juce::MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity);
    audioProcessor.addMidi(msg2, 0); 
}


