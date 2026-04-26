<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Media Backends

Normative media pipeline rules live in:

- `docs/specification/lofibox-zero-media-pipeline-spec.md`
- `docs/specification/lofibox-zero-audio-dsp-spec.md`

## Pipeline

The intended pipeline is:

```text
Source -> Decode -> DSP Chain -> Volume -> Output
```

## Decode And Metadata

Format support should be provided through distribution-packaged media backends such as FFmpeg or GStreamer, with tag handling through governed metadata dependencies such as TagLib where appropriate.

The application must not implement format support as scattered `if mp3`, `if flac`, `if m4a` UI logic.

Metadata writeback is currently mediated by the packaged `tag_writer.py` helper, using system Python and Debian's `python3-mutagen` package. This is a runtime host-adapter detail: UI and playback code must depend on the `TagWriter` service boundary, not on Python, Mutagen, or helper script paths.

Remote media helper execution is mediated by the packaged `remote_media_tool.py` helper. Its current implementation uses Python standard-library networking and JSON modules only; any future non-stdlib dependency must be admitted through the dependency policy and represented in Debian packaging.

## DSP

DSP is a chain, not an EQ page special case.
The chain should support:

- EQ
- biquad filters
- ReplayGain
- limiter
- loudness
- future DSP nodes

## Test Output

Tests must support `NullAudioOutput` so they do not depend on real audio hardware.
