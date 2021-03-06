# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased
### Added
- Server now sends a notification to the client when it detects that a track has disappeared or reappeared.
- Option to prevent the display from being turned off after a period of user inactivity. Windows-only for now.
- Music collection view: track highlighting.
- Track info dialog. Available as an option in context menus of the queue, the history, and the music collection.
- "Track info" button for the current track.
- The current track is now marked in the collection view.
- Dynamic mode preferences are now saved and restored for each user.
- The command-line remote now supports user authentication, both interactive and non-interactive.
- Commandline-remote: new commands "qdel" and "break".
- Commandline-remote: new options for printing help or version numbers.

### Changed
- Volume control now uses a logarithmic scale.
- Dynamic mode has been rewritten.
- "High-scored tracks wave" has been redesigned; track selection has improved, progress is shown, and it can now be stopped before it has completed.
- Full indexation will now notice when tracks have disappeared from their previously known location.
- Faster transitions between subsequent tracks.
- Breakpoints look better now.
- The track progress bar uses different colors now.
- Switched to millisecond precision for track lengths.
- Track lengths are now usually displayed like "03:44.7" instead of "00:03:44".
- The queue now has a size limit; the limit is currently set to 2 million items.
- Command-line remote: port number is now optional.
- Naming of log files was changed a bit to allow for better sorting.
- Log files now start with a line that contains the PMP version number.
- Timestamps in log files now have millisecond precision.
- Exit code is now written to the log.
- PMP now requires a C++14 compiler.
- PMP now requires Qt 5.8 or later.

### Fixed
- Tracks were sometimes wrongly marked as unavailable in the client.
- Server crashed when trying to play a track marked as unavailable while being in 'stopped' mode.
- Music collection change notifications were not transmitted correctly.
- The client was not always notified when the known length of a track changed.
- When starting the client its main window sometimes appeared outside the visible screen area.
- All broken commands in the command-line remote have been fixed.
- Off-by-one error in bounds check when moving an item in the queue could lead to a crash of the server.

## 0.0.6 - 2018-12-26
### Added
- The GUI-Remote will now respond to keyboard multimedia keys Play/Pause/Next when the window has focus.
- Music collection view: new columns Length and Album.
- Queue context-menu option "Duplicate".
- Queue context-menu option "Move to end".
- History view: support for dragging tracks from the history to the queue.
- History context-menu with options "Add to front of queue" and "Add to end of queue".
- Dynamic mode: new "Start high-scored tracks wave" action.
- Server will report a failed database initialization procedure to the client.
- Server now prints its version number and some copyright information at startup.
- Command-line remote is now able to shut down the server again.

### Changed
- Music collection view: improved sorting.
- Debug builds of the GUI Remote no longer have a console window by default on Windows, just like release builds.
- PMP now requires at least Qt version 5.7.

### Fixed
- Queue columns "Last heard" and "Score" could be empty because of a breakpoint somewhere in the queue.
- Tracks that were preloaded to our TEMP directory but then deleted by a TEMP folder cleanup failed to play.
- Server discovery did not always work properly, because the UDP sockets were not flushed after sending a datagram.
- Server discovery did not identify an IP as belonging to localhost properly. The fix for this problem will only work on Qt version 5.8 or higher; users of Qt 5.7 will still encounter this (very minor) problem.
- Stopped using the deprecated "qt5_use_modules" CMake macro.
