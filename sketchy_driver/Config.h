#ifndef CONFIG_H
#define CONFIG_H

typedef struct
{
    const char* versionString;
    const char* name;
    const char* email;
    int canvasWidth;
    int canvasHeight;
    int exitStatus;
    float nibSize;
    const char *imagename;
    const char *motionScript;
    const char *motionSVG;
    const char *_svg;
    const char *_lua;
   	int maxDelay;
   	int minDelay;
    int minMoveDelay;
    int initialThreshold;
    int usePenChangeInLookAhead;
    int lookaheadMM;
} Config;

Config config;

void Config_reload();
void Config_load(char *inifilename);
void Config_write(char *inifilename);

int Config_getCanvasWidth();
int Config_getCanvasHeight();
float Config_getNibSize();

const char* Config_getScriptName();
const char* Config_getSVGName();
const char* Config_getEmail();

int Config_exitStatus();

int Config_maxDelay();
void Config_setMaxDelay(int value);

int Config_getLookaheadMM();
void Config_setLookaheadMM(int value);

int Config_minDelay();
void Config_setMinDelay(int value);

int Config_minMoveDelay();
void Config_setMinMoveDelay(int value);

int Config_canvasWidth();
void Config_setCanvasWidth(int value);

int Config_canvasHeight();
void Config_setCanvasHeight(int value);

void Config_setBasePath(char *bp);
int Config_setIniBasePath(char *inifilePath);

int Config_usePenChangeInLookAhead();
void Config_setUsePenChangeInLookAhead(int value);

void Config_setSVGJob(const char * value);
void Config_setLuaJob(const char * value);

const char * Config_getJob();

const char* Config_getJSON();

#endif
