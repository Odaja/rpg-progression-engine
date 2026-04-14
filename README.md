# C++ RPG Character Engine

This is a backend progression engine I wrote in C++ to handle character stats, dynamic attribute scaling, and leveling for a fantasy RPG concept. 

I originally built this to test out non-linear progression models rather than using static, hardcoded level-up charts. It was also a great opportunity to practice writing clean, object-oriented modern C++.

## What it does (and how)
* **Object-Oriented Design:** I broke the system down into dedicated classes (`Character`, `AttributeSystem`, `LevelSystem`) to keep the data encapsulated and the logic cleanly separated.
* **Math-Based Scaling:** To prevent stats from scaling infinitely, the attribute windows (Min/Max) are calculated dynamically using logarithmic and polynomial math formulas.
* **Fast Lookups:** Because skills and jobs are dynamic, I used `std::unordered_map` to handle them, which gives $O(1)$ constant-time lookups.
* **Algorithm Optimization:** The leveling system relies on finding a character's "Top-K" skills. Instead of doing a full sort on the skill array every time (which is expensive), I implemented `std::nth_element` to solve it in $O(N)$ linear time.

## Tech Stack
* **Language:** C++ (C++17/C++20)
* **Standard Library:** `<unordered_map>`, `<algorithm>`, `<array>`, `<cmath>`

## How to run it locally
If you want to test the console simulator yourself, you just need a compiler that supports C++17.

1. Clone the repo:
   ```bash
   git clone [https://github.com/YourUsername/rpg-progression-engine.git](https://github.com/YourUsername/rpg-progression-engine.git)
