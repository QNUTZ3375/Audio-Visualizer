# Audio Visualizer

A cross-platform audio visualizer that reacts to music in real-time using C++, OpenGL, ImGui, miniaudio, and more.

![Screenshot 2025-06-01 at 1 47 53 PM](https://github.com/user-attachments/assets/5d1de309-db5a-4e33-8cde-f4a65a0e6f0b)

## Features:
- Real-time audio visualization
- Amplitude and Frequency visualization
- Supports .wav, .mp3, .flac, and .ogg
- Cross-platform (Windows/Mac/Linux)
- Simple and customizable UI

## Dependencies:
- C++11
- FFTW
- portable-file-dialogs
- ImGui
- miniaudio
- OpenGL version 3.3 Core
- GLFW 3.4
- GLAD

## Language Limitations
This app does not support Unicode or non-ASCII characters in its UI or file paths. All displayed text and file names must be composed of English (ASCII) characters. Internationalized or localized content is not currently supported.

## Build Instructions:
git clone https://github.com/QNUTZ3375/audio-visualizer.git

cd audio-visualizer

make

./Application


## Usage:
Launch the app, select an audio file, and run it by pressing SPACEBAR

## Controls:
- Start/Stop a track: SPACEBAR
- Open a new audio file: CTRL+O/CMD+O (Track must be paused)
- Switch Theme: TAB
- All controls can be performed by clicking the on-screen icons as well
