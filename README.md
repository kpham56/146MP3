# CMPE146 Mp3 player
## by Kevin Pham, Maurice Washington, Johnny Nguyen

| Part        | usage           |
| ------------- |:-------------:|
| SJ2 Board     | right-aligned |
| Mikroe Clic mp3 decoder| centered|
| Sparkfun 16 x 2 lcd | are neat| 
| cherry mx green switches |control|
| Anker powerbank | power |

---

Project was built over the span of 6 weeks. Parts were mainly ordered from sparkfun or amazon. Construction was through a cardboard box, soldering, duct tape and dreams. ![mp3box](https://user-images.githubusercontent.com/52023760/169568227-be9b632f-c555-4695-b1c8-0bcb44db94c7.jpg)

![mp3box2](https://user-images.githubusercontent.com/52023760/169568835-8b5e0f9f-d60f-4752-b8e4-bff785544bde.jpg)

---

## usage
Device has 4 buttons
Up button is used to increment the song, volume, bass or treble depending on the mode
Down button is used to decrement the song, volume, bass or treble depending on the mode selected
Mode button is used to cycle through song, volume, bass, or treble mode
Frowny face is used to pause, play, or send the next song to play

## coding
This device programmed onto the SJ2 board. Coded in C using FreeRtos and built using scons. Device features 4 main tasks, and has about 50% cpu usage while playing.




