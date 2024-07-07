/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

    This file is part of PMP (Party Music Player).

    PMP is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    PMP is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along
    with PMP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "common/logging.h"
#include "common/version.h"
#include "common/util.h"

// for testing
#include "common/newfuture.h"
#include "common/newasync.h"
#include "common/newconcurrent.h"

#include "command.h"
#include "commandlineclient.h"
#include "commandparser.h"
#include "console.h"

#include <QByteArray>
#include <QtCore>
#include <QTcpSocket>

using namespace PMP;

static const char * const usageTextTemplate = R""""(
usage:
  {{PROGRAMNAME}} help|--help|version|--version
  {{PROGRAMNAME}} <server-name-or-ip> [<server-port>] <command>
  {{PROGRAMNAME}} <server-name-or-ip> [<server-port>] <login-command> : <command>

  commands:
    login: force authentication before running the next command (see below)
    status: get status information, like volume, queue length... (see below)
    play: start/resume playback
    pause: pause playback
    skip: jump to next track in the queue
    volume: get current volume percentage (0-100)
    volume <number>: set volume percentage (0-100)
    nowplaying: get info about the track currently playing
    queue: print queue length and the first tracks waiting in the queue
    history: print recent playback history
    personalmode: switch to personal mode
    publicmode: switch to public mode
    dynamicmode on|off: enable/disable dynamic mode (auto queue fill)
    break: insert a break at the front of the queue if not present there yet
    insert <item> <position>: insert an item into the queue (see below)
    qdel <QID>: delete an entry from the queue
    qmove <QID> <-diff>: move a track up in the queue (e.g. -3)
    qmove <QID> <+diff>: move a track down in the queue (eg. +2)
    start indexation [full]: start a full indexation of music files
    start indexation new: start a quick scan for new music files
    scrobbling enable|disable <provider>: enable scrobbling for the current user
    scrobbling status <provider>: get scrobbling status
    scrobbling authenticate <provider>: enter credentials for scrobbling
    shutdown: shut down the server program
    reloadserversettings: instruct the server to reload its settings file
    delayedstart wait <number> <time unit>: activate delayed start (see below)
    delayedstart at [<date>] <time>: activate delayed start (see below)
    delayedstart abort|cancel: cancel delayed start (see below)
    trackinfo <hash>: get track information like artist, title, length, etc.
    trackstats <hash>: get track statistics
    trackhistory <hash>: get personal listening history for a track
    serverversion: get server version information

  'login' command:
    login: forces authentication to occur; prompts for username and password
    login <username>: forces authentication to occur; prompts for password
    login <username> -: forces authentication to occur; reads password from
                        standard input
    login - [-]: forces authentication to occur; reads username and
                 password from standard input

    When reading username and password from standard input, it is assumed
    that the first line of the input is the username and the second line is
    the password.

  'status' command:
    This command does not require arguments and can be used without
    authenticating first. It provides general information about the server,
    like: is a track loaded, is something playing, what is the volume, what
    is the length of the queue, is dynamic mode active, etc.

  'insert' command:
    insert break <position>: insert a break into the queue
    insert barrier <position>: insert a barrier into the queue
    insert <hash> <position>: insert a track into the queue
    insert <item> front: insert something at the front of the queue
    insert <item> end: insert something at the end of the queue
    insert <item> index <number>: insert something at a specific index

    This command inserts a single item into the queue at a specific
    position. The item to be inserted can be a track, a break, or a
    barrier. The position can be the front of the queue, the end of the
    queue, or a specific index counted from the front. The index is
    zero-based, meaning that index 0 refers to the front of the queue,
    index 1 indicates after the first existing item, etc.
    Inserting a break or a barrier with the 'insert' command requires a
    fairly recent version of the PMP server in order to work. Older servers
    do not support barriers, and they only support inserting a break at the
    front of the queue, with the condition that there is no break present
    yet at that location (see the 'break' command).
    A barrier is like a break, but is never consumed. Playback just stops
    when the current track finishes and the first item in the queue is a
    barrier. The barrier will remain in the queue until it is deleted by
    the user.
    The hash of a track can be obtained with the 'track info' dialog in the
    Desktop Remote or with the command-line hash tool.

  'scrobbling' command:
    scrobbling enable <provider>: enable scrobbling for the current user
    scrobbling disable <provider>: disable scrobbling for the current user
    scrobbling status <provider>: get scrobbling status for the current user
    scrobbling authenticate <provider>: enter credentials for scrobbling

    This command controls scrobbling for the user running the command (you).
    It can be used to turn scrobbling on or off, to get the current status
    of the scrobbling mechanism, and to enter user credentials for the
    scrobbling provider. A provider always needs to be specified, so each
    operation will only apply to that provider.

    Only Last.FM is currently supported. Use "lastfm" or "last.fm" without
    quotes as the provider.

    Disabling scrobbling will not exclude any tracks for scrobbling; it will
    only suspend scrobbling temporarily until it is enabled again.

    Scrobbling must be enabled first before attempting to authenticate. The
    authentication for scrobbling can only be done interactively; username
    and password cannot be read from standard input. Authentication is only
    needed once; the PMP server will store a security token for all future
    access to the scrobbling provider. The 'scrobbling status' command will
    indicate if authentication is necessary.

    IMPORTANT: the authentication command for scrobbling will send your
    credentials for the scrobbling provider (Last.fm) to the PMP server.
    Do not run this command if the PMP server and remote are not on the
    same local network or if untrusted parties have access to the local
    network. The connection between the PMP server and the remote is not
    encrypted, so your credentials could be intercepted by an attacker.

  'delayedstart' command:
    delayedstart abort: cancel delayed start
    delayedstart cancel: cancel delayed start
    delayedstart wait <number> <time unit>: activate delayed start
    delayedstart at [<date>] <time>: activate delayed start

    Delayed start causes playback to start in the future, based on a timer.
    After the timer runs out, PMP starts playing as if the user had issued
    the 'play' command. Delayed start should not be affected by changes to
    the clock time on the server or the client after activation.
    Use 'wait' for specifying an exact delay between issuing the command
    and the time when playback will start. Time unit can be hours, minutes,
    seconds, or milliseconds. The countdown will start when the server
    receives the command, not earlier; keep that in mind if you need to
    type username or password in the console for authentication purposes.
    Reading username and password from standard input is recommended (see
    the 'login' command).
    Use 'at' for specifying the exact date and time when playback needs to
    start. If the date is omitted, the current date is assumed. The time is
    local client clock time and expected to be in format 'H:m' or 'H:m:s'.
    Only 24-hours notation is supported, no AM or PM. The date is expected
    to be in format 'yyyy-MM-dd'.
    A delayed start that has been activated but whose deadline has not been
    reached yet can still be cancelled with 'cancel' or 'abort'. Delayed
    start is cancelled automatically when playback is started before the
    deadline.

  'trackinfo' command:
    trackinfo <hash>: get track information like artist, title, length, etc.

    Retrieves title, artist, album, album artist, length and availability
    for the track that was specified as an argument.
    The hash of a track can be obtained with the 'track info' dialog in the
    Desktop Remote or with the command-line hash tool.

  'trackstats' command:
    trackstats <hash>: get track statistics for the current user

    Retrieves 'last heard' and 'score' for the current user and the track
    that was specified as an argument.
    The hash of a track can be obtained with the 'track info' dialog in the
    Desktop Remote or with the command-line hash tool.

  'trackhistory' command:
    trackhistory <hash>: get personal listening history for the current user

    Retrieves the recent listening history for the current user and the
    track that was specified as an argument.
    The hash of a track can be obtained with the 'track info' dialog in the
    Desktop Remote or with the command-line hash tool.

  NOTICE:
    Some commands require a fairly recent version of the PMP server in order
    to work.
    The 'shutdown' command no longer supports arguments.

  Authentication:
    All commands that have side-effects or access data that is user-specific
    require authentication. One exception to this principle is the 'queue'
    command; it requires authentication although it really should not. This
    may change in the future.
    Commands that require authentication will prompt for username and
    password in the console. The 'login' command can be used for
    non-interactive authentication.
    It used to be possible to run the 'shutdown' command with the
    server password as its argument and without logging in as a PMP user,
    but that is no longer possible. Support for this could be added again
    in the future, but that would not be compatible with older PMP servers.

  Server Password:
    This is a global password for the server, printed to stdout at
    server startup. It is no longer relevant for the PMP command-line
    client.

  Examples:
    {{PROGRAMNAME}} localhost status
    {{PROGRAMNAME}} localhost nowplaying
    {{PROGRAMNAME}} localhost personalmode
    {{PROGRAMNAME}} localhost dynamicmode on
    {{PROGRAMNAME}} localhost queue
    {{PROGRAMNAME}} ::1 volume
    {{PROGRAMNAME}} localhost volume 100
    {{PROGRAMNAME}} 127.0.0.1 play
    {{PROGRAMNAME}} localhost insert break index 2
    {{PROGRAMNAME}} localhost insert barrier front
    {{PROGRAMNAME}} localhost qmove 42 +3
    {{PROGRAMNAME}} localhost login : nowplaying
    {{PROGRAMNAME}} localhost login MyUsername : play
    {{PROGRAMNAME}} localhost login MyUsername - : play <passwordfile
    {{PROGRAMNAME}} localhost login - : play <credentialsfile
    {{PROGRAMNAME}} localhost delayedstart wait 1 minute
    {{PROGRAMNAME}} localhost delayedstart wait 90 seconds
    {{PROGRAMNAME}} localhost delayedstart at 15:30
    {{PROGRAMNAME}} localhost delayedstart at 9:30:00
    {{PROGRAMNAME}} localhost delayedstart at 2022-02-28 00:00
)"""";

