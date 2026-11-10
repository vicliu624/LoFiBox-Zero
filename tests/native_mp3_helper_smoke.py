# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare PCM with FFmpeg and exercise the private producer's exec boundary."""
import array
import json
import math
import os
import pathlib
import signal
import struct
import subprocess
import sys
import tempfile
import time

helper = pathlib.Path(sys.argv[1]).resolve()
ffmpeg = pathlib.Path(sys.argv[2]).resolve()


def execute(args, **kwargs):
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15, **kwargs)
    assert result.returncode == 0, (args, result.returncode, result.stderr.decode(errors='replace'))
    return result.stdout


def decode_args(path, offset=0, paced=False):
    return (['-re'] if paced else []) + ['-hide_banner', '-loglevel', 'error', '-nostdin',
        '-ss', str(offset), '-i', str(path), '-vn', '-ac', '2', '-ar', '48000', '-f', 'f32le', '-']


def helper_args(path, offset=0, paced=False, fallback=None):
    return [str(helper), '--input', str(path), '--start', str(offset)] + ([] if paced else ['--unpaced']) + [
        '--', str(fallback or ffmpeg), *decode_args(path, offset, paced)]


def compare(actual, expected, label):
    assert len(actual) == len(expected) and len(actual) % 8 == 0, (label, len(actual), len(expected))
    a = array.array('f', actual)
    b = array.array('f', expected)
    assert a and all(math.isfinite(x) for x in a), label
    rms = math.sqrt(math.fsum(x*x for x in b) / len(b))
    error = math.sqrt(math.fsum((x-y)**2 for x, y in zip(a, b)) / len(b))
    assert rms > .001 and error / rms < .001, (label, rms, error)
    return error / rms


