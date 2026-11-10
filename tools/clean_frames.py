"""Keep only the main dancer in each 64x128 frame (removes neighbouring poses
that were caught in the crop). Usage: python3 tools/clean_frames.py IN_DIR OUT_DIR

Method: find the dancer's body column (busiest column near the centre), walk
left/right until opacity drops to a valley (the gap between two dancers),
erase everything past it, then drop small disconnected scraps."""
import sys, os, glob
import numpy as np
from PIL import Image
from scipy import ndimage

def clean(im):
    a = np.array(im.convert("RGBA"))
    solid = a[..., 3] > 128
    prof = ndimage.uniform_filter1d(solid.sum(0).astype(float), 3)
    w = len(prof)
    c = 16 + int(np.argmax(prof[16:w-16]))          # body column
    peak = prof[c]
    def walk(step):
        x, best, bx = c, peak, c
        while 0 < x < w - 1:
            x += step
            if prof[x] < best: best, bx = prof[x], x
            if prof[x] <= 0.12 * peak: return x       # clear gap
            if prof[x] > best + 0.35 * peak and abs(x - c) > 14: return bx  # next dancer rising
        return x
    lo, hi = walk(-1), walk(+1)
    a[:, :lo, 3] = 0
    a[:, hi + 1:, 3] = 0
    # remove leftover scraps not connected to the body
    mask = a[..., 3] > 16
    lab, n = ndimage.label(mask)
    if n > 1:
        sizes = ndimage.sum(mask, lab, range(1, n + 1))
        keep = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s >= 0.08 * sizes.max()])
        a[~keep] = 0
    return Image.fromarray(a)

src, dst = sys.argv[1], sys.argv[2]
os.makedirs(dst, exist_ok=True)
for f in sorted(glob.glob(os.path.join(src, "*.png"))):
    clean(Image.open(f)).save(os.path.join(dst, os.path.basename(f)))
