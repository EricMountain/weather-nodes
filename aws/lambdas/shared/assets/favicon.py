"""Helpers to serve the binary favicon asset."""

from __future__ import annotations

import base64
from importlib import resources

ASSET_PACKAGE = "assets"
FAVICON_FILENAME = "favicon.ico"


def _read_favicon_bytes() -> bytes:
    favicon_path = resources.files(ASSET_PACKAGE).joinpath(FAVICON_FILENAME)
    return favicon_path.read_bytes()


def get_favicon_base64() -> str:
    """Return the favicon encoded as base64 for Lambda binary responses."""
    return base64.b64encode(_read_favicon_bytes()).decode("ascii")
