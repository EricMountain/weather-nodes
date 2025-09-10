# Moon phases font

* Downloaded from [dafont](https://www.dafont.com/moon-phases.font)
* Got [otf2bdf from AUR](https://aur.archlinux.org/packages/otf2bdf), makepkg etc
* `cp moon_phases.ttf ~/.local/share/fonts` and open LibreOffice to try it out
* Convert to BDF:
  * `otf2bdf -p 24 moon_phases.ttf -o moon_phases.bdf`
  * `otf2bdf -p 48 moon_phases.ttf -o moon_phases_48.bdf`
* Convert to C using [u8g2’s](https://github.com/olikraus/u8g2) bdfconv:
  * `u8g2/tools/font/bdfconv/bdfconv -f 1 -m '32-255' -n moon_phases -o ../../src/fonts/moon_phases.h moon_phases.bdf`
  * `u8g2/tools/font/bdfconv/bdfconv -f 1 -m '48,65-90' -n moon_phases_48pt -o ../../src/fonts/moon_phases_48pt.h moon_phases.bdf`
