"""Shared helpers for serving PWA assets."""
from __future__ import annotations

import base64
import json
from importlib import resources
from typing import Dict

ASSET_PACKAGE = "assets"
ICON_FILES: Dict[int, str] = {
    192: "icon-192.png",
    512: "icon-512.png",
}

DEFAULT_THEME_COLOR = "#667eea"
DEFAULT_BACKGROUND_COLOR = "#667eea"


def _read_icon_bytes(size: int) -> bytes:
    filename = ICON_FILES.get(size)
    if not filename:
        raise ValueError(f"No icon available for size {size}px")

    icon_path = resources.files(ASSET_PACKAGE).joinpath(filename)
    return icon_path.read_bytes()


def get_icon_base64(size: int) -> str:
    """Return the requested icon encoded as base64 for Lambda binary responses."""
    return base64.b64encode(_read_icon_bytes(size)).decode("ascii")


def build_manifest(name: str, short_name: str | None = None, start_path: str = "/") -> str:
    """Generate a Web App Manifest JSON string."""
    if not start_path:
        start_path = "/"
    if not start_path.startswith("/"):
        start_path = f"/{start_path}"

    manifest = {
        "name": name,
        "short_name": short_name or name,
        "start_url": start_path,
        "scope": "/",
        "display": "standalone",
        "orientation": "portrait-primary",
        "background_color": DEFAULT_BACKGROUND_COLOR,
        "theme_color": DEFAULT_THEME_COLOR,
        "icons": [
            {
                "src": f"icon-{size}.png",
                "sizes": f"{size}x{size}",
                "type": "image/png",
                "purpose": "any",
            }
            for size in ICON_FILES
        ],
    }

    return json.dumps(manifest)