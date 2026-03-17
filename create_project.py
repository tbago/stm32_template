#!/usr/bin/env python3

from __future__ import annotations

import re
import shutil
import sys
from pathlib import Path


def usage() -> None:
    print("Usage:")
    print("  python3 create_project.py <project_name>")
    print()
    print("Description:")
    print("  Create a new STM32 project by copying the template directory.")


def validate_project_name(project_name: str) -> None:
    if not re.fullmatch(r"[A-Za-z0-9_-]+", project_name):
        raise ValueError("project name must contain only letters, numbers, '_' or '-'.")


def rewrite_text(path: Path, project_name: str) -> None:
    text = path.read_text()
    text = text.replace("project(template C ASM)", f"project({project_name} C ASM)")
    text = text.replace("cd template", f"cd {project_name}")
    text = text.replace("template/build/debug/", f"{project_name}/build/debug/")
    text = text.replace("template/build/release/", f"{project_name}/build/release/")
    path.write_text(text)


def ignore_copy(_src: str, names: list[str]) -> set[str]:
    ignored = {"build", "__pycache__"}
    return {name for name in names if name in ignored}


def main() -> int:
    if len(sys.argv) != 2:
        usage()
        return 1

    project_name = sys.argv[1]

    try:
        validate_project_name(project_name)
    except ValueError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    template_dir = Path(__file__).resolve().parent
    target_dir = template_dir.parent / project_name

    if target_dir.exists():
        print(f"Error: target already exists: {target_dir}", file=sys.stderr)
        return 1

    shutil.copytree(template_dir, target_dir, ignore=ignore_copy)
    (target_dir / "create_project.py").unlink()
    (target_dir / "README.md").rename(target_dir / "template.md")

    rewrite_text(target_dir / "CMakeLists.txt", project_name)
    rewrite_text(target_dir / "template.md", project_name)

    print(f"Created project: {target_dir}")
    print("Next steps:")
    print(f"  cd {project_name}")
    print("  cmake --preset debug")
    print("  cmake --build --preset debug")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