static const char * const versionTextTemplate = R""""(
{{PROGRAMNAMEVERSIONBUILD}}
{{COPYRIGHT}}
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
)"""";

inline void testFutures()
{
    auto work = []() { return ResultOrError<int, FailureType>::fromResult(42); };

    // ====

    QObject* object = new QObject();

    auto eventLoopFuture = NewAsync::runOnEventLoop<int, FailureType>(object, work);

    // ====

    auto* threadPool = QThreadPool::globalInstance();
    auto future = NewConcurrent::runOnThreadPool<int, FailureType>(threadPool, work);

    auto work2 =
        [](ResultOrError<int, FailureType> input) -> ResultOrError<QString, FailureType>
    {
        if (input.failed())
            return failure;

        return QString::number(input.result()) + "!";
    };

    auto future2 = future.thenOnThreadPool<QString, FailureType>(threadPool, work2);

    // ====

    auto promise = NewAsync::createPromise<QString, FailureType>();
    auto future3 = promise.future();

    promise.setOutcome(failure);
}

void printVersion(QTextStream& out)
{
    // temporary call for testing - to be removed
    testFutures();

    const auto programNameVersionBuild =
        QString(VCS_REVISION_LONG).isEmpty()
            ? QString("Party Music Player %1")
                .arg(PMP_VERSION_DISPLAY)
            : QString("Party Music Player %1 build %2 (%3)")
                .arg(PMP_VERSION_DISPLAY,
                     VCS_REVISION_LONG,
                     VCS_BRANCH);

    QString versionText = versionTextTemplate;
    versionText = versionText.trimmed();
    versionText.replace("{{PROGRAMNAMEVERSIONBUILD}}", programNameVersionBuild);
    versionText.replace("{{COPYRIGHT}}", Util::getCopyrightLine(true));

    out << versionText << Qt::endl;
}

