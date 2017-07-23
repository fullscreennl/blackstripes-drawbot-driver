#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Config.h"
#include "inih/ini.h"
#include "machine-settings.h"

static char *basePath;
static char* iniFileName;

static int Config_handler(void* user, const char* section, const char* name,
                   const char* value)
{

    Config* pconfig = (Config*)user;
    pconfig->_lua = "";
    pconfig->_svg = "";

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("machine_settings", "version")) {

        pconfig->versionString = strdup(value);

    } else if (MATCH("machine_settings", "canvas_height")) {

        pconfig->canvasHeight = atoi(value);

    }else if (MATCH("machine_settings", "lookahead_mm")) {

        pconfig->lookaheadMM = atoi(value);

    } else if (MATCH("machine_settings", "canvas_width")) {

        pconfig->canvasWidth = atoi(value);

    }else if (MATCH("machine_settings", "marker_nib")) {

        pconfig->nibSize = atof(value);

    } else if (MATCH("user", "name")) {

        pconfig->name = strdup(value);

    } else if (MATCH("user", "email")) {

        pconfig->email = strdup(value);

    }else if (MATCH("jobticket", "motion_script")) {

        static char motionScript[128] = "";
        strcat(motionScript, basePath);
        strcat(motionScript, value);
        pconfig->motionScript = strdup(motionScript);
        pconfig->_lua = strdup(value);

    }else if (MATCH("jobticket", "motion_svg")) {

        static char motionSVG[128] = "";
        strcat(motionSVG, basePath);
        strcat(motionSVG, value);
        pconfig->motionSVG = strdup(motionSVG);
        pconfig->_svg = strdup(value);

    }else if (MATCH("machine_settings", "max_delay")) {

        pconfig->maxDelay = atoi(value);

    }else if (MATCH("machine_settings", "min_delay")) {

        pconfig->minDelay = atoi(value);

    }else if (MATCH("machine_settings", "min_move_delay")) {

        pconfig->minMoveDelay = atoi(value);

    }else if(MATCH("machine_settings", "pen_lookahead")){

        pconfig->usePenChangeInLookAhead = atoi(value);

    } else {

        return 0;  /* unknown section/name, error */
        
    }
    return 1;
}

int Config_setIniBasePath(char *inifilePath){

    char* token;
    char* string;
    char* tofree;

    string = strdup(inifilePath);

    if(strlen(inifilePath) > 128){
        return -1;
    }

    char bp[128] = "";

    if (string != NULL) {
        tofree = string;
        while ((token = strsep(&string, "/")) != NULL){
            if(strstr(token, ".ini") == NULL) {
                strcat(bp, token);
                strcat(bp, "/");
            }
        }
        free(tofree);
    }

    Config_setBasePath(bp);
    return 0;
}

void Config_setBasePath(char *bp){
    basePath = strdup(bp);
}

int Config_canvasWidth(){
    return config.canvasWidth;
}

int Config_canvasHeight(){
    return config.canvasHeight;
}

int Config_usePenChangeInLookAhead(){
    if(!config.usePenChangeInLookAhead){
        return 0;
    }
    return config.usePenChangeInLookAhead;
}

int Config_maxDelay(){
    if(!config.maxDelay){
        return MAXDELAY;
    }
    return config.maxDelay;
}

int Config_minDelay(){
    if(!config.minDelay){
        return MINDELAY;
    }
    return config.minDelay;
}

int Config_getCanvasWidth(){
    return config.canvasWidth;
}

int Config_getCanvasHeight(){
    return config.canvasHeight;
}

int Config_getLookaheadMM(){
    return config.lookaheadMM;
}

float Config_getNibSize(){
    return config.nibSize;
}

const char* Config_getScriptName(){
    return config.motionScript;
}

const char* Config_getSVGName(){
    return config.motionSVG;
}

const char* Config_getEmail(){
    return config.email;
}

int Config_minMoveDelay(){
    return config.minMoveDelay;
}


//setters

void Config_setMinMoveDelay(int value){
    config.minMoveDelay = value;
}

