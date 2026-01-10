#!/usr/bin/env python3

import io
from pathlib import Path

from cairosvg import svg2png
from PIL import Image


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parents[1]
SVG_SOURCE = SCRIPT_DIR / 'temperature-weather-svgrepo-com-mod.svg'
ASSETS_DIR = PROJECT_ROOT / 'aws' / 'lambdas' / 'shared' / 'assets'
ICON_SIZES = (192, 512)

ASSETS_DIR.mkdir(parents=True, exist_ok=True)


def render_svg(size: int) -> Image.Image:
    if not SVG_SOURCE.exists():
        raise FileNotFoundError(f'Missing SVG source at {SVG_SOURCE}')

    png_bytes = svg2png(
        url=str(SVG_SOURCE),
        output_width=size,
        output_height=size,
        background_color='transparent',
    )
    return Image.open(io.BytesIO(png_bytes)).convert('RGBA')


def main() -> None:
    for size in ICON_SIZES:
        icon = render_svg(size)
        out_path = ASSETS_DIR / f'icon-{size}.png'
        icon.save(out_path, optimize=True)
        print(f'Wrote {out_path} ({size}x{size})')


if __name__ == '__main__':
    main()
