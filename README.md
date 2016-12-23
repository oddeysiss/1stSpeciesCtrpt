# 1stSpeciesCtrpt
First species counterpoint generator

This is my C++ program designed to compose a first species counterpoint randomly with any given music key (A, B, C#, etc.).
Running the program creates a .csd file titled counterpoint.csd which you need CSound installed to play. You can download CSound
at http://csound.github.io/download.html.
My main goal in writing this was to generate a random composition that fit the constraints of a first species counterpoint, which
is a Baroque-era style of composition with strict guidelines. If you couldn't tell, I'm a bit of a music nerd. Some of the more
challenging aspects of this project were formatting musical notes in such a way that I could automatically identify musical
intervals and implementing a backtracking algorithm to write the counterpoint melody after my original implementation failed to
meet all constraints. I plan on adding minor key tonality soon!

Enjoy!
-Mitchell
