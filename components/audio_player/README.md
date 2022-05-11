# Audio player component for esp32

## Capabilities

* MP3 decoding (via libhelix-mp3)
* Wav/wave file decoding

## Who is this for?

Decode only audio playback on esp32 where the features and footprint of esp-adf are not necessary.

## What about esp-adf?

This component is not intended to compete with esp-adf, a much more fully developed
audio framework.

It does however have a number of advantages at the moment including:

* Fully open source (esp-adf has a number of binary modules at the moment)
* Minimal size (it's less capable, but also simpler, than esp-adf)

## Tests

Unity tests are implemented in the test/ folder.
