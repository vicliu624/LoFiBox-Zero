#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later

from pathlib import Path
import sys

sys.dont_write_bytecode = True


def _add_import_roots() -> None:
    script_dir = Path(__file__).resolve().parent
    sys.path.insert(0, str(script_dir))
    source_root = script_dir.parent / "src"
    if source_root.exists():
        sys.path.insert(0, str(source_root))


def main() -> int:
    _add_import_roots()
    from remote.tooling.remote_media_dispatcher import main as dispatch_main

    return dispatch_main(sys.argv)


if __name__ == "__main__":
    raise SystemExit(main())
