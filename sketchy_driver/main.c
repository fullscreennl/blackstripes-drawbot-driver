//user: rpi - pw: linuxcnc ip: 192.168.0.102
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NANOSVG_IMPLEMENTATION  // Expands implementation
#include "nanosvg/nanosvg.h"

#include "bool.h"
#include "sketchy.h"
#include "Preview.h"
#include "Config.h"
#include "machine-settings.h"
#include "Model.h"

#include "lua-5.2.3/lua.h"
#include "lua-5.2.3/lauxlib.h"
#include "lua-5.2.3/lualib.h"

#include "sketchy-ipc.h"


Point *POINT;

void moveTo(float x, float y){
    Model_moveTo(x, y);
}

void home(){
    Model_moveHome();
}

int __performAutoNulling(lua_State *L){
    autoNull(0);
    return 0;
}

int __performPowerDown(lua_State *L){
    autoNull(2);
    return 0;
}

int __performRefill(lua_State *L){
    autoNull(1);
    return 0;
}

int __moveTo(lua_State *L){

    DriverCommand *cmd = getCommand();
    if(cmd->commandCode == commandCodeStop || cmd->commandCode == commandCodePreviewAbort){
        Model_setPenMode(penModeManualUp);
        return lua_yield (L, 0);
    }

    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    moveTo(x, y);

    return 0;
}

int __penUp(lua_State *L){
    Model_setPenMode(penModeManualUp);
    return 0;
}

int __penDown(lua_State *L){
    Model_setPenMode(penModeManualDown);
    return 0;
}

int __canvasSize(lua_State *L){
    int w = Config_canvasWidth();
    int h = Config_canvasHeight();
    lua_pushnumber(L, w);
    lua_pushnumber(L, h);
    return 2;
}

void loadLua(){

    lua_State *L;

    L = luaL_newstate();
    luaL_openlibs(L);
    lua_register(L,"moveTo",__moveTo);
    lua_register(L,"autoNull",__performAutoNulling);
    lua_register(L,"powerDown",__performPowerDown);
    lua_register(L,"refill",__performRefill);
    lua_register(L,"penUp",__penUp);
    lua_register(L,"penDown",__penDown);
    lua_register(L,"canvasSize",__canvasSize); 

    if (luaL_loadfile(L, Config_getScriptName())){
        printf("luaL_loadfile() failed scriptname: %s\n",Config_getScriptName());   
    }

    if (lua_pcall(L, 0, 0, 0)){
        printf("lua_pcall() failed\n");  
    }

    lua_close(L);
}

void runLuaScript(){
    POINT = Point_allocWithSteps(0 ,0);
    loadLua();
    home();
    Model_finish();
    Model_null();
    Point_release(POINT);
}

//SVG handling
static float distPtSeg(float x, float y, float px, float py, float qx, float qy)
{
    float pqx, pqy, dx, dy, d, t;
    pqx = qx-px;
    pqy = qy-py;
    dx = x-px;
    dy = y-py;
    d = pqx*pqx + pqy*pqy;
    t = pqx*dx + pqy*dy;
    if (d > 0) t /= d;
    if (t < 0) t = 0;
    else if (t > 1) t = 1;
    dx = px + t*pqx - x;
    dy = py + t*pqy - y;
    return dx*dx + dy*dy;
}

static void cubicBez(float x1, float y1, float x2, float y2,
                     float x3, float y3, float x4, float y4,
                     float tol, int level)
{
    float x12,y12,x23,y23,x34,y34,x123,y123,x234,y234,x1234,y1234;
    float d;

    if (level > 12) return;

    x12 = (x1+x2)*0.5f;
    y12 = (y1+y2)*0.5f;
    x23 = (x2+x3)*0.5f;
    y23 = (y2+y3)*0.5f;
    x34 = (x3+x4)*0.5f;
    y34 = (y3+y4)*0.5f;
    x123 = (x12+x23)*0.5f;
    y123 = (y12+y23)*0.5f;
    x234 = (x23+x34)*0.5f;
    y234 = (y23+y34)*0.5f;
    x1234 = (x123+x234)*0.5f;
    y1234 = (y123+y234)*0.5f;

    d = distPtSeg(x1234, y1234, x1,y1, x4,y4);
    if (d > tol*tol) {
        cubicBez(x1,y1, x12,y12, x123,y123, x1234,y1234, tol, level+1); 
        cubicBez(x1234,y1234, x234,y234, x34,y34, x4,y4, tol, level+1); 
    } else {
        moveTo(x4, y4);
    }
}

