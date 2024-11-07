README

this library drives three hardware features on the HexBoard
in order of priority:
 1) output audio to the piezo buzzer and audio jack
 2) input from the rotary knob
 3) input from hexagon buttons, driven by a multiplexer

a single timer controls all three background processes
on whichever core the setup_timers() routine is called.
 
each tick, the library takes turns audio processing 
and input processing. effectively, the audio has a
sample rate of 1,000,000Hz / (2 * frequency_poll in uS).

frequency_poll should be a factor of 500,000 so that 
the resulting audio sample rate will be an integer.

 poll uS 10 16    20 25 32     40   50
 sr  kHz 50 31.25 25 20 16.125 12.5 10

the hexagon buttons are grouped in 10 columns, each
associated with one GPIO pin. a multiplexer with
16 states (2^4, using 4 binary pins) filters which
button to drive on the column pin. the hex routine
cycles through each column pin, reading the input
of the pin after changing the multiplexer to each
of its valid states. it takes about 10 uS for the
electronics of the multiplexer to do its work.
the routine uses the clock cycles to time the
multiplexer write and the column pin read, which
avoids calling a code-blocking delay() function.
the player feels lag of 16*10*2*frequency_poll.
generally, 10ms or longer is "sluggish."

 poll uS 10   16    20   25 32    40   50
 lag  ms  3.2  5.12  6.4  8 10.24 12.8 16

the rotary pin sends two digital signals that work
in tandem to communicate whether someone turned 
the rotary knob, and which direction. to do this,
the pins blink (go off->on->off) in phase, with
the direction indicated by which pin blinked first.
the rotary routine watches for the correct sequence
and then queues each successful turn in a buffer.
in this way, if the user spins the knob very quickly,
multiple turns can be registered instantly.
the knob can also be pressed in the middle. the
routine tracks the duration of each press so that
the developer can discern a held-press from a quick one.
empirically, each of the four steps in the knob sequence
(off/off -> on/off -> on/on -> off/on -> off/off)
takes 500 - 800 uS, so as long as the rotary sequence
occurs at least this often, it should not miss any spins.
