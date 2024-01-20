#include <fstream>
#include <cstring>

#include "seeborg-irc-settings.h"
#include "seeutil.h"

using namespace std;

BotSettings botsettings;

// Bot Config File
// ---------------------------------------------------------------------------

struct ConfigSetting {
    const char *configline;
    const char *description;

    string *stringptr;
    float *floatptr;
    int *intptr;
};

static const ConfigSetting configsettings[] = {
  {"server", "Address of IRC server", &botsettings.server, NULL, NULL},
  {"serverport", "Server port", NULL, NULL, &botsettings.serverport},

  {"nickname", "Bot's nickname", &botsettings.nickname, NULL, NULL},
  {"username", "Bot's username (will show as ~<username>@some.host.com)", &botsettings.username, NULL, NULL},
  {"realname", "Bot's realname (will show in whois)", &botsettings.realname, NULL, NULL},
  {"quitmessage", "Bot's quit message", &botsettings.quitmessage, NULL, NULL},

  {NULL, NULL, NULL, NULL, NULL},      // Newline in cfg

  {"replyrate", "Reply rate to all messages (in percent)", NULL, &botsettings.replyrate, NULL},
  {"replynick", "Reply rate to messages containing bot's nickname (in percent)", NULL, &botsettings.replyrate_mynick, NULL},
  {"replymagic", "Reply rate to messages containing magic words (in percent)", NULL, &botsettings.replyrate_magic, NULL},

  {NULL, NULL, NULL, NULL, NULL},      // Newline in cfg

  {"speaking", "Controls whether the bot speaks at all (boolean)", NULL, NULL, &botsettings.speaking},
  {"learning", "Does the bot learn, or just replies (boolean)", NULL, NULL, &botsettings.learning},
  {"stealth", "Try to emulate a popular IRC client's behaviour (TODO, boolean)", NULL, NULL, &botsettings.stealth},
  {"joininvites", "Join the channels the bot was invited to (0 - no, 1 - yes, 2 - only by owner)", NULL, NULL, &botsettings.joininvites},

  {NULL, NULL, NULL, NULL, NULL},      // Newline in cfg

  {"autosaveperiod", "Autosave period (in seconds)", NULL, NULL, &botsettings.autosaveperiod},

  {NULL, NULL, NULL, NULL, NULL}
};

static const int numconfigsettings =
    sizeof(configsettings) / sizeof(configsettings[0]) - 1;

void LoadBotSettings() {
    string str;
    ifstream ifs("seeborg-irc.cfg");
    if (ifs.bad()) {
        return;
    }

    while (getline(ifs, str)) {
        trimString(str);
        if (str[0] == ';' || str[0] == '#' || str.empty()) {
            continue;
        }

        vector<string> cursetting;

        if (splitString(str, cursetting, "=") < 2) {
            continue;
        }

        trimString(cursetting[0]);
        trimString(cursetting[1]);

        bool channels = (!strcasecmp(cursetting[0].c_str(), "channels"));
        bool owners = (!strcasecmp(cursetting[0].c_str(), "owners"));
        bool magicwords = (!strcasecmp(cursetting[0].c_str(), "magicwords"));
        if (channels || owners || magicwords) {
            if (channels) {
                botsettings.channels.clear(); // remove default channels
            }
            vector<string> cursplit;
            if (!splitString(cursetting[1], cursplit, " ")) {
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

        for (int i = 0; i < numconfigsettings; i++) {
            const ConfigSetting *s = &configsettings[i];
            if (s->configline == NULL) {
                continue;
            }
            if (!strcasecmp(s->configline, cursetting[0].c_str())) {
                if (s->stringptr != NULL) {
                    *s->stringptr = cursetting[1];
                } else if (s->floatptr != NULL) {
                    *s->floatptr = atof(cursetting[1].c_str());
                } else if (s->intptr != NULL) {
                    *s->intptr = atoi(cursetting[1].c_str());
                }
                break;
            }
        }
    }
    ifs.close();
}
