# Bismuth mod

<cr>**WARNING: This mod is still in the experimental stage and
very buggy! Alot of levels will not display correctly. This mod
should be percieved as something to experiment with rather than
something to use in actual gameplay.**</c>

A rendering optimization mod that serves to make levels draw
faster while also doing some of the trigger logic on the GPU.

This mod is also useful for creators as it comes with the
overdraw visualizer which will help creators make their
levels run better.

For high end devices, the biggest bottleneck is usually
trigger and color logic that's done on the CPU. Having
to apply move, rotate and scale effects on many objects
while also changing color can be a drag for the CPU.
Which is why Bismuth does this processing on the GPU
instead.

For low end decives, the biggest bottleneck is drawing
all those objects on screen. Having to write so many
pixels can sometimes be alot of work on their small
GPUs. Bismuth serves to reduce the amount of pixels
to draw mainly by using advanced techniques that
reduce overdraw.

The time it takes to process a frame is directly correlated
to input latency. With Bismuth making the game faster, it
could potentially make the game more reactive.

I am currently not accepting bugfixes as there is already alot
of broken stuff to be fixed first.

2.2 levels currently don't work correctly.

Icon created by Saryu