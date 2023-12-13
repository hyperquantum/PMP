# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased
### Added
- Scrobbling to Last.fm.
- Waiting spinner shown while loading the music collection.
- Queue presence indicator in the music collection.
- Music collection: ability to have three filters at the same time.
- Music collection: new filter criteria "in the queue" and "not in the queue".
- Music collection: new filter criterium "no longer available".
- Music collection: new filter criteria "heard at least once" and "with score".
- Music collection: new filter criteria "score < 50" and "score ≥ 80".
- Music collection: new filter criterium "not heard in the last year".
- Music collection: new filter criterium "not heard in the last 2 years".
- Music collection: new filter criterium "not heard in the last 3 years".
- Music collection: new filter criterium "not heard in the last 5 years".
- Music collection: new filter criteria "without title", "without artist", and "without album".
- Music collection: display count of tracks shown.
- Support for album artist.
- User statistics display switch.
- Track info dialog: user for track statistics can now be chosen.
- Dynamic mode parameters: add "12 weeks" to repetition avoidance setting.
- Command-line remote: new commands "serverversion", "personalmode", "publicmode", "dynamicmode" and "status".
- Command-line remote: new commands "history" and "trackhistory".
- Command-line remote: the "insert" command can now insert a track into the queue.
- Hash tool: print album and album artist.
- License file is now part of the release archive.
- Run unit tests in GitHub Actions.

### Changed
- Music collection: change criterium "score ≤ 30" to "score < 30".
- Music collection: change criterium "length ≤ 1 min." to "length < 1 min."
- Server will now refuse to insert a track into the queue if the hash is not familiar.
- Hash tool: all fields printed are now aligned.
- Hash tool: log to file instead of console.
- Console logging: prints date and time instead of just time.
- Release archive for Windows renamed from "PMP_win64.zip" to "PMP-win64.zip".
- PMP now requires CMake 3.21 or later.

### Removed
- Music collection: remove criterium "not heard in the last 1000 days".
- Music collection: remove criterium "not heard in the last 365 days".

### Fixed
- Incorrect hash if APEv2 tag present.
- Incorrect score and last heard for the public user.
- Client crash when receiving updated track data.

## 0.2.0 - 2022-11-15
### Added
- Delayed start.
- Ability to reload server settings.
- Ability to name a server instance.
- Setting for database port number.
- "Scan for servers" button in the server selection screen.
- Server can now find a file after it has been moved or renamed. Only occurs when the server is looking for a specific file.
- Client is now notified when the server discovers that a track has disappeared or reappeared.
- Client is now notified when the server discovers that the length of a track has changed.
- Full indexation now detects when a file has been moved or deleted.
- Music collection view: track filter and track highlighting.
- Track info dialog. Available as an option in context menus of the queue, the history, and the music collection.
- "Track info" button for the current track.
- Ability to display time remaining for the track that is playing.
- Ability to insert a breakpoint at any location in the queue.
- Ability to insert a barrier into the queue. Barriers are like breakpoints, but are not consumed by the player.
- Music collection view now displays a marker at the current track.
- Dynamic mode preferences are now saved and restored for each user.
- Option to prevent the display from being turned off after a period of user inactivity. Windows-only for now.
- Command-line remote: support for user authentication, both interactive and non-interactive.
- Command-line remote: new commands "insert", "qdel", "break", "delayedstart", "reloadserversettings", and "trackstats".
- Command-line remote: new options for printing help or version numbers.
- Server: write status of initial full indexation to the console.

### Changed
- Volume control now uses a logarithmic scale.
- Dynamic mode has been rewritten.
- Dynamic mode settings have been moved to a separate dialog.
- "High-scored tracks wave" has been redesigned; track selection has improved, progress is shown, and it can now be stopped before it has completed.
- High-scored tracks wave is now called high-scored tracks mode.
- High-scored tracks mode will now give up if it cannot find enough tracks that fit the criteria.
- Faster transitions between consecutive tracks.
- Breakpoints look better.
- Track progress bar uses different colors.
- Track lengths: use millisecond precision.
- Track lengths: only display number of hours if at least one hour long.
- Track lengths: show at least one decimal: "03:44.7".
- Queue drop indicator is now a line between tracks.
- Column "prev. heard" in the queue now uses relative times, and displays the actual time in a tooltip when hovering.
- Improved positioning of the main window at startup.
- Dates/times displayed in the client now take the clock difference between client and server into account.
- Queue now has a size limit; the limit is currently set to 2 million items.
- Command-line remote: output of "queue" command has changed a little bit.
- Command-line remote: usage text is now more extensive.
- Command-line remote: port number is now optional.
- Naming of log files was changed a bit to allow for better sorting.
- Log files now start with a line that contains the PMP version number.
- Timestamps in log files now have millisecond precision.
- Exit code is now written to the log.
- Build instructions for Windows have been greatly simplified and now make use of vcpkg.
- The script for building a Win32 release archive has been replaced with a new one that does a 64-bit build.
- PMP now requires a C++17 compiler.
- PMP now requires CMake 3.8 or later.
- PMP now requires Qt 5.14 or later.
- PMP now requires TagLib 1.12 or later.

### Fixed
- Tracks were sometimes incorrectly marked as unavailable in the client.
- Server crashed when trying to play a track marked as unavailable while being in 'stopped' mode.
- Music collection change notifications were not transmitted correctly.
- When starting the client its main window sometimes appeared outside the visible screen area.
- All broken commands in the command-line remote have been fixed.
- Off-by-one error in bounds check when moving an item in the queue could lead to a crash of the server.
- When starting a new track, artist and title were displayed as "unknown" for a fraction of a second.
- For files that have two different hashes, both hashes are now used for calculating track statistics.

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