int drawPath(float* pts, int npts, char closed, float tol)
{

    if(pts[0] > MAX_CANVAS_SIZE_X || pts[1] > MAX_CANVAS_SIZE_Y || pts[0] < 0 || pts[1] < 0 ){
        updateDriverState(driverStateOutOfBoundsError,"","OUT_OF_BOUNDS_ERROR");
        return -1;
    }

    moveTo(pts[0], pts[1]);
    Model_setPenMode(penModeManualDown);

    int i;
    for (i = 0; i < npts-1; i += 3) {
        DriverCommand *cmd = getCommand();
        if(cmd->commandCode == commandCodeStop || cmd->commandCode == commandCodePreviewAbort){
            return -1;
        }
        float* p = &pts[i*2];
        cubicBez(p[0],p[1], p[2],p[3], p[4],p[5], p[6],p[7], tol, 0);
    }

    if (closed) {
        moveTo(pts[0], pts[1]);
    }

    Model_setPenMode(penModeManualUp);

    return 0;
}

void runSVG(){

    POINT = Point_allocWithSteps(0 ,0);

    Model_setPenMode(penModeManualUp);

    float px = 0.8;
    NSVGshape* shape;
    NSVGpath* path;

    struct NSVGimage* image;
    image = nsvgParseFromFile(Config_getSVGName(), "px", 96);
    printf("svg path %s\n",Config_getSVGName());
    printf("svg size: %f x %f\n", image->width, image->height);

    if(image->shapes == NULL){
        updateDriverState(driverStateNoDataFoundInSVGError,"","NO_DATA_IN_SVG_ERROR");
    }

    for (shape = image->shapes; shape != NULL; shape = shape->next) {
        for (path = shape->paths; path != NULL; path = path->next) {
            int status = drawPath(path->pts, path->npts, path->closed, px * 0.1f);
            if(status == -1){
                nsvgDelete(image);
                Model_setPenMode(penModeManualUp);
                home();
                Model_finish();
                Model_null();
                Point_release(POINT);
                return;
            }
        }
    }

    nsvgDelete(image);
    home();
    Model_finish();
    Model_null();
    Point_release(POINT);

}


int main(int argc, char *argv[]){

    char *inifile = "config/default.ini";
    if(argc == 2){
        inifile = argv[1];
    }

    if (Config_setIniBasePath(inifile) == -1){
        printf("%s inifile name to long.\n", inifile);
        return 1;
    }

    shmCreate();

    // 1) load the config
    Config_load(inifile);
    int exitStatus = Config_exitStatus();
    updateDriverState(driverSatusCodeBusy,inifile,"DRIVER_STATE_BUSSY");

    int status;
    if(Config_getSVGName()){
        // 2) if we have an svg for the motion
        status = run(runSVG);
    }else{
        // 3) else run the lua script (motion control)
        status = run(runLuaScript);
    }

    DriverState *state = driverState();
    if(state->statusCode < 3){
        // 4) tell the server we are done if we dont have errors.
        setCommand("none",commandCodeNone,0.0,0);
        if(exitStatus != driverSatusCodeIdle){
            updateDriverState(exitStatus,"","DRIVER_STATE_NOT_NULLED");
	}else{
            updateDriverState(driverSatusCodeIdle,"","DRIVER_STATE_IDLE");
        }
    }else{
        printf("ERROR code: %i , %s\n",state->statusCode, state->name);
    }
    return status;

}



