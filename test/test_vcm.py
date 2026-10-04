#!/usr/bin/env python3
"""Functional smoke tests for the vcm VapourSynth plugin.

Usage: python3 test/test_vcm.py [path/to/libvcm.so]

Without arguments the plugin is expected to be autoloaded (e.g. from the
installed wheel). Every public function is called on small synthetic clips
in several formats; the test checks that the filters run, return clips of
the expected format and size, actually change the picture where they are
supposed to, and that unsupported input is rejected with an error instead
of a crash.

Requires the vapoursynth Python module and numpy.
"""
import sys

import numpy as np
import vapoursynth as vs

core = vs.core

WIDTH, HEIGHT, FRAMES = 128, 96, 4
SEED = 1234

failures = []


def check(cond, msg):
    if not cond:
        failures.append(msg)
        print("FAIL", msg)
    else:
        print("ok  ", msg)


def noise_clip(fmt):
    """A deterministic gradient + noise pattern in the requested format."""
    f = core.get_video_format(fmt)
    rng = np.random.default_rng(SEED)
    base = core.std.BlankClip(width=WIDTH, height=HEIGHT, length=FRAMES, format=fmt)
    planes = []
    for p in range(f.num_planes):
        w = WIDTH >> (f.subsampling_w if p else 0)
        h = HEIGHT >> (f.subsampling_h if p else 0)
        yy, xx = np.mgrid[0:h, 0:w]
        gradient = (xx / max(w - 1, 1) + yy / max(h - 1, 1)) / 2  # 0..1
        frames = []
        for n in range(FRAMES):
            img = gradient * 0.6 + 0.2 + rng.normal(0, 0.03, size=(h, w)) + 0.02 * np.sin(n)
            img = np.clip(img, 0, 1)
            if f.sample_type == vs.INTEGER:
                img = np.rint(img * ((1 << f.bits_per_sample) - 1)).astype(np.uint8 if f.bits_per_sample == 8 else np.uint16)
            else:
                if p and f.color_family == vs.YUV:
                    img = img - 0.5
                img = img.astype(np.float32)
            frames.append(img)
        planes.append(frames)

    def fill(n, f):
        f = f.copy()
        for p in range(f.format.num_planes):
            np.asarray(f[p])[:] = planes[p][n]
        return f

    return core.std.ModifyFrame(base, base, fill)


def to_arrays(clip, frames=None):
    out = []
    for n in range(clip.num_frames if frames is None else frames):
        fr = clip.get_frame(n)
        out.append([np.array(fr[p]) for p in range(fr.format.num_planes)])
    return out


def differs(a, b):
    return any(not np.array_equal(x, y) for fa, fb in zip(a, b) for x, y in zip(fa, fb))


def run(name, fn, src=None, same_size=True, changes=True, frames=2):
    """Run fn(), fetch some frames and compare with src."""
    try:
        clip = fn()
        out = to_arrays(clip, frames)
    except vs.Error as e:
        failures.append("%s raised: %s" % (name, str(e).strip().splitlines()[-1]))
        print("FAIL", failures[-1])
        return None
    msg = name
    ok = True
    if src is not None:
        if same_size:
            ok = ok and clip.format.id == src.format.id and clip.width == src.width and clip.height == src.height
            msg += " (format and size kept)"
            if changes:
                ok = ok and differs(out, to_arrays(src, frames))
                msg += " (output differs from input)"
        else:
            ok = ok and clip.format.id == src.format.id
            msg += " -> %dx%d" % (clip.width, clip.height)
    check(ok, msg)
    return clip


def expect_error(name, fn):
    try:
        fn().get_frame(0)
    except vs.Error:
        check(True, name + " is rejected")
    else:
        check(False, name + " was accepted")


