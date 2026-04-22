Image morpher

TODO:
~~1. Investigate and implement subWindow coords to be reshape automatically (save coefficients of scaling)~~ Done
~~2. Invsetigate image size vs catmull subdivision dependence (on large images we see peaks at small morph radius, why doesnt this happen on small images?)~~ Solved by increasing subdivision
~~3. Implement tube morphing~~ Effect is doubtable, increased subdivion mostly solved the problem
4. Research mesh normals rotation, and/or should it be z-coord elevation (how high to elevate? ) Looks very promising. zero height does not conflict with hill climbing and descent
