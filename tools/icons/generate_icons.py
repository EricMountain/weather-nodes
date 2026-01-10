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
FAVICON_SIZES = (16, 32, 48)

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


def get_icon(cache: dict[int, Image.Image], size: int) -> Image.Image:
    if size not in cache:
        cache[size] = render_svg(size)
    return cache[size]


def write_pngs(cache: dict[int, Image.Image]) -> None:
    for size in ICON_SIZES:
        icon = get_icon(cache, size)
        out_path = ASSETS_DIR / f'icon-{size}.png'
        icon.save(out_path, optimize=True)
        print(f'Wrote {out_path} ({size}x{size})')


def write_favicon(cache: dict[int, Image.Image]) -> None:
    favicon_path = ASSETS_DIR / 'favicon.ico'
    base_size = max(FAVICON_SIZES)
    base_icon = get_icon(cache, base_size)
    sizes = [(size, size) for size in sorted(set(FAVICON_SIZES))]
    base_icon.save(favicon_path, format='ICO', sizes=sizes)
    print(f'Wrote {favicon_path} ({"/".join(str(s) for s in FAVICON_SIZES)} px)')


def main() -> None:
    cache: dict[int, Image.Image] = {}
    write_pngs(cache)
    write_favicon(cache)


if __name__ == '__main__':
    main()