void printUsage(QTextStream& out)
{
    auto programName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();

    QString usageText = usageTextTemplate;
    usageText = usageText.trimmed();
    usageText.replace("{{PROGRAMNAME}}", programName);

    out << usageText << Qt::endl;
}

bool looksLikePortNumber(QString const& string)
{
    return string.size() > 0 && string[0].isDigit();
}

struct AuthenticationData
{
    QString username;
    QString password;
    QString error;

    bool haveError() const { return !error.isEmpty(); }
};

AuthenticationData handleAuthentication(CommandParser const& commandParser,
                                        bool commandRequiresAuthentication)
{
    AuthenticationData result;

    auto authenticationMode = commandParser.authenticationMode();

    switch (authenticationMode)
    {
        case AuthenticationMode::Implicit:
            if (!commandRequiresAuthentication)
                return result; /* no authentication */

            // fall through
        case AuthenticationMode::ExplicitAllInteractive:
            result.username = Console::prompt("PMP username: ");
            result.password = Console::promptForPassword("password: ");
            break;

        case AuthenticationMode::ExplicitPasswordInteractive:
            result.username = commandParser.explicitLoginUsername();
            result.password = Console::promptForPassword("password: ");
            break;
        case AuthenticationMode::ExplicitPasswordFromStdIn:
        {
            auto lines = Console::readLinesFromStdIn(1);
            if (lines.size() < 1)
            {
                result.error = "Could not read password from stdin";
                return result;
            }

            result.username = commandParser.explicitLoginUsername();
            result.password = lines[0];
            break;
        }
        case AuthenticationMode::ExplicitAllFromStdIn:
        {
            auto lines = Console::readLinesFromStdIn(2);
            if (lines.size() < 2)
            {
                result.error = "Could not read username and password from stdin";
                return result;
            }

            result.username = lines[0];
            result.password = lines[1];
            break;
        }
    }

    if (result.username.isEmpty())
    {
        result.error = "Username must not be empty";
    }
    else if (result.password.isEmpty())
    {
        result.error = "Password must not be empty";
    }

    return result;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Remote");
    QCoreApplication::setApplicationVersion(PMP_VERSION_DISPLAY);
    QCoreApplication::setOrganizationName(PMP_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(PMP_ORGANIZATION_DOMAIN);

    /* make sure that log messages do not go to stdout/stderr */
    Logging::enableTextFileOnlyLogging();
    Logging::setFilenameTag("CR"); /* CR = CMD-Remote */

    QTextStream out(stdout);
    QTextStream err(stderr);

    QStringList args = QCoreApplication::arguments();
    /* args[0] is the name of the program, throw that away */
    args = args.mid(1);

    if (args.size() > 0)
    {
        if (args[0] == "version" || args[0] == "--version")
        {
            printVersion(out);
            return 0;
        }
        if (args[0] == "help" || args[0] == "--help")
        {
            printUsage(out);
            return 0;
        }
    }

    if (args.size() < 2)
    {
        err << "Not enough arguments specified!" << Qt::endl;
        printUsage(err);
        return 1;
    }

    QVector<QString> commandWithArgs;

    QString server = args[0];

    quint16 portNumber;
    QString maybePortArg = args[1];
    if (looksLikePortNumber(maybePortArg))
    {
        /* Validate port number */
        bool ok = true;
        uint number = maybePortArg.toUInt(&ok);
        if (!ok || number > 0xFFFFu)
        {
            err << "Invalid port number: " << maybePortArg << Qt::endl;
            return 1;
        }

        portNumber = number;
        commandWithArgs = args.mid(2).toVector();
    }
    else
    {
        portNumber = 23432;
        commandWithArgs = args.mid(1).toVector();
    }

    if (commandWithArgs.size() == 0)
    {
        err << "Not enough arguments specified!" << Qt::endl;
        printUsage(err);
        return 1;
    }

    CommandParser commandParser;
    commandParser.parse(commandWithArgs);

    if (!commandParser.parsedSuccessfully())
    {
        err << commandParser.errorMessage() << Qt::endl;
        return 1;
    }

    Command* command = commandParser.command();

    auto authentication =
            handleAuthentication(commandParser, command->requiresAuthentication());

    if (authentication.haveError())
    {
        err << authentication.error << Qt::endl;
        return 1;
    }

    CommandlineClient client(nullptr, &out, &err, server, portNumber,
                             authentication.username, authentication.password, command);
    QObject::connect(
        &client, &CommandlineClient::exitClient,
        &app, &QCoreApplication::exit
    );

    client.start();

    return app.exec();
}