with tempfile.TemporaryDirectory(prefix='lofibox-native-mp3-') as temporary:
    root = pathlib.Path(temporary)
    sentinel = root / 'no-fallback'
    sentinel.write_text('#!/bin/sh\nexit 91\n')
    sentinel.chmod(0o755)
    report = {'pcm_relative_rms_error': {}}
    fixtures = {}
    for name, rate, channels, vbr in [('stereo-vbr', 44100, 2, True), ('stereo-cbr', 44100, 2, False),
                                     ('mono', 44100, 1, False), ('rate48', 48000, 2, False)]:
        path = root / (name + ' 空 格.mp3')
        wave = f'aevalsrc=0.12*sin(2*PI*440*t)|0.08*sin(2*PI*997*t):s={rate}:d=0.7'
        execute([str(ffmpeg), '-hide_banner', '-loglevel', 'error', '-nostdin', '-f', 'lavfi', '-i', wave,
                 '-ac', str(channels), '-c:a', 'libmp3lame', *(['-q:a', '2'] if vbr else ['-b:a', '192k']),
                 '-metadata', 'replaygain_track_gain=+6 dB', str(path)])
        fixtures[name] = path
        full = execute([str(ffmpeg), *decode_args(path)])
        for offset in (0, .2):
            expected = full[round(offset * 48000) * 8:] if name.startswith('stereo') else execute([str(ffmpeg), *decode_args(path, offset)])
            # Native-compatible inputs must decode without ever invoking FFmpeg.
            actual = execute(helper_args(path, offset, fallback=sentinel if name.startswith('stereo') else None))
            report['pcm_relative_rms_error'][f'{name}:{offset}'] = compare(actual, expected, f'{name}:{offset}')

    # Bounded metadata inspection must accept normal trailing tags, but keep
    # concatenated/missing-length streams on FFmpeg before any native PCM.
    tagged = root / 'trailing-tags.mp3'
    item = struct.pack('<II', 4, 0) + b'Title\0test'
    footer = b'APETAGEX' + struct.pack('<IIII', 2000, len(item) + 32, 1, 0x80000000) + bytes(8)
    header = bytearray(footer)
    header[23] |= 0x20
    tagged.write_bytes(fixtures['stereo-vbr'].read_bytes() + header + item + footer + b'TAG' + bytes(125))
    compare(execute(helper_args(tagged, fallback=sentinel)),
            execute([str(ffmpeg), *decode_args(tagged)]), 'APEv2 and ID3v1')
    for name in ('mono', 'rate48'):
        joined = root / ('joined-' + name + '.mp3')
        joined.write_bytes(fixtures['stereo-vbr'].read_bytes() + fixtures[name].read_bytes())
        fixtures['joined-' + name] = joined
    unindexed = root / 'without-xing.mp3'
    execute([str(ffmpeg), '-hide_banner', '-loglevel', 'error', '-i', str(fixtures['stereo-vbr']),
             '-c:a', 'copy', '-write_xing', '0', str(unindexed)])
    fixtures['without-xing'] = unindexed
    renamed = root / 'renamed-wave.mp3'
    execute([str(ffmpeg), '-hide_banner', '-loglevel', 'error', '-i', str(fixtures['stereo-vbr']),
             '-f', 'wav', str(renamed)])
    fixtures['renamed-wave'] = renamed

    # The fallback must replace this PID and inherit neither the MPEG input FD
    # nor another private descriptor. It then runs the exact original argv.
    wrapper = root / 'fallback'
    wrapper.write_text(f'#!{sys.executable}\nimport json,os,pathlib,sys\n'
        'fds={}\n'
        'for item in pathlib.Path("/proc/self/fd").iterdir():\n'
        ' try:fds[item.name]=os.readlink(item)\n'
        ' except FileNotFoundError:pass\n'
        'pathlib.Path(os.environ["FALLBACK_REPORT"]).write_text(json.dumps({"pid":os.getpid(),"fds":fds,"args":sys.argv[1:]}))\n'
        f'os.execv({str(ffmpeg)!r},[{str(ffmpeg)!r},*sys.argv[1:]])\n')
    wrapper.chmod(0o755)
    for name in ('mono', 'rate48', 'joined-mono', 'joined-rate48', 'without-xing', 'renamed-wave'):
        path = fixtures[name]
        destination = root / (name + '.json')
        args = helper_args(path, fallback=wrapper)
        with subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                env=dict(os.environ, FALLBACK_REPORT=str(destination))) as process:
            output, errors = process.communicate(timeout=15)
            assert process.returncode == 0, errors
            value = json.loads(destination.read_text())
            assert value['pid'] == process.pid, value
            assert all(int(fd) <= 2 for fd in value['fds']), value
            assert value['args'] == decode_args(path), value
        compare(output, execute([str(ffmpeg), *decode_args(path)]), 'exec fallback')

    before = len(os.listdir('/proc/self/fd'))
    # Use a longer, wide-band source and non-frame-aligned seeks. Compare a
    # full 48 kHz reference sliced in output frames, including its filter tail.
    # This catches rate/clock errors as well as lost resampling history.
    wide = root / 'wide-band.mp3'
    execute([str(ffmpeg), '-hide_banner', '-loglevel', 'error', '-nostdin', '-f', 'lavfi', '-i',
        'aevalsrc=0.1*sin(2*PI*440*t)+0.04*sin(2*PI*16123*t)|0.08*sin(2*PI*997*t)+0.03*sin(2*PI*13001*t):s=44100:d=7.933',
        '-c:a', 'libmp3lame', '-q:a', '2', str(wide)])
    reference_args = decode_args(wide)
    reference_args[-1:-1] = ['-af', 'aresample=resampler=soxr:precision=20']
    reference = execute([str(ffmpeg), *reference_args])
    assert abs(len(reference) / (48000 * 8) - 7.933) < 1 / 44100
    for offset in (0, .33331, 1.23457, 4.50001, 7.73123):
        expected = reference[round(offset * 48000) * 8:]
        actual = execute(helper_args(wide, offset, fallback=sentinel))
        report['pcm_relative_rms_error'][f'wide-band:{offset}'] = compare(actual, expected, f'wide-band:{offset}')
    report['pcm_output_rate_hz'] = 48000
    report['wide_band_duration_seconds'] = len(reference) / (48000 * 8)
    for index in range(16):
        execute(helper_args(fixtures['stereo-vbr'], fallback=sentinel))
    assert len(os.listdir('/proc/self/fd')) == before
    # Pacing must not disappear when pipe consumers are fast. Pausing this
    # producer must not make it emit the rest of the file in a catch-up burst.
    started = time.monotonic()
    with subprocess.Popen(helper_args(fixtures['stereo-vbr'], paced=True, fallback=sentinel),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE) as process:
        first = os.read(process.stdout.fileno(), 8192)
        assert first
        process.send_signal(signal.SIGSTOP)
        time.sleep(.25)
        resumed = time.monotonic()
        process.send_signal(signal.SIGCONT)
        tail, errors = process.communicate(timeout=5)
        assert process.returncode == 0, errors
        finished = time.monotonic()
    compare(first + tail, execute([str(ffmpeg), *decode_args(fixtures['stereo-vbr'])]), 'paced pause/resume')
    assert finished - resumed > .45, 'producer caught up instead of preserving playback pacing'
    report['paced_pause_total_seconds'] = round(finished - started, 3)
    for value in ('nan', 'inf', '1e308', '-1'):
        result = subprocess.run([str(helper), '--input', str(fixtures['stereo-vbr']), '--start', value,
            '--', str(sentinel)], capture_output=True, timeout=3)
        assert result.returncode == 2 and not result.stdout
    print(json.dumps(report))
