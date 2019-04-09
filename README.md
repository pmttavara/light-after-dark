# Light After Dark
This is the source code for [Light After Dark v2](http://philliptrudeau.itch.io/light-after-dark), first created for the [2017 GMTK Game Jam](https://itch.io/jam/gmtk-jam/rate/159828). Working in a three-person team, I was the programmer and so responsible for implementing game mechanics the team conceived, as well as rendering, asset pipeline, particles, physics, and more.
On July 21, 2018 I pushed a full-featured update with new & improved music, sound effects, particle effects, and credits.
This source repo does not contain graphics (not owned by me), so it is mostly a showcase of the code. Due to initially being a game jam game, much more content was hardcoded into the source than is usual of game engines, which helped the team meet the jam deadline.

(The music is mine, but without the graphics I saw no reason to include the raw music files.)

### Building
For posterity, and in case the assets are released or added in the future and/or someone creates their own:
Building this program on Windows involves calling `project.bat build` in the Command Prompt.	
To run the build on Windows, launch the resulting .exe. To run on Unix, launch the .exe in [Wine](https://www.winehq.org/).
For release builds, call `project.bat build release`.
To preprocess assets into a header file, call `project.bat metagen`.
For "final" builds, remember to call `project.bat metagen release final` before calling `project.bat build release final`.
The build script was imported from a more general project script, so the .exe name will be the name of the folder in which the `.bat` file is present.

### License
Source code is copyright Phillip Trudeau-Tavara. All rights reserved.
