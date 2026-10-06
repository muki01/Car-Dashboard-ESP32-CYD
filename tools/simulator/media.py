#!/usr/bin/env python3
"""
Marketing media for the README, generated from real simulator output:

    make_demo_gif()  - animated product tour (images/demo.gif)
    make_banner()    - 1280x640 GitHub social preview image (images/social-preview.png)

Requires Pillow. Called by build.py (--demo / --banner).
"""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[2]
FONT_DIR = ROOT / "tools" / "fonts" / "ttf"

ACCENT = (31, 203, 242)
PCB_YELLOW = (240, 190, 40)
TEXT = (233, 238, 244)
TEXT_DIM = (139, 151, 167)


def _font(name: str, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(FONT_DIR / name), size)


# ---- Demo animation -------------------------------------------------------------------------

def _read_manifest(frames_dir: Path) -> dict[int, tuple[int, int]]:
    """frame index -> touch position for frames where the screen is pressed."""
    touches = {}
    manifest = frames_dir / "frames.txt"
    if manifest.exists():
        for line in manifest.read_text().splitlines():
            index, pressed, x, y = (int(v) for v in line.split())
            if pressed:
                touches[index] = (x, y)
    return touches


def make_demo_gif(frames_dir: Path, out: Path, scale: int = 2, frame_ms: int = 100) -> Path:
    frames_paths = sorted(frames_dir.glob("frame_*.ppm"))
    if not frames_paths:
        raise SystemExit(f"no frames in {frames_dir}")
    touches = _read_manifest(frames_dir)

    frames = []
    for i, path in enumerate(frames_paths):
        img = Image.open(path).convert("RGB")
        if scale > 1:
            img = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
        if i in touches:  # visualise the finger so the viewer can follow the tour
            x, y = (v * scale for v in touches[i])
            overlay = Image.new("RGBA", img.size, (0, 0, 0, 0))
            d = ImageDraw.Draw(overlay)
            r = 13 * scale
            d.ellipse((x - r, y - r, x + r, y + r), fill=(255, 255, 255, 90), outline=(255, 255, 255, 220),
                      width=2 * scale)
            img = Image.alpha_composite(img.convert("RGBA"), overlay).convert("RGB")
        frames.append(img)

    # One shared palette keeps colours stable between frames and lets the GIF
    # encoder store only the changed rectangle of each frame.
    sample = Image.new("RGB", (frames[0].width, frames[0].height * 4))
    for k, idx in enumerate(range(0, len(frames), max(1, len(frames) // 4))[:4]):
        sample.paste(frames[idx], (0, k * frames[0].height))
    palette = sample.quantize(colors=255, method=Image.Quantize.MEDIANCUT)
    indexed = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]

    out.parent.mkdir(parents=True, exist_ok=True)
    durations = [frame_ms] * len(indexed)
    durations[-1] = 1500  # pause before the loop restarts
    indexed[0].save(out, save_all=True, append_images=indexed[1:], duration=durations, loop=0,
                    optimize=False, disposal=1)
    print(f"demo animation: {out.relative_to(ROOT)} ({len(indexed)} frames, {out.stat().st_size // 1024} KB)")
    return out


# ---- Hero / social preview banner -----------------------------------------------------------

def _rounded(size: tuple[int, int], radius: int, fill) -> Image.Image:
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    ImageDraw.Draw(img).rounded_rectangle((0, 0, size[0] - 1, size[1] - 1), radius, fill=fill)
    return img


def _device(screen: Image.Image) -> Image.Image:
    """Stylised ESP32-2432S028R: yellow PCB, black bezel, the real UI screenshot."""
    sw, sh = 560, 420
    screen = screen.resize((sw, sh), Image.LANCZOS)
    pcb_w, pcb_h = sw + 96, sh + 64
    device = _rounded((pcb_w, pcb_h), 22, PCB_YELLOW + (255,))
    d = ImageDraw.Draw(device)
    for hx, hy in ((18, 18), (pcb_w - 18, 18), (18, pcb_h - 18), (pcb_w - 18, pcb_h - 18)):
        d.ellipse((hx - 9, hy - 9, hx + 9, hy + 9), fill=(40, 34, 20, 255))  # mounting holes
    bezel = _rounded((sw + 20, sh + 20), 10, (8, 10, 13, 255))
    device.alpha_composite(bezel, (38, 22))
    device.paste(screen, (48, 32))
    # Silkscreen label in the bottom PCB margin (below the bezel).
    d.text((pcb_w // 2, pcb_h - 5), "ESP32-2432S028R", font=_font("BarlowSemiCondensed-SemiBold.ttf", 14),
           fill=(110, 86, 22, 255), anchor="ms")
    return device


def make_banner(screens_dir: Path, out: Path) -> Path:
    width, height = 1280, 640
    banner = Image.new("RGB", (width, height), (6, 8, 12))

    # Vertical gradient + soft accent glow behind the device.
    grad = Image.new("RGB", (1, height))
    for y in range(height):
        t = y / (height - 1)
        grad.putpixel((0, y), (int(12 - 6 * t), int(17 - 9 * t), int(26 - 14 * t)))
    banner.paste(grad.resize((width, height)))
    glow = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    ImageDraw.Draw(glow).ellipse((620, 70, 1260, 590), fill=ACCENT + (70,))
    banner = Image.alpha_composite(banner.convert("RGBA"), glow.filter(ImageFilter.GaussianBlur(90)))

    # Device with drop shadow, slightly overlapping the right edge area.
    device = _device(Image.open(screens_dir / "03_dashboard_shift.png").convert("RGB"))
    shadow = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    dx, dy = width - device.width - 36, (height - device.height) // 2
    ImageDraw.Draw(shadow).rounded_rectangle((dx + 10, dy + 22, dx + device.width + 10, dy + device.height + 22),
                                             24, fill=(0, 0, 0, 170))
    banner = Image.alpha_composite(banner, shadow.filter(ImageFilter.GaussianBlur(18)))
    banner.alpha_composite(device, (dx, dy))

    d = ImageDraw.Draw(banner)
    x = 64
    max_x = dx - 28  # text column must not touch the device

    def fitted(text: str, name: str, size: int) -> ImageFont.FreeTypeFont:
        font = _font(name, size)
        while d.textlength(text, font=font) > max_x - x and size > 10:
            size -= 1
            font = _font(name, size)
        return font

    d.text((x, 112), "OPEN SOURCE  ·  ESP32  ·  LVGL 9", font=_font("BarlowSemiCondensed-SemiBold.ttf", 24),
           fill=ACCENT)
    d.text((x - 4, 142), "CarCYD", font=_font("BarlowSemiCondensed-SemiBold.ttf", 118), fill=TEXT)
    subtitle = ["Car dashboard & OBD-II display", "for the ESP32 Cheap Yellow Display"]
    sub_font = fitted(max(subtitle, key=len), "BarlowSemiCondensed-Medium.ttf", 38)
    for i, line in enumerate(subtitle):
        d.text((x, 284 + i * 44), line, font=sub_font, fill=(214, 221, 230))

    chips = ["Gauges", "Live data", "DTC read / clear", "Trip computer"]
    chip_font = _font("BarlowSemiCondensed-SemiBold.ttf", 22)
    cx, cy = x, 392
    for chip in chips:
        w = int(d.textlength(chip, font=chip_font)) + 30
        if cx + w > max_x:  # wrap to the next row
            cx, cy = x, cy + 52
        d.rounded_rectangle((cx, cy, cx + w, cy + 40), 20, outline=ACCENT, width=2, fill=(10, 30, 40))
        d.text((cx + w // 2, cy + 20), chip, font=chip_font, fill=TEXT, anchor="mm")
        cx += w + 10
    footer = "Arduino IDE ready  ·  320×240 touch  ·  ESP32-2432S028R"
    d.text((x, 540), footer, font=fitted(footer, "BarlowSemiCondensed-Medium.ttf", 22), fill=TEXT_DIM)

    out.parent.mkdir(parents=True, exist_ok=True)
    banner.convert("RGB").save(out, optimize=True)
    print(f"banner: {out.relative_to(ROOT)} ({out.stat().st_size // 1024} KB)")
    return out
