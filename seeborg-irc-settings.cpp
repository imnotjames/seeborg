#include <fstream>
#include <cstring>

#include "seeborg-irc-settings.h"
#include "seeutil.h"

using namespace std;

BotSettings botsettings;

// Bot Config File
// ---------------------------------------------------------------------------

struct ConfigSetting {
    const char *env;
    const char *description;

    string *stringptr;
    float *floatptr;
    int *intptr;
};

static const ConfigSetting configsettings[] = {
        {"SEEBORG_SERVER_HOST", "Address of IRC server", &botsettings.server, NULL, NULL},
        {"SEEBORG_SERVER_PORT", "Server port", NULL, NULL, &botsettings.serverport},

        {"SEEBORG_NICK", "Bot's nickname", &botsettings.nickname, NULL, NULL},
        {"SEEBORG_USER", "Bot's username (will show as ~<username>@some.host.com)", &botsettings.username, NULL, NULL},
        {"SEEBORG_REAL", "Bot's realname (will show in whois)", &botsettings.realname, NULL, NULL},
        {"SEEBORG_QUIT", "Bot's quit message", &botsettings.quitmessage, NULL, NULL},

        {"SEEBORG_REPLY_RATE", "Reply rate to all messages (in percent)", NULL, &botsettings.replyrate, NULL},
        {"SEEBORG_REPLY_NICK", "Reply rate to messages containing bot's nickname (in percent)", NULL, &botsettings.replyrate_mynick, NULL},
        {"SEEBORG_REPLY_MAGIC", "Reply rate to messages containing magic words (in percent)", NULL, &botsettings.replyrate_magic, NULL},

        {"SEEBORG_SPEAKING", "Controls whether the bot speaks at all (boolean)", NULL, NULL, &botsettings.speaking},
        {"SEEBORG_LEARNING", "Does the bot learn, or just replies (boolean)", NULL, NULL, &botsettings.learning},
        {"SEEBORG_STEALTH", "Try to emulate a popular IRC client's behaviour (TODO, boolean)", NULL, NULL, &botsettings.stealth},
        {"SEEBORG_JOIN_INVITES", "Join the channels the bot was invited to (0 - no, 1 - yes, 2 - only by owner)", NULL, NULL, &botsettings.joininvites},

        {"SEEBORG_CHANNELS", "Channels, delimited by spaces", NULL, NULL, &botsettings.joininvites},

        {"SEEBORG_MAGIC_WORDS", "Magic words to trigger the bot, delimited by spaces", NULL, NULL, NULL},
        {"SEEBORG_OWNERS", "Owners, delimited by spaces", NULL, NULL, NULL},

        {"SEEBORG_DICTIONARY_FILE", "Autosave period (in seconds)", &botsettings.dictionaryFile, NULL, NULL},
        {"SEEBORG_DICTIONARY_SAVE_INTERVAL", "Autosave period (in seconds)", NULL, NULL, &botsettings.autosaveperiod},
};

static const int numconfigsettings =
        sizeof(configsettings) / sizeof(configsettings[0]) - 1;

void LoadBotSettings() {
    for (int i = 0; i < numconfigsettings; i++) {
        const ConfigSetting *s = &configsettings[i];

        string value = getenv(s->env);

        // Oh, this is some bad code but whatever, works.

        bool channels = (!strcasecmp(s->env, "SEEBORG_CHANNELS"));
        bool owners = (!strcasecmp(s->env, "SEEBORG_OWNERS"));
        bool magicwords = (!strcasecmp(s->env, "SEEBORG_MAGIC_WORDS"));


        if (channels || owners || magicwords) {
            if (channels) {
                botsettings.channels.clear(); // remove default channels
            }
            vector<string> cursplit;
            if (!splitString(value, cursplit, " ")) {
                continue;
            }
            for (auto &s : cursplit) {
                if (channels) {
                    lowerString(s);
                    botsettings.channels.insert(s);
                } else if (owners) {
                    IrcBotOwner ircbotowner;
                    ircbotowner.nickname = s;
                    botsettings.owners.push_back(ircbotowner);
                } else if (magicwords) {
                    botsettings.magicwords.push_back(s);
                }
            }
            continue;
        }

        if (s->stringptr != nullptr) {
            *s->stringptr = value;
        } else if (s->floatptr != nullptr) {
            *s->floatptr = atof(value.c_str());
        } else if (s->intptr != nullptr) {
            *s->intptr = atoi(value.c_str());
        }
    }
}
