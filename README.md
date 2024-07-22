# MarkovImproviser
MIDI improviser plugin based on a variable order Markov model.

## Author
Lelio Casale

## Abstract
The project is a Markov model based improviser which can generate note sequences that resemble the same style of the input.

## Description
The plugin takes as an input a sequence of MIDI note, either played in real-time or from an external MIDI file, and converts is into a variable order Markov model.
The user can then generate notes based on this model, save all the desired models, load an external model and eventually add a randomization to the output.

## Implemented features
1. A Markov model that is the base of the audio plugin. It can learn in real time input MIDI sequences or the training can be done using a pre-recorded MIDI file. The trained Markov model is capable of generating notes sequences that resemble the style of the training ones.
2. Two toggles: one can be used to give to the user the possibility to stop the training process, such that the plugin doesn't learn MIDI sequences when the toggle is set to off, while the other is used to start the notes generation process based on the learned sequences when it is set to on.
3. A reset button that completely cleans the Markov Model such that the Markov model has no memory about the older input sequences.
4. Two functions to load and save learned models to keep track of any musically interesting model and to eventually restart the plugin directly from the saved model without needing to re-train it.
5. A key detector that displays the key and mode based on the MIDI data. This feature works both for real-time learned models and for models loaded from a previously saved file. This information is then used by the randomness selector to choose which random notes can be played and which not.
6. A randomness selector which is used to give some variation to the Markov model output: the user can select a randomness value in percentage which represents the number of notes in percentage that need to be randomized.

## How to use it
You can build this project using CMake. You have to clone this repository, setup the CMake.txt file with the right dependencies based on your folder configurations and set the right Juce location on your local machine. Then build it by the command line interface using CMake: first open the terminal with admin rights, then use the command "cmake .." and after that run "cmake --build . --config Release". You will find the .vst3 file of the plugin in the "VST3" folder contained in the "Release" folder.

## Technologies used
1. Juce
2. C++
3. CMake

## Report
'Improviser_Report.pdf'

## Presentation Video
[![Watch the video](https://drive.google.com/file/d/1BJ5qhqEYFdj-J91e_yI-TEbJwSifioy8/view?usp=drive_link)