def main():
    if len(sys.argv) > 1:
        core.std.LoadPlugin(sys.argv[1])
    v = core.vcm

    names = sorted(f for f in dir(v) if not f.startswith("_"))
    expected = ["Amp", "Bokeh", "Circles", "ColorBox", "DeBarrel", "DeJitter", "F1QClean", "F1QLimit",
                "F1Quiver", "F2QBlur", "F2QBokeh", "F2QCorr", "F2QLimit", "F2QSharp", "F2Quiver", "Fan",
                "Fisheye", "GBlur", "Grid", "Hist", "Jitter", "MBlur", "Mean", "Median", "Neural",
                "Pattern", "Reform", "Rotate", "SaltPepper", "StepFilter", "Variance", "Veed"]
    check(names == expected, "all %d functions are registered" % len(expected))

    for fmt in [vs.YUV444P8, vs.YUV444P16, vs.RGB24, vs.GRAY8, vs.YUV444PS, vs.YUV420P8]:
        f = core.get_video_format(fmt)
        tag = " " + f.name
        src = noise_clip(fmt)
        blurred = v.GBlur(src)
        is_rgb = f.color_family == vs.RGB
        is_yuv = f.color_family == vs.YUV
        is_gray = f.color_family == vs.GRAY
        subsampled = f.subsampling_w or f.subsampling_h

        # pixel modifying filters
        run("Amp" + tag, lambda: v.Amp(src, sh=[2, 2, 2], sm=[2, 2, 2]), src)
        run("Amp sclip" + tag, lambda: v.Amp(src, useclip=1, sclip=blurred, sh=[2, 2, 2], sm=[2, 2, 2]), src)
        run("Fan" + tag, lambda: v.Fan(src), src)
        if not is_rgb:
            run("Hist" + tag, lambda: v.Hist(src), src)
        if is_yuv:
            run("Hist clipm" + tag, lambda: v.Hist(src, clipm=blurred, type=2), src)
            run("Hist table" + tag, lambda: v.Hist(src, type=3, table=[10, 20, 50, 60, 90, 20]), src)
        run("Median" + tag, lambda: v.Median(src), src)
        run("GBlur" + tag, lambda: v.GBlur(src), src)
        run("MBlur" + tag, lambda: v.MBlur(src), src)
        run("Variance" + tag, lambda: v.Variance(src, lx=10, wd=40, ty=10, ht=40), src)
        run("SaltPepper" + tag, lambda: v.SaltPepper(src), src, changes=False)
        is_int = f.sample_type == vs.INTEGER
        run("Veed" + tag, lambda: v.Veed(src), src, changes=is_int)
        run("Mean" + tag, lambda: v.Mean(src), src, changes=is_int)
        run("Neural" + tag, lambda: v.Neural(src, tclip=blurred, iter=2, bestof=1, xpts=3, ypts=3), src, changes=False)

        # frequency domain filters
        run("F1Quiver" + tag, lambda: v.F1Quiver(src, filter=[0, 100, 200, 2]), src)
        run("F1Quiver test" + tag, lambda: v.F1Quiver(src, filter=[0, 100, 200, 2], test=1), src)
        run("F1QClean" + tag, lambda: v.F1QClean(src), src, changes=False)
        run("F1QLimit" + tag, lambda: v.F1QLimit(src, freqs=[100, 200]), src, changes=False)
        run("F2Quiver" + tag, lambda: v.F2Quiver(src, fspec=[1, 2, 100, 200, 2]), src)
        run("F2Quiver test" + tag, lambda: v.F2Quiver(src, fspec=[1, 2, 100, 200, 2], test=1), src)
        run("F2QLimit" + tag, lambda: v.F2QLimit(src, fspec=[20, 30, 10]), src, changes=False)
        run("F2QBlur" + tag, lambda: v.F2QBlur(src), src)
        run("F2QSharp" + tag, lambda: v.F2QSharp(src), src)
        run("F2QCorr" + tag, lambda: v.F2QCorr(src, blurred), src, same_size=False)
        if not subsampled:
            kw = dict(rgb=[1, 1, 1]) if is_rgb else {}
            run("F2QBokeh" + tag, lambda: v.F2QBokeh(src, blurred, **kw), src)
            run("Bokeh" + tag, lambda: v.Bokeh(src, blurred, **kw), src)

        # pixel moving filters
        if not subsampled:
            run("Rotate" + tag, lambda: v.Rotate(src, blurred, angle=15.0), src)
            run("DeBarrel" + tag, lambda: v.DeBarrel(src, abc=[0.1, 0.05, 0.01]), src)
            run("DeBarrel test" + tag, lambda: v.DeBarrel(src, abc=[0.1, 0.05, 0.01], test=1), src)
            run("Reform" + tag, lambda: v.Reform(src, blurred, rect=[0.1, 0.1, 0.9, 0.9],
                                                 quad=[0.1, 0.1, 0.9, 0.05, 0.95, 0.9, 0.05, 0.8]), src)
            run("Fisheye" + tag, lambda: v.Fisheye(src), src, same_size=False)
            run("Fisheye test" + tag, lambda: v.Fisheye(src, test=1), src)
            run("Jitter" + tag, lambda: v.Jitter(src), src)
            run("Jitter sine" + tag, lambda: v.Jitter(src, type=2), src)
            run("DeJitter" + tag, lambda: v.DeJitter(src), src, changes=False)

        # miscellaneous
        run("Grid" + tag, lambda: v.Grid(src), src)
        run("Pattern" + tag, lambda: v.Pattern(src), src)
        run("Pattern sine" + tag, lambda: v.Pattern(src, type=4), src)
        run("StepFilter" + tag, lambda: v.StepFilter(src), src, changes=False)
        run("Circles" + tag, lambda: v.Circles(src), src)

    # DeJitter moves rows that start with dark pixels back to the left edge: a clip whose rows
    # start 6 dark samples late must come back as the original (apart from the blackened row end),
    # with the default jmax as well as with an explicit one.
    for fmt in (vs.GRAY8, vs.YUV444P8, vs.YUV444P16, vs.GRAYS, vs.RGB24):
        f = core.get_video_format(fmt)
        name = f.name
        floor = 0.2 if f.sample_type == vs.FLOAT else int(0.2 * ((1 << f.bits_per_sample) - 1))
        # no dark samples in the planes DeJitter looks at, so every row start is found
        bright = core.std.Expr(noise_clip(fmt), ["x %s max" % floor] + ([""] * (f.num_planes - 1) if f.color_family == vs.YUV else []))
        late = core.std.Crop(core.std.AddBorders(bright, left=6), right=6)

        def restored(clip):
            a = core.std.Crop(clip, right=8)
            b = core.std.Crop(bright, right=8)
            return all(fr.props["PlaneStatsDiff"] == 0 for p in range(f.num_planes)
                       for fr in core.std.PlaneStats(a, b, plane=p).frames())

        check(not restored(late), "DeJitter %s: the shifted clip differs from the original" % name)
        check(restored(v.DeJitter(late, wsyn=0)), "DeJitter %s: default jmax restores the rows" % name)
        check(restored(v.DeJitter(late, jmax=20, wsyn=0)), "DeJitter %s: jmax=20 restores the rows" % name)

    # source filter
    box = run("ColorBox", lambda: v.ColorBox())
    check(box is not None and box.format.id == vs.YUV444P8 and box.width > 0, "ColorBox default is YUV444P8")
    box10 = run("ColorBox 10 bit", lambda: v.ColorBox(format=vs.YUV444P10))
    check(box10 is not None and box10.format.id == vs.YUV444P10, "ColorBox format argument is honoured")

    # unsupported input must be rejected, not crash
    half = core.std.BlankClip(width=WIDTH, height=HEIGHT, format=vs.YUV444PH)
    expect_error("Amp half float", lambda: v.Amp(half, sh=[1, 1, 1]))
    expect_error("GBlur ksize 4", lambda: v.GBlur(noise_clip(vs.YUV444P8), ksize=4))
    expect_error("F2Quiver without fspec", lambda: v.F2Quiver(noise_clip(vs.YUV444P8)))
    expect_error("ColorBox RGB", lambda: v.ColorBox(format=vs.RGB24))
    expect_error("Jitter subsampled", lambda: v.Jitter(noise_clip(vs.YUV420P8)))

    if failures:
        print("\n%d FAILURES:\n  %s" % (len(failures), "\n  ".join(failures)))
        raise SystemExit(1)
    print("\nall tests passed")


if __name__ == "__main__":
    main()
