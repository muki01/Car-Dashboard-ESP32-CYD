#!/usr/bin/env python3
"""
Builds and runs the CarCYD desktop simulator (headless, writes screenshots).

The simulator compiles LVGL, the portable firmware code (src/core, src/ui,
src/ui/fonts) and the files in this folder with a host C/C++ compiler.

Requirements:
    - a host compiler: `zig` (recommended, single download) or gcc/clang
    - the LVGL library (default: the Arduino library folder)
    - Pillow to convert the screenshots / build the media files

Usage:
    python tools/simulator/build.py                       # screenshots (English)
    python tools/simulator/build.py --lang tr             # screenshots (Turkish UI)
    python tools/simulator/build.py --demo --banner       # images/demo.gif + social-preview.png
    python tools/simulator/build.py --zig C:/tools/zig/zig.exe
    python tools/simulator/build.py --cc gcc --cxx g++

Output: images/screenshots/<lang>/*.png, images/screenshots/overview_<lang>.png, images/demo.gif,
        images/social-preview.png
"""

from __future__ import annotations

import argparse
import concurrent.futures as cf
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SKETCH = ROOT / "CarCYD"
BUILD = HERE / "build"
DEFAULT_LVGL = Path.home() / "Documents" / "Arduino" / "libraries" / "lvgl"


def find_compilers(opts) -> tuple[list[str], list[str]]:
    if opts.cc and opts.cxx:
        return [opts.cc], [opts.cxx]
    zig = opts.zig or os.environ.get("ZIG") or shutil.which("zig")
    if zig:
        return [zig, "cc"], [zig, "c++"]
    for cc, cxx in (("clang", "clang++"), ("gcc", "g++")):
        if shutil.which(cc) and shutil.which(cxx):
            return [cc], [cxx]
    sys.exit("error: no host compiler found (install zig, clang or gcc, or pass --zig/--cc/--cxx)")


def sources(lvgl: Path) -> list[Path]:
    files = sorted((lvgl / "src").rglob("*.c"))
    files += sorted((SKETCH / "src" / "ui" / "fonts").glob("*.c"))
    files += sorted((SKETCH / "src" / "core").glob("*.cpp"))
    files += sorted((SKETCH / "src" / "ui").rglob("*.cpp"))
    files += sorted(HERE.glob("*.cpp"))
    return files


def object_path(src: Path) -> Path:
    try:
        rel = src.relative_to(ROOT)
        tag = "app"
    except ValueError:
        rel = Path(*src.parts[-4:])
        tag = "lvgl"
    return BUILD / "obj" / tag / rel.with_suffix(rel.suffix + ".o")


def parse_depfile(text: str) -> list[str]:
    """Prerequisites of a Makefile style .d file (handles 'C:' drives and '\\ ' escaped spaces)."""
    text = text.replace("\\\r\n", " ").replace("\\\n", " ")
    sep = text.find(": ")  # a drive letter colon is never followed by a space
    body = text[sep + 2:] if sep >= 0 else ""
    deps, current, i = [], "", 0
    while i < len(body):
        c = body[i]
        if c == "\\" and i + 1 < len(body) and body[i + 1] == " ":
            current += " "
            i += 2
            continue
        if c.isspace():
            if current:
                deps.append(current)
            current = ""
        else:
            current += c
        i += 1
    if current:
        deps.append(current)
    return deps


def up_to_date(obj: Path) -> bool:
    dep = obj.with_suffix(".d")
    if not obj.exists() or not dep.exists():
        return False
    stamp = obj.stat().st_mtime
    deps = parse_depfile(dep.read_text(errors="ignore"))
    if not deps:
        return False
    for d in deps:
        p = Path(d)
        if not p.exists() or p.stat().st_mtime > stamp:
            return False
    return True


