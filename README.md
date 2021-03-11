# Background: Skulls of Sedlec
[Skulls of Sedlec](https://boardgamegeek.com/boardgame/303553/skulls-sedlec) is a card game that revolves around drawing, choosing and positioning cards into a certain shape, in order to maximize their total score.

Scoring, TLDR:
1) Each Peasant gives 1 point;
2) Each Adjacent Pair of Lovers gives 6 points;
3) Each Assassin gives 2 points if adjacent to a Priest
4) Each Priest gives 2 points per row (other Priests on same row are ignored);
5) Each Hangman gives 1 point, +1 point for all connected Assassins;
6) Each Guard gives 1 point, + 1 point for all Royals beneath it;
7) Each Royal gives +1 point for all Royals/Villagers beneath it;

Classically, the cards are drawn in turns and placed into a pyramid.
* For >2 players, this pyramid has a base-width of 3 cards, and a height of 3 cards ("3x3");
* For 2 players, this pyramid has a base-width of 4 cards, and a height of 3 cards (flat top) ("4x3");

# Program
This small program was made to find the highest possible score for any given pyramid.

For the 3x3 and 4x3 pyramids, this task is trivially solved by brute-forcing. However, as the amount of permutations grows rapidly, a 7x7 pyramid would already require 132,626,429,906,095,529,318,154,240,000,000 iterations to solve.

Instead, this program uses the following pseudo-evolution-algorithm for finding the optimal layout:
1) Generate a random layout of cards; regenerate it several thousand times in order to find a good candidate;
2) "Evolve" the layout by swapping two cards within it (always find and make the optimal swap); keep evolving for as long as it increases the score;
3) "Mutate" the layout by swapping several cards at the same time (including cards previously not in the layout); keep mutating until a layout with higher score is found;
4) "Evolve" again (small, optimal swaps);
5) "Mutate" again (wider, chaotic swaps);
6) Rinse and repeat for as long as the score keeps increasing; if we reach a dead end, but there is a hint that higher scores are possible, regenerate the layout and restart from top;
7) If no such hint exists, assume we have found the optimal solution and save it;

This process is ran in parallel on every available thread.

# Results

**TLDR:   
For the standard 3x3 and 4x3 pyramids, the maximum scores are 35 and 69 respectively.**

## 3x3 Pyramid. Max score: 35
Example:   
<img src="https://raw.githubusercontent.com/Variszangt/SkullSolver/master/pics/3x3.png" width="300"/>   
Guidelines:   
* Don't use Royals;
* Only 1 Peasant/Guard should appear as a filler;
* Only 1 Pair of Lovers might appear;
* Focus on the Assassin+Priest+Hangman combo;

## 4x3 Pyramid. Max score: 61
Example:   
<img src="https://raw.githubusercontent.com/Variszangt/SkullSolver/master/pics/4x3.png" width="300"/>
Guidelines:   
* Don't use Royals;
* Only a few peasants/guards should appear as a filler;
* Only a few pairs of lovers might appear (preferrably at the top of the pyramid);
* Focus on the Assassin+Priest+Hangman combo;

## 4x4 Pyramid. Max score: 69
Example:   
<img src="https://raw.githubusercontent.com/Variszangt/SkullSolver/master/pics/4x4.png" width="300"/>   
Guidelines (for all larger pyramids):   
* Royals and Guards a very top;
* Use Peasants/Guards as filler at bottom;
* Use Lovers as filler near edges (Lovers are generally outranked by other cards, don't focus much on them);
* Focus on a huge Assassin+Priest+Hangman combo in middle!

## 5x5 Pyramid. Max score: 125

## 6x6 Pyramid. Max score: 183

## 7x7 Pyramid. Max score: 244
* Given 30 cards, this is the biggest pyramid (28 cards) that could be possibly be built.   
* Probably the maximum? Not 100% sure. TODO: check again when I'm smarter.

Example:   
<img src="https://raw.githubusercontent.com/Variszangt/SkullSolver/master/pics/7x7.png" width="300"/>
