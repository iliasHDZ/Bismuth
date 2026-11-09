# Bismuth

<cr>**WARNING: This mod is still in the experimental stage and
is very buggy! Alot of levels will not display correctly. This mod
should be percieved as something to experiment with rather than
something to use in actual play.**</c>

A rendering optimization mod that does alot of the trigger
and visibility calculations on the GPU. Bismuth completely
replaces and recreates GD's object rendering from scratch.

Bismuth is **especially** effective with high object counts
and excessive move/rotate trigger logic.

Currently, this mod renders 2.1 levels best. Newer 2.2
features and visual quirks haven't been implemented yet.

Levels with great results:
- Peaceful
- A daydream journey
- Departure
- Slaughterhouse

Bismuth currently works best on medium to high-end GPUs.

For low-end GPUs like many **Intel GPUs**, the advantages
of Bismuth's culling system are way less and in fact can
be **slower** than vanilla. They may also crash sometimes!

This mod features **overdraw view**! A helpful feature for
creators to visualize overdraw. Since Bismuth is still unstable,
it is recommended to **disable Bismuth** in the **settings window**
when using overdraw view for creating. Overdraw view can be enabled
by pressing **F4** in-game or in the editor.

I am currently not accepting bugfixes as there is already alot
of broken stuff to be fixed first.

Join the Discord server for the latest nightly builds: https://discord.gg/B6H6ZVJx9C

Icon created by Saryu