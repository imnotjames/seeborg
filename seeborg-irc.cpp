#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <clocale>
#include <cctype>
#include <cstring>
#include <iostream>

#include "seeborg.h"
#include "seeborg-irc.h"
#include "seeutil.h"
#include "seeborg-irc-settings.h"

using namespace std;

// Variables
irc_session_t *Session;

static SeeBorg gSeeBorg;
static bool cleanedup = false;

// Hacky mackro to define functions that do extraction of the params
#define DefExtract(X, Y) \
static inline string Extract ## X(const char *who) { \
    char buf[4096]; \
    buf[0] = '\0'; \
    irc_target_get_ ## Y(who, buf, sizeof(buf)); \
    return string(buf); \
}
DefExtract(Host, host)
DefExtract(Nick, nick)

// Message processing
// ---------------------------------------------------------------------------
static void checkOwners(const string& hostname, const string& nickname) {
    for (auto &owner : botsettings.owners) {
        if (owner.hostname.empty() && equalIString(nickname, owner.nickname)) {
            owner.hostname = hostname;
            cout << "Locked owner '" << nickname << "' to '"
                 << hostname << "'\n";
            return;
        }
    }
}

static bool isOwner(const string& hostname, const string& nickname) {
    for (auto &owner : botsettings.owners) {
        if (equalIString(hostname, owner.hostname) &&
            equalIString(nickname, owner.nickname)) {
            return true;
        }
    }
    return false;
}


static string ProcessMessage(const string &hostname,
        const string &nickname, const char msg[], bool replying = false) {
    string stdmessage = msg;
    lowerString(stdmessage);

    checkOwners(hostname, nickname);

    if (stdmessage[0] == '!') {
        string stdcmdreply;
        if (!isOwner(hostname, nickname)) return "";

        return gSeeBorg.ParseCommands(stdmessage);
    }

    trimString(stdmessage);

    // Ignore quotes
    if (isdigit(stdmessage[0]) || stdmessage[0] == '<' ||
        stdmessage[0] == '[') {
        return "";
    }

    // only both to see if we should reply if we're talking
    if (botsettings.speaking) {
        if (!replying && randFloat(0, 99) < botsettings.replyrate) {
            replying = true;
        }

        if (!replying && botsettings.replyrate_magic > 0) {
            for (auto& word : botsettings.magicwords) {
                if (stdmessage.find(word) == string::npos)
                    continue;

                if (randFloat(0, 99) < botsettings.replyrate_magic) {
                    replying = true;
                    break;
                }
            }
        }
        if (!replying && botsettings.replyrate_mynick > 0) {
            string nickname = botsettings.nickname;
            lowerString(nickname);
            if (stdmessage.find(nickname) != string::npos &&
                randFloat(0, 99) < botsettings.replyrate_mynick) {
                replying = true;
            }
        }
    }

    string replystring;

    if (replying && botsettings.speaking) {
        replystring = gSeeBorg.Reply(stdmessage);
    }

    if (botsettings.learning) {
        gSeeBorg.Learn(stdmessage);
    }

    return replystring;
}


static void DoChanTalk(irc_session_t *S, const string& hostname,
        const string &nickname, const char *Msg, const char *Chan,
        bool priv = false) {
    string reply = ProcessMessage(hostname, nickname, Msg, priv);

    if (reply.empty()) return;

    vector<string> curlines;
    splitString(reply, curlines, "\n");
    for (auto& str : curlines) {
        if (priv) {
            cout << Chan << " -> " << botsettings.nickname << ": ";
        } else {
            cout << "(" << Chan << ") <" << botsettings.nickname << "> ";
        }
        cout << str << "\n";
        irc_cmd_msg(S, Chan, str.c_str());
    }
}