void Config_setMaxDelay(int value){
    config.maxDelay = value;
}

void Config_setLookaheadMM(int value){
    config.lookaheadMM = value;
}

void Config_setMinDelay(int value){
    config.minDelay = value;
}

void Config_setCanvasWidth(int value){
    config.canvasWidth = value;
}

void Config_setCanvasHeight(int value){
    config.canvasHeight = value;
}

void Config_setUsePenChangeInLookAhead(int value){
    config.usePenChangeInLookAhead = value;
}

void Config_setSVGJob(const char * value){
    config._svg = value;
    config._lua = "";
}

void Config_setLuaJob(const char * value){
    config._svg = "";
    config._lua = value;
}

const char * Config_getJob(){
    if(config._svg[0] != '\0'){
        return config._svg;
    }else if(config._lua[0] != '\0'){
        return config._lua;
    }
    return "";
}

const char * Config_getJSON(){
    char *json = "{"
                 "\"status\": \"success\","
                 "\"canvas_width\": %d,"
                 "\"canvas_height\": %d,"
                 "\"max_delay\": %d,"
                 "\"min_delay\": %d,"
                 "\"min_move_delay\": %d,"
                 "\"pen_lookahead\": %d,"
                 "\"lookahead_mm\": %d"
                 "}";

    static char formatted[200];
    sprintf(formatted, json, config.canvasWidth,
                             config.canvasHeight,
                             config.maxDelay,
                             config.minDelay,
                             config.minMoveDelay,
                             config.usePenChangeInLookAhead,
                             config.lookaheadMM);
    return formatted;
}

void Config_write(char *inifilename){

    FILE * fp;
    fp = fopen(inifilename, "w");
    fprintf(fp, "[machine_settings]\n");
    fprintf(fp, "version = %s\n", config.versionString);
    fprintf(fp, "canvas_width = %d\n", config.canvasWidth);
    fprintf(fp, "canvas_height = %d\n", config.canvasHeight);
    fprintf(fp, "marker_nib = %.1f\n", config.nibSize);
    fprintf(fp, "max_delay = %d\n", config.maxDelay);
    fprintf(fp, "min_delay = %d\n", config.minDelay);
    fprintf(fp, "min_move_delay = %d\n", config.minMoveDelay);
    fprintf(fp, "pen_lookahead = %d\n", config.usePenChangeInLookAhead);
    fprintf(fp, "lookahead_mm = %d\n\n", config.lookaheadMM);
    fprintf(fp, "[user]\n");
    fprintf(fp, "name = %s\n", config.name);
    fprintf(fp, "email = %s\n\n", config.email);
    fprintf(fp, "[jobticket]\n");
    if(config._lua[0] != '\0'){
        fprintf(fp, "motion_script = %s\n", config._lua);
    }
    if(config._svg[0] != '\0'){
        fprintf(fp, "motion_svg = %s\n", config._svg);
    }
    fclose(fp);
    
}

void Config_reload(){
    Config_load(iniFileName);
}

void Config_load(char *inifilename){
    
    if (ini_parse(inifilename, Config_handler, &config) < 0) {
        printf("Can't load '%s'\n",inifilename);
        exit(1);
    }

    iniFileName = inifilename;


    printf("Config loaded from '%s': version=%s, name=%s, email=%s w=%i h=%i nib=%f\n", 
        inifilename,
        config.versionString, 
        config.name, 
        config.email,
        config.canvasWidth,
        config.canvasHeight,
        config.nibSize);

    // printf("BASE %s\n", basePath);
    // printf("svg %s\n", Config_getSVGName());
    // printf("script %s\n", Config_getScriptName());
    // printf("pen_lookahead %d\n", Config_usePenChangeInLookAhead());

    // float xnull = CENTER - Config_getCanvasWidth()/2.0;
    // float ynull = SHOULDER_HEIGHT+CANVAS_Y + (500.0 - Config_getCanvasHeight()/2.0);

    // printf("origin x:%f y:%f \n",xnull,ynull);

}