def compile_one(src: Path, cc, cxx, includes) -> tuple[Path, str | None]:
    obj = object_path(src)
    if up_to_date(obj):
        return obj, None
    obj.parent.mkdir(parents=True, exist_ok=True)
    is_cpp = src.suffix == ".cpp"
    cmd = (cxx if is_cpp else cc) + [
        "-c", str(src), "-o", str(obj), "-MMD", "-MF", str(obj.with_suffix(".d")),
        "-O1", "-g0", "-w" if "lvgl" in obj.parts else "-Wall",
        "-Wno-date-time",  # __DATE__ is shown on the About page
        "-std=gnu++17" if is_cpp else "-std=gnu11",
    ] + includes
    if is_cpp:
        cmd += ["-fno-exceptions", "-fno-rtti"]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return obj, f"{src}:\n{result.stdout}{result.stderr}"
    if result.stderr.strip():
        print(result.stderr.strip())
    return obj, None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--lvgl", type=Path, default=Path(os.environ.get("LVGL_DIR", DEFAULT_LVGL)))
    parser.add_argument("--zig")
    parser.add_argument("--cc")
    parser.add_argument("--cxx")
    parser.add_argument("--lang", choices=["en", "tr"], default="en")
    parser.add_argument("--out", type=Path, default=ROOT / "images" / "screenshots")
    parser.add_argument("--scale", type=int, default=2, help="PNG scale factor (nearest neighbour)")
    parser.add_argument("--demo", action="store_true", help="record the animated tour -> images/demo.gif")
    parser.add_argument("--banner", action="store_true", help="render images/social-preview.png from the screenshots")
    parser.add_argument("--no-run", action="store_true")
    opts = parser.parse_args()

    if opts.banner and not opts.demo:
        from media import make_banner
        make_banner(ROOT / "images" / "screenshots" / "en", ROOT / "images" / "social-preview.png")
        return 0

    if not (opts.lvgl / "lvgl.h").exists():
        sys.exit(f"error: LVGL not found at {opts.lvgl} (use --lvgl)")
    cc, cxx = find_compilers(opts)
    # lv_conf.h is picked up from the sketch folder exactly like on the device.
    includes = ["-I", str(SKETCH), "-I", str(opts.lvgl / "src"), "-I", str(opts.lvgl)]

    srcs = sources(opts.lvgl)
    print(f"compiling {len(srcs)} files with {' '.join(cc)}")
    errors = []
    objects = []
    with cf.ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        for obj, err in pool.map(lambda s: compile_one(s, cc, cxx, includes), srcs):
            objects.append(obj)
            if err:
                errors.append(err)
    if errors:
        print("\n".join(errors[:10]))
        return 1

    exe = BUILD / ("carcyd_sim.exe" if os.name == "nt" else "carcyd_sim")
    # Response file: the object list exceeds the Windows command line limit.
    rsp = BUILD / "objects.rsp"
    rsp.write_text("\n".join('"' + o.as_posix() + '"' for o in objects), encoding="utf-8")
    result = subprocess.run(cxx + ["-o", str(exe), f"@{rsp}"], capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stdout, result.stderr)
        return 1
    print(f"built {exe.relative_to(ROOT)}")
    if opts.no_run:
        return 0

    if opts.demo:
        frames = BUILD / "frames"
        shutil.rmtree(frames, ignore_errors=True)
        frames.mkdir(parents=True)
        run = subprocess.run([str(exe), str(frames), opts.lang, "demo"])
        if run.returncode != 0:
            return run.returncode
        from media import make_banner, make_demo_gif
        make_demo_gif(frames, ROOT / "images" / "demo.gif", scale=opts.scale)
        if opts.banner:
            make_banner(ROOT / "images" / "screenshots" / "en", ROOT / "images" / "social-preview.png")
        return 0

    opts.out.mkdir(parents=True, exist_ok=True)
    run = subprocess.run([str(exe), str(opts.out), opts.lang])
    if run.returncode != 0:
        return run.returncode
    return export_screenshots(opts.out, opts.lang, opts.scale)


def export_screenshots(out: Path, lang: str, scale: int) -> int:
    """Converts the simulator's PPM frames to PNG (<out>/<lang>/) and builds an overview sheet."""
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        print("Pillow not installed (pip install pillow): screenshots left as .ppm")
        return 0

    lang_dir = out / lang
    lang_dir.mkdir(parents=True, exist_ok=True)
    for old in lang_dir.glob("*.png"):
        old.unlink()

    shots = []
    for ppm in sorted(out.glob(f"{lang}_*.ppm")):
        name = ppm.stem[len(lang) + 1:]
        img = Image.open(ppm).convert("RGB")
        ppm.unlink()
        shots.append((name, img))
        if scale > 1:  # nearest neighbour keeps the pixels exactly as on the device
            img = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
        img.save(lang_dir / f"{name}.png")

    # Overview sheet: all screens at device resolution with captions.
    cols, pad, caption_h = 4, 12, 26
    w, h = 320, 240
    rows = (len(shots) + cols - 1) // cols
    sheet = Image.new("RGB", (pad + cols * (w + pad), pad + rows * (h + caption_h + pad)), (20, 23, 28))
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.load_default(size=15)
    except TypeError:
        font = ImageFont.load_default()
    for i, (name, img) in enumerate(shots):
        x = pad + (i % cols) * (w + pad)
        y = pad + (i // cols) * (h + caption_h + pad)
        sheet.paste(img, (x, y))
        draw.text((x + 2, y + h + 5), name.replace("_", " "), fill=(170, 180, 195), font=font)
    sheet.save(out / f"overview_{lang}.png")
    print(f"{len(shots)} screenshots -> {lang_dir}  (overview: overview_{lang}.png)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
