# SPDX-License-Identifier: GPL-3.0-or-later

import json
import urllib.request
from typing import Any, Dict


def json_request(url: str, headers: Dict[str, str] | None = None, method: str = "GET", body: bytes | None = None) -> Any:
    request = urllib.request.Request(url, data=body, headers=headers or {}, method=method)
    with urllib.request.urlopen(request, timeout=20) as response:
        return json.loads(response.read().decode("utf-8"))


def base_url(profile: Dict[str, Any]) -> str:
    base = profile["base_url"].strip().rstrip("/")
    if "://" not in base:
        base = "http://" + base
    return base
