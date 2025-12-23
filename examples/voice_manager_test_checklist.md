# Voice Manager Panel - Manual Test Checklist

This document provides a comprehensive manual testing checklist for the Voice Manager Panel
Qt Multimedia integration as specified in [Issue #1](https://github.com/VisageDvachevsky/nvmdnv/issues/1).

## Prerequisites

- NovelMind Editor built with Qt6 Multimedia support
- Audio files for testing (mp3, wav, ogg formats recommended)
- Project with Scripts folder containing .nms dialogue files
- Project Assets/Voice folder with voice files

## Test Cases

### A. Build / Dependencies

- [ ] Editor builds successfully in Debug configuration
- [ ] Editor builds successfully in Release configuration
- [ ] No new compiler warnings introduced
- [ ] Qt6::Multimedia module links correctly

### B. Playback Engine

#### Basic Playback
- [ ] Select a voice entry with matched file → Click Play → Audio plays correctly
- [ ] Volume slider changes audio volume in real-time
- [ ] Stop button stops playback immediately
- [ ] Progress bar shows correct playback position
- [ ] Duration label shows correct format (mm:ss.s)

#### Track Switching
- [ ] Playing new track stops previous track correctly
- [ ] Quick play/stop clicks don't cause crashes
- [ ] Selecting new line during playback stops current audio

#### State Management
- [ ] Play button disabled during playback
- [ ] Stop button enabled during playback
- [ ] UI resets correctly after playback ends
- [ ] UI resets correctly after manual stop

### C. UI/UX Correctness

#### Progress Bar
- [ ] Range matches audio duration (in ms)
- [ ] Progress updates smoothly during playback
- [ ] Progress resets to 0 on stop
- [ ] Progress resets to 0 when track ends

#### Duration Label
- [ ] Format is consistent (mm:ss.s)
- [ ] Shows "0:00 / 0:00" when no audio selected
- [ ] Shows position/duration during playback
- [ ] Updates in real-time

#### Error Handling
- [ ] Non-existent file shows error message
- [ ] Unsupported format shows error message
- [ ] Error message auto-clears after 3 seconds
- [ ] Status bar returns to normal after error

### D. Duration Scanning

#### Async Probing
- [ ] Duration probing doesn't freeze UI
- [ ] Large folders scan without visible freeze
- [ ] Duration values appear in list after scan

#### Caching
- [ ] Re-scan is faster (uses cached values)
- [ ] Modified files get re-probed
- [ ] Cache clears on full re-scan

### E. File Format Support

Test with each format where platform supports it:

- [ ] MP3 files play correctly
- [ ] WAV files play correctly
- [ ] OGG files play correctly
- [ ] FLAC files play correctly (if supported)

### F. Edge Cases

- [ ] Empty voice folder doesn't crash
- [ ] No project open doesn't crash
- [ ] Deleting file during playback shows error, doesn't crash
- [ ] Very short files (< 1s) play correctly
- [ ] Very long files play correctly
- [ ] Files with special characters in name play correctly

### G. Memory & Resource Management

- [ ] No memory leaks after repeated play/stop cycles
- [ ] Audio resources released on panel shutdown
- [ ] No zombie processes after editor close

## Test Procedure

1. Open NovelMind Editor
2. Create or open a project with voice files
3. Navigate to Voice Manager panel
4. Click "Scan" to populate the list
5. Run through each test case above
6. Note any failures with steps to reproduce

## Notes

- Test on both Windows and Linux if available
- Focus on UI responsiveness during operations
- Report any crashes with full stack trace if possible
