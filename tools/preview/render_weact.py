"""Render the actual WeAct layout and exercise its decoder on the host.
Needs gcc/g++, Pillow, Adafruit GFX and pngle. Set ADAFRUIT_GFX_LIB and
PNGLE_LIB to the installed libraries' root and src directories respectively.
"""
import os
from pathlib import Path
import subprocess
from PIL import Image

HERE = Path(__file__).resolve().parent
OUT = HERE / 'out' / 'weact'
OUT.mkdir(parents=True, exist_ok=True)
GFX = Path(os.environ.get('ADAFRUIT_GFX_LIB', '~/Documents/Arduino/libraries/Adafruit_GFX_Library')).expanduser()
PNG = Path(os.environ.get('PNGLE_LIB', '~/Documents/Arduino/libraries/pngle/src')).expanduser()
flags = ['-DARDUINO=100', '-DWEACT_HOST_PREVIEW=1', '-I' + str(HERE / 'weact_stubs'),
         '-I' + str(GFX), '-I' + str(PNG)]
objects = []
for name in ('pngle.c', 'miniz.c'):
    obj = OUT / (name + '.o')
    subprocess.run(['gcc', '-O2', '-c', str(PNG / name), '-o', str(obj)], check=True)
    objects.append(str(obj))
exe = OUT / 'preview'
subprocess.run(['g++', '-std=c++17', '-O2', *flags, str(HERE / 'weact.cpp'),
                str(GFX / 'Adafruit_GFX.cpp'), *objects, '-o', str(exe)], check=True)
# Small PNG models the preferred plate_N_small.png member on the printer.
with Image.open(HERE / 'plate_1.png') as image:
    image.thumbnail((128, 128))
    image.save(OUT / 'sample.png')
subprocess.run([str(exe), str(OUT / 'sample.png')], cwd=HERE, check=True)
for ppm in OUT.glob('*.ppm'):
    with Image.open(ppm) as image:
        image.save(ppm.with_suffix('.png'))
print('Rendered PNGs:', OUT)
