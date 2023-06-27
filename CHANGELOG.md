# Changelog

## Unreleased

---

## 1.5 - 2023-06-27

### Added
- New pokemons from Scarlet and Violet
- QR Code generation that can be used to open a web page with the pokemon's data

### Changed
- Sprites are now scrapped from a much more reliable source, but they are now 96x96 pixels instead of 64x64 pixels
- Refactor Main Pokemon form to accomodate the new size of the pokemon sprites
- Scrapper script completely rewritten in Go instead of Python

### Fixed
- The clear button in the main form now is responsive to the current screen orientation
- The back button in the Pokemon form now is responsive to the current screen orientation
- The back button in the Type Chart form now is responsive to the current screen orientation
- And probably other small things...

---

## 1.4 - 2023-02-16

This release adds some requested quality of life improvements:

- Remember the search query
- Remember last selected Pokémon in the list
- Scroll the Pokémon list with hard buttons and/or Jog Dial
- Added a button the clear the search and reset the filters
- When on Pokémon detail form, scroll thru adjacent mons using the hard buttons
- On Sony devices with a Jog containing the "back" button, pressing it when on the Pokémon Detail/Type form, will not close the app. Now it get's back to the main list
- Bumped search limit to 64 results
- Make the "Back to the list" button an icon, instead of a text button
- Added the "Back to the list" button to the Pokémon Type form as well
- There is no need to reinstall the SpritePack if you are updating from V1.3.

---

## 1.3 - 2023-01-09

So, DmitryGR offered some patches, and now this app has really grown into something special! Here is the changelog:


- Dedicated sprites for each screen mode. Install only the sprites that your device is capable of - handling! No more wasting space.
- Support for every version of Palm OS, from 1.0 thru 6.1! On Palm OS 1.0 and 2.0, when keeping the - pencil down on the Pokemon image, it will be displayed in full grayscale if corresponding spritepack is - installed, which is super neat, as those devices never supported grayscale!
- Support for High Resolution screens, including Sony's OS4, Palm OS and HandEra versions. Added new - pokemon images for that, and corresponding type badges as well.
- ACI imagery. That's a new compressor made exclusively for this app that is specialized in compressing - pokemon images, and decompressing them on the 68k platform. Assembly code was added to quickly decode - it on Palm OS leading to huge space savings and stellar performance.
- Improved performance all over the board. Searching pokemons are faster, and so is opening them. Thanks - to ACI imagery, pokemon images loads almost instantly on every device!
- Support for various screen sizes. Expand the app UI on devices that have a digital input area! Or even - run it on a Dana for no reason, it's supported!
- Added missing pokemon sprites.
- And various under-the-hood improvements that made the app faster and smaller.


Enjoy!

---

## 1.2 - 2022-12-17

With this release, all Pokemon sprites are stored as PNGs! This means that the sprites size was reduced from 3464K to 606K!

You ***will*** need to upgrade to the new version of the Sprites `.prc` file if you have them installed for this version to work. 

Changelog:
- The sprites are now stored as PNGs
- Ported [MiniZ](https://github.com/richgel999/miniz) and [Pngle](https://github.com/kikuchan/pngle) to PalmOS to decode the PNGs (Thanks for the help Dmitry & others!)
- Pokemon number is now shown in the main list
- Remove search button from main form
- Better error handling 
- Better error messages on unsupported devices

---

## 1.1
- Initial (public) release! Enjoy!

---

## 1.0
- Initial (private) release