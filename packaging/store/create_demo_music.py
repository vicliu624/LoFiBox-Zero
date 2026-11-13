# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate original demo MP3s on the development host; never play or upload them."""
from __future__ import annotations

import argparse
import array
import hashlib
import json
import math
from pathlib import Path
import random
import shutil
import subprocess
import sys
import wave


def render_cycle(rate: int, bpm: int, chords: list[list[float]], seed: int) -> tuple[array.array, float, float]:
    random_source = random.Random(seed)
    count = round(16 * 60 / bpm * rate)
    samples = array.array('h')
    peak = energy = 0.0
    previous_noise = 0.0
    for index in range(count):
        t = index / rate
        beat = t * bpm / 60
        position = (beat % 1) * 60 / bpm
        half_position = ((beat * 2) % 1) * 30 / bpm
        chord = chords[min(3, int(beat // 4))]
        chord_position = (beat % 4) * 60 / bpm
        attack = min(1.0, chord_position / 0.035)
        pad = 0.018 * attack * sum(math.sin(2 * math.pi * note * t) for note in chord)
        bass = 0.075 * min(1, position / 0.008) * math.exp(-5 * position) * math.sin(2 * math.pi * chord[0] / 2 * t)
        noise = random_source.uniform(-1, 1)
        hat = 0.018 * (noise - previous_noise) * math.exp(-75 * half_position)
        previous_noise = noise
        if int(beat) % 2 == 0:
            drum = 0.13 * math.exp(-23 * position) * math.sin(2 * math.pi * (50 * position + 1.4 * (1 - math.exp(-25 * position))))
        else:
            drum = 0.045 * noise * math.exp(-24 * position)
        edge = min(1.0, t / 0.012, (count - index - 1) / (rate * 0.012))
        left = (pad + bass + drum + hat) * max(0, edge)
        right = (pad * 0.98 + bass + drum + hat * 0.95) * max(0, edge)
        for value in (left, right):
            assert math.isfinite(value) and abs(value) < 0.45
            peak = max(peak, abs(value))
            energy += value * value
            samples.append(round(value * 32767))
    return samples, peak, math.sqrt(energy / (count * 2))


def write_track(output: Path, title: str, bpm: int, chords: list[list[float]], seed: int,
                index: int, seconds: int, cover: Path, ffmpeg: str) -> dict:
    from mutagen.id3 import APIC, ID3, TXXX, USLT

    rate = 44100
    cycle, peak, rms = render_cycle(rate, bpm, chords, seed)
    if sys.byteorder != 'little':
        cycle.byteswap()
    data = cycle.tobytes()
    destination = output / (title.lower().replace(' ', '-') + '.mp3')
    wav_path = destination.with_suffix('.wav')
    # Fixed chunks bound host memory use. No audio device is opened.
    with wave.open(str(wav_path), 'wb') as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(rate)
        remaining = seconds * rate * 4
        while remaining:
            block = data[:min(remaining, len(data))]
            wav.writeframesraw(block)
            remaining -= len(block)
    subprocess.run([ffmpeg, '-hide_banner', '-loglevel', 'error', '-y', '-i', str(wav_path),
                    '-af', f'afade=t=in:st=0:d=0.04,afade=t=out:st={seconds - 1}:d=1',
                    '-c:a', 'libmp3lame', '-q:a', '2', '-id3v2_version', '3',
                    '-metadata', 'title=' + title, '-metadata', 'artist=LoFiBox Demo',
                    '-metadata', 'album=First Launch', '-metadata', 'album_artist=LoFiBox Demo',
                    '-metadata', 'composer=LoFiBox Demo', '-metadata', 'genre=Lo-fi',
                    '-metadata', f'track={index}/2', str(destination)], check=True)
    tags = ID3(destination)
    tags.add(APIC(encoding=3, mime='image/png', type=3, desc='LoFiBox', data=cover.read_bytes()))
    lyrics = (f'[00:00.00]{title.upper()}\n[00:06.00]An original LoFiBox demo\n'
              '[00:12.00]A quiet loop and a steady pulse\n[00:20.00]Music continues when the screen sleeps\n'
              '[00:28.00]Press a key to return\n[00:36.00]LoFiBox - First Launch\n'
              '[00:48.00]An original instrumental demo\n')
    tags.add(USLT(encoding=3, lang='eng', desc='', text=lyrics))
    tags.add(TXXX(encoding=3, desc='SYNCEDLYRICS', text=[lyrics]))
    tags.save(destination, v2_version=3)
    # The player reads the explicit synced tag; retain an LRC review copy too.
    destination.with_suffix('.lrc').write_text(lyrics, encoding='utf-8')
    wav_path.unlink()
    return {'file': destination.name, 'title': title, 'bpm': bpm, 'source_rate_hz': rate,
            'channels': 2, 'seconds': seconds, 'lyrics_sidecar': destination.with_suffix('.lrc').name,
            'source_peak': round(peak, 6),
            'source_rms': round(rms, 6), 'sha256': hashlib.sha256(destination.read_bytes()).hexdigest()}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seconds', type=int, default=60, choices=range(60, 121))
    args = parser.parse_args()
    ffmpeg = shutil.which('ffmpeg')
    if not ffmpeg:
        parser.error('ffmpeg is required on the development host')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    cover = Path(__file__).resolve().parents[2] / 'assets/ui/icons/legacy-lofibox/logo.png'
    sequences = [
        ('Night Circuit', 84, [[130.81, 164.81, 196, 246.94], [110, 130.81, 164.81, 196],
                              [146.83, 174.61, 220, 261.63], [98, 123.47, 146.83, 174.61]], 173),
        ('Morning Light', 96, [[174.61, 220, 261.63, 329.63], [130.81, 164.81, 196, 246.94],
                              [146.83, 174.61, 220, 261.63], [98, 123.47, 146.83, 174.61]], 271),
    ]
    tracks = [write_track(output, title, bpm, chords, seed, index, args.seconds, cover, ffmpeg)
              for index, (title, bpm, chords, seed) in enumerate(sequences, 1)]
    report = {'provenance': 'Original locally synthesized instrumentals; no personal library audio or artwork.',
              'cover': 'Existing project LoFiBox logo, embedded without alteration.',
              'tracks': tracks}
    (output / 'provenance.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
