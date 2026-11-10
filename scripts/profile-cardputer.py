#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only Linux /proc sampler for a running LoFiBox and its audio children.

CPU percentages use one core = 100%; system CPU uses the whole machine = 100%.
RSS sums resident pages, including shared pages once per process. Processes that
start and exit entirely between samples cannot be accounted for by /proc polling.
No SSH credentials, command-line arguments or library contents are collected.
"""
import argparse
import json
import math
import os
from pathlib import Path
import platform
import statistics
import time


def read_text(path):
    try:
        return Path(path).read_text().strip()
    except (OSError, UnicodeError):
        return None


def processes():
    result = {}
    for path in Path('/proc').glob('[0-9]*/stat'):
        try:
            text = path.read_text()
            close = text.rindex(')')
            fields = text[close + 2:].split()
            result[int(path.parent.name)] = {
                'name': text[text.index('(') + 1:close],
                'ppid': int(fields[1]),
                'ticks': int(fields[11]) + int(fields[12]),
                'start': int(fields[19]),
                'rss_bytes': max(0, int(fields[21])) * os.sysconf('SC_PAGE_SIZE'),
            }
        except (OSError, ValueError, IndexError):
            continue
    return result


def descendants(table, root):
    selected = {root} if root in table else set()
    while True:
        extra = {pid for pid, proc in table.items() if proc['ppid'] in selected}
        expanded = selected | extra
        if expanded == selected:
            return selected
        selected = expanded


def cpu_counters():
    fields = Path('/proc/stat').read_text().splitlines()[0].split()[1:9]
    values = [int(value) for value in fields]
    return sum(values), values[3] + values[4]


def system_memory():
    values = {}
    for line in Path('/proc/meminfo').read_text().splitlines():
        fields = line.split()
        if fields[0] in ('MemTotal:', 'MemAvailable:', 'SwapTotal:', 'SwapFree:'):
            values[fields[0].rstrip(':') + '_bytes'] = int(fields[1]) * 1024
    return values


def device_files():
    paths = ['/sys/class/graphics/fb*/name', '/sys/class/graphics/fb*/virtual_size',
             '/sys/class/graphics/fb*/bits_per_pixel', '/sys/class/graphics/fb*/stride',
             '/sys/class/thermal/thermal_zone*/temp',
             '/sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq',
             '/proc/pressure/cpu', '/proc/pressure/memory', '/proc/pressure/io']
    return {str(path): read_text(path) for pattern in paths
            for path in Path('/').glob(pattern.lstrip('/'))}


def sample(root, before, after, elapsed, previous_cpu, current_cpu):
    members = descendants(after, root)
    details = []
    hz = os.sysconf('SC_CLK_TCK')
    for pid in sorted(members):
        proc = after[pid]
        previous = before.get(pid)
        ticks = max(0, proc['ticks'] - previous['ticks']) if previous and previous['start'] == proc['start'] else 0
        details.append({'pid': pid, 'name': proc['name'], 'rss_bytes': proc['rss_bytes'],
                        'cpu_percent': round(100 * ticks / hz / elapsed, 2)})
    total = current_cpu[0] - previous_cpu[0]
    idle = current_cpu[1] - previous_cpu[1]
    return {'tree_cpu_percent': round(sum(proc['cpu_percent'] for proc in details), 2),
            'tree_rss_bytes': sum(proc['rss_bytes'] for proc in details),
            'system_cpu_percent': round(100 * (total - idle) / total, 2) if total > 0 else None,
            'memory': system_memory(), 'processes': details, 'device': device_files()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', required=True, type=int, help='LoFiBox process ID (same user or root)')
    parser.add_argument('--duration', type=float, default=60)
    parser.add_argument('--interval', type=float, default=1)
    parser.add_argument('--output', type=Path, help='JSON output; otherwise stdout')
    args = parser.parse_args()
    if (args.pid <= 0 or not math.isfinite(args.duration) or not math.isfinite(args.interval)
            or args.duration <= 0 or args.interval < 0.1):
        parser.error('pid/duration must be positive and interval must be finite and >= 0.1 seconds')
    if platform.system() != 'Linux':
        parser.error('this tool requires Linux /proc')
    before = processes()
    if args.pid not in before:
        parser.error('target PID is unavailable or not readable')
    root_start = before[args.pid]['start']
    report = {'schema': 1, 'machine': platform.machine(), 'kernel': platform.release(),
              'os_release': read_text('/etc/os-release'), 'framebuffers': read_text('/proc/fb'),
              'cpu_count': os.cpu_count(), 'pid': args.pid, 'samples': [],
              'cpu_units': 'tree: one core=100%; system: all cores=100%',
              'rss_units': 'bytes; shared pages counted per process',
              'limitations': 'short-lived children between samples and first observed child CPU may be omitted'}
    previous_cpu = cpu_counters()
    started = previous_time = time.monotonic()
    try:
        while time.monotonic() < started + args.duration:
            time.sleep(min(args.interval, max(0, started + args.duration - time.monotonic())))
            after = processes()
            now = time.monotonic()
            current_cpu = cpu_counters()
            if args.pid not in after or after[args.pid]['start'] != root_start:
                report['target_exited'] = True
                break
            row = sample(args.pid, before, after, now - previous_time, previous_cpu, current_cpu)
            row['elapsed_seconds'] = round(now - started, 3)
            report['samples'].append(row)
            before, previous_time, previous_cpu = after, now, current_cpu
    except KeyboardInterrupt:
        report['interrupted'] = True
    rows = report['samples']
    if rows:
        report['summary'] = {
            'samples': len(rows),
            'tree_cpu_mean_percent': round(statistics.mean(row['tree_cpu_percent'] for row in rows), 2),
            'tree_cpu_max_percent': max(row['tree_cpu_percent'] for row in rows),
            'tree_rss_max_bytes': max(row['tree_rss_bytes'] for row in rows),
        }
    encoded = json.dumps(report, ensure_ascii=False, indent=2) + '\n'
    if args.output:
        args.output.write_text(encoded, encoding='utf-8')
    else:
        print(encoded, end='')


if __name__ == '__main__':
    main()
