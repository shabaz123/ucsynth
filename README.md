# ucsynth

Implements a 4-operator synthesizer. The code as-is runs on Linux, but is adaptable for microcontrollers, because it uses a low amount of resources (i.e. it was coded specifically to be ported to microcontrollers, because there are already far better synthesizers for Linux!).

The code sets up the operators as shown here:

<img width="100%" align="left" src="doc\synth-alg.jpg">

[For more information see this blog](https://community.element14.com/challenges-projects/design-challenges/bluetoothunleashed/b/blog/posts/smart-doorbell-system-part-8-fm-sound-synthesizer-and-xds110-debugger-and-tag-connect-adaptor)

To use the code, type:

    make
    ./synth

A file called out.wav will be generated, which plays a simple tune. The purpose of the tune is just to demonstrate that the 4 operators were set up to produce a woodwind instrument type of sound.