#define PROC_ON(x) \
static void ProcOn ## x (irc_session_t *S, const char *event, \
        const char *origin, const char **params, unsigned int count) {\
    string nickname = ExtractNick(origin); \
    string hostname = ExtractHost(origin);

#define PROC_ON_END }


// BotNet callback functions
// ---------------------------------------------------------------------------
PROC_ON(Connected)
    cout << "Connected...\n";

    for (auto &cname : botsettings.channels) {
        cout << "Joining " << cname << "...\n";
        irc_cmd_join(S, cname.c_str(), NULL);
    }
PROC_ON_END

PROC_ON(Invite)
    const char *Chan = params[1];

    cout << "Received invitation to " << Chan << " by " << nickname << "\n";

    if (botsettings.joininvites) {
        if (botsettings.joininvites != 1) {
            // Check if the invite is sent by owner
            if (!isOwner(hostname, nickname)) {
                return;
            }
        }
        irc_cmd_join(S, Chan, NULL);
    }
PROC_ON_END

PROC_ON(Kick)
    const char *Chan = params[0];
    const char *Msg = params[2];
    const char *Whom = params[1];

    cout << "(" << Chan << ") * " << Whom << " has been kicked from " << Chan
         << " by " << nickname << " [" << Msg << "]\n";

    if (nickname == botsettings.nickname) {
        irc_cmd_join(S, Chan, NULL);
    }
PROC_ON_END

PROC_ON(PrivateTalk)
    const char *Msg = params[1];

    cout << nickname << ": " << Msg << "\n";

    DoChanTalk(S, hostname, nickname, Msg, origin, true);
PROC_ON_END

PROC_ON(ChannelTalk)
    const char *Chan = params[0];
    const char *Msg = params[1];

    cout << "(" << Chan << ") <" << nickname << "> " << Msg << "\n";

    DoChanTalk(S, hostname, nickname, Msg, Chan);
PROC_ON_END

PROC_ON(Action)
    const char *Msg = params[0];
    const char *Chan = params[1]; //? not specified in documentation

    string action = nickname + " " + Msg;

    cout << "(" << Chan << ") * " << action << "\n";

    DoChanTalk(S, hostname, nickname, action.c_str(), Chan);
PROC_ON_END

PROC_ON(Join)
    const char *Chan = params[0];

    cout << "(" << Chan << ") " << nickname << " (" << hostname
         << ") has joined the channel\n";

    DoChanTalk(S, hostname, nickname, nickname.c_str(), Chan);
PROC_ON_END

PROC_ON(Part)
    const char *Chan = params[0];
    const char *Msg = params[1];

    if (nickname == botsettings.nickname) return;

    cout << "(" << Chan << ") " << nickname << " (" << hostname
         << ") has left the channel the channel (" << Msg << ")\n";

    DoChanTalk(S, hostname, nickname, Msg, Chan);
PROC_ON_END

PROC_ON(Quit)
    const char *Chan = params[0];
    const char *Msg = params[1]?params[1]:"";

    cout << "" << nickname << " (" << hostname
         << ") has quit IRC (" << Msg << ")\n";

    DoChanTalk(S, hostname, nickname, Msg, Chan);
PROC_ON_END

// Main Body
// ---------------------------------------------------------------------------

void cleanup(void)
{
    if (cleanedup) return;
    cleanedup = true;
    if (irc_is_connected(Session)) {
        cout << "Disconnecting from server...\n";
        irc_cmd_quit(Session, botsettings.quitmessage.c_str());
    }
    cout << "Saving dictionary...\n";
    gSeeBorg.SaveSettings(botsettings.dictionaryFile);
}

static void sig_term(int i)
{
    if (!cleanedup) {
        // Save the settings before returning back to default signal handler
        cleanup();
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        exit(0);
    }
}


int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    cout << "SeeBorg v" SEEBORGVERSIONSTRING
           ", copyright (C) 2003 Eugene Bujak.\n";

    LoadBotSettings();
    if (argc < 2) {
        if (botsettings.server.empty()) {
            cout << "No server to connect to (check seeborg-irc.cfg)";
            return 1;
        }
    } else {
        botsettings.server = argv[1];
    }

    irc_callbacks_t calls;
    memset(&calls, 0, sizeof(calls));

    calls.event_connect = ProcOnConnected;
    calls.event_invite = ProcOnInvite;
    calls.event_kick = ProcOnKick;
    calls.event_join = ProcOnJoin;
    calls.event_part = ProcOnPart;
    calls.event_quit = ProcOnQuit;
    calls.event_channel = ProcOnChannelTalk;
    calls.event_privmsg = ProcOnPrivateTalk;
    calls.event_ctcp_action = ProcOnAction;

    Session = irc_create_session(&calls);

    srand(time(NULL));
    cout << "Loading dictionary...\n";
    gSeeBorg.LoadSettings(botsettings.dictionaryFile);
    signal(SIGINT, sig_term);
    signal(SIGTERM, sig_term);

    InstallIrcCommands(&gSeeBorg);

    cleanedup = false;
    atexit(cleanup);

    cout << "Connecting...\n";

    while (!irc_connect(Session, botsettings.server.c_str(),
                botsettings.serverport, 0, botsettings.nickname.c_str(),
                botsettings.username.c_str(), botsettings.realname.c_str())) {
        cout << "Connected.";
        irc_run(Session);
        cout << "Disconnected.\n";
        sleep(10);
        cout << "Reconnecting...\n";
    }
    return 0;
}
