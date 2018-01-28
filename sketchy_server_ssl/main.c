/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#if MG_ENABLE_SSL
/*
 * This example starts an SSL web server on https://localhost:8443/
 *
 * Please note that the certificate used is a self-signed one and will not be
 * recognised as valid. You should expect an SSL error and will need to
 * explicitly allow the browser to proceed.
 */

#include "machine-settings.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#define NANOSVG_IMPLEMENTATION  // Expands implementation
#include "nanosvg.h"

#include <sys/types.h>
#include <sys/stat.h>
#include "mongoose/mongoose.h"
#include "sketchy-ipc.h"
#include "Config.h"

static const char *s_http_port = "8000";
static const char *s_ssl_cert = "server.pem";
static const char *s_ssl_key = "server.key";
// static struct mg_serve_http_opts s_http_server_opts;

/**
static const char *s_no_cache_header =
    "Cache-Control: max-age=0, post-check=0, "
    "pre-check=0, no-store, no-cache, must-revalidate\r\n";
*/

static void bs_printf(struct mg_connection *conn, const char* message){
    mg_printf(conn, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    mg_printf_http_chunk(conn, message);
    mg_send_http_chunk(conn, "", 0);
    return;
}

static int is_valid_job(struct mg_connection *conn){
    int w = Config_canvasWidth();
    int h = Config_canvasHeight();
    if (w > MAX_CANVAS_SIZE_X || h > MAX_CANVAS_SIZE_Y){
        char msg[255];
        sprintf(msg, "{ \"status\": \"failed\" , \"call\" : \"start-preview\" , \"msg\" : \"The drawing size is bigger than the machine can handle. (max size width x height is %3.2f x %3.2f)\"}", MAX_CANVAS_SIZE_X, MAX_CANVAS_SIZE_Y);
        bs_printf(conn, msg);
        return 0;
    }

    if(access("job/manifest.ini", F_OK ) == -1 ) {
        bs_printf(conn, "{ \"status\": \"failed\" , \"call\" : \"start-preview\" , \"msg\" : \"The manifest.ini file is not found.\"}");
        return 0;
    }

    char jobname[50];
    sprintf(jobname,"job/%s",Config_getJob());
    if(access(jobname, F_OK ) == -1 ) {
        bs_printf(conn, "{ \"status\": \"failed\" , \"call\" : \"start-preview\" , \"msg\" : \"The drawing is not found, plase try to upload the drawing again.\"}");
        return 0;
    }

    return 1;
}

static void handle_status_call(struct mg_connection *conn) {
    DriverState *state = driverState();
    char msg[255];
    sprintf(msg,   
                "{\"status\": \"success\", "
                            "\"call\" : \"status\", "
                            "\"name\": \"%s\", "
                            "\"statusCode\": %i, "
                            "\"messageID\": %i, "
                            "\"joburl\": \"%s\"}\n",  state->name,
                                                    state->statusCode,
                                                    state->messageID,
                                                    state->joburl);
    bs_printf(conn, msg);
}

static void update_config(){
    struct NSVGimage* image;
    image = nsvgParseFromFile("job/job.svg", "px", 96);
    Config_setCanvasWidth(image->width);
    Config_setCanvasHeight(image->height);
    Config_setSVGJob("job.svg");
    Config_write("job/manifest.ini");
}

static void handle_svg_call(struct mg_connection *conn, struct http_message *hm) {
    DriverState *state = driverState();
    char image_url[255];
    mg_get_http_var(&hm->body, "img_url", image_url, sizeof(image_url));
    char system_cmd[300] = "";
    sprintf(system_cmd,"wget -O job/job.svg %s",image_url); 
    system(system_cmd);

    update_config();

    char message[300]="";
    sprintf(message, "{\"status\": \"success\", "
                           "\"call\" : \"svg\", "
                           "\"statusCode\": %i}\n",
                                   state->statusCode);
    bs_printf(conn, message); 
}

static int read_settings_ini(){
    char *inifile = "job/manifest.ini";
    if (Config_setIniBasePath(inifile) == -1){
        printf("%s inifile name to long.\n", inifile);
        return 1;
    }
    Config_load(inifile);
    return 1;
}

static int handle_settings_update(struct mg_connection *conn, struct http_message *hm){

    //speeds
    char mindelay[100];
    char maxdelay[100];
    char minmovedelay[100];
    mg_get_http_var(&hm->body, "min_delay", mindelay, sizeof(mindelay));
    mg_get_http_var(&hm->body, "min_move_delay", minmovedelay, sizeof(minmovedelay));
    mg_get_http_var(&hm->body, "max_delay", maxdelay, sizeof(maxdelay));
    if(maxdelay[0] != '\0'){
        Config_setMaxDelay(atoi(maxdelay));
    }
    if(mindelay[0] != '\0'){
        Config_setMinDelay(atoi(mindelay));
    }
    if(minmovedelay[0] != '\0'){
        Config_setMinMoveDelay(atoi(minmovedelay));
    }

    //sizes
    char canvas_width[100];
    char canvas_height[100];
    mg_get_http_var(&hm->body, "canvas_width", canvas_width, sizeof(canvas_width));
    mg_get_http_var(&hm->body, "canvas_height", canvas_height, sizeof(canvas_height));
    if(canvas_width[0] != '\0'){
        Config_setCanvasWidth(atoi(canvas_width));
    }
    if(canvas_height[0] != '\0'){
        Config_setCanvasHeight(atoi(canvas_height));
    }

    //pen speed strategy
    char pen_lookahead[100];
    mg_get_http_var(&hm->body, "pen_lookahead", pen_lookahead, sizeof(pen_lookahead));
    if(pen_lookahead[0] != '\0'){
        Config_setUsePenChangeInLookAhead(atoi(pen_lookahead));
    }

    //lookahead mm
    char lookahead_mm[100];
    mg_get_http_var(&hm->body, "lookahead_mm", lookahead_mm, sizeof(lookahead_mm));
    if(lookahead_mm[0] != '\0'){
        Config_setLookaheadMM(atoi(lookahead_mm));
    }

    Config_write("job/manifest.ini");
    mg_http_send_redirect(conn, 302, mg_mk_str("/"), mg_mk_str(NULL));
    return 1;
}

static int handle_job_upload(struct mg_connection *conn, struct http_message *hm) {

    char var_name[100], file_name[100];
    const char *data;
    size_t chunk_len, n1, n2;

    n1 = n2 = 0;
    while ((n2 = mg_parse_multipart(hm->body.p + n1,
				    hm->body.len - n1,
				    var_name, sizeof(var_name),
				    file_name, sizeof(file_name),
				    &data, &chunk_len)) > 0) {

          // printf("var: %s, file_name: %s, size: %d, data: [%.*s]\n",
	  //        var_name, file_name, (int) chunk_len,
	  //        (int) chunk_len, data);

       n1 += n2;
    }

    //check if extension is either .svg // .lua // .ini // else .sketchy
    //.svg rename to job.svg
    //.lua rename to lua.svg
    //.ini thsts the mainfest referring to either svg or lua
    //.sketchy is the archive workflow containing both (manifest and source file) 
    struct stat st = {0};

    if (stat("job", &st) == -1) {
        mkdir("job", 0700);
    }

    //file_name
    char *ext = strrchr(file_name, '.');
    char *check_ini = "ini";
    char *check_svg = "svg";
    char *check_lua = "lua";
    if (!ext) {
     /* no extension */
    } else {

         if (strcmp(check_ini, ext+1)==0){

            FILE *fp = fopen("job/manifest.ini","w");
            fwrite(data, 1, chunk_len, fp);
            fclose(fp);

            read_settings_ini();

         }else if(strcmp(check_svg, ext+1)==0){

            FILE *fp = fopen("job/job.svg","w");
            fwrite(data, 1, chunk_len, fp);
            fclose(fp);

            update_config();

         }else if(strcmp(check_lua, ext+1)==0){

            FILE *fp = fopen("job/job.lua","w");
            fwrite(data, 1,chunk_len, fp);
            fclose(fp);
            Config_setLuaJob("job.lua");
            Config_write("job/manifest.ini");

         }else{

            system("exec rm -r job/*");
            FILE *fp = fopen("job/sketchy-job.tar.gz","w");
            fwrite(data, 1,chunk_len, fp);
            fclose(fp);
            system("tar -xf job/sketchy-job.tar.gz -C job");
            system("exec rm job/sketchy-job.tar.gz");

         }
    }
    mg_http_send_redirect(conn, 302, mg_mk_str("/"), mg_mk_str(NULL));
    return 1;
}

static void handle_preview_status_call(struct mg_connection *conn){
    DriverState *state = driverState();
    if(state->statusCode == driverSatusCodeIdle){
        bs_printf(conn, "{ \"status\": \"success\", \"call\" : \"preview-status\", \"msg\":\"DONE\"}");
        return;
    }else{
        bs_printf(conn, "{ \"status\": \"success\", \"call\" : \"preview-status\", \"msg\":\"BUSSY\"}");
        return;
    }
}

static void handle_preview_abort_call(struct mg_connection *conn){

    setCommand("preview-abort",commandCodePreviewAbort,0.0,0);
    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"preview-abort\"}");

}

static void handle_preview_call(struct mg_connection *conn){

    if(!is_valid_job(conn)){
        return;
    }

    DriverState *state = driverState();
    if(state->statusCode == driverSatusCodeBusy){
        bs_printf(conn, "{ \"status\": \"failed\", \"call\" : \"start\", \"msg\":\"Sketchy is busy\"}");
        return;
    }

    int status;
    if(fork() == 0){ 
        // Child process will return 0 from fork()
        status = system("./sketchy-preview job/manifest.ini");
        if(status != -1){
            //do something?
        }
        exit(0);
    }else{
        // Parent process will return a non-zero value from fork()
    }

    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"preview\"}");

}

static void handle_start_call(struct mg_connection *conn) {

    if(!is_valid_job(conn)){
        return;
    }

    DriverState *state = driverState();
    if(state->statusCode == driverSatusCodeBusy){
        bs_printf(conn, "{ \"status\": \"failed\", \"call\" : \"start\", \"msg\":\"Sketchy is busy\"}");
        return;
    }

    int status;
    if(fork() == 0){ 
        // Child process will return 0 from fork()
        status = system("./sketchy-driver job/manifest.ini");
        if(status != -1){
            //do something?
        }
        exit(0);
    }else{
        // Parent process will return a non-zero value from fork()
    }

    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"start\"}");

}

static void handle_null_call(struct mg_connection *conn) {

    //if(!is_valid_job(conn)){
    //    return;
    //}

    DriverState *state = driverState();
    if(state->statusCode == driverSatusCodeBusy){
        bs_printf(conn, "{ \"status\": \"failed\", \"call\" : \"start\", \"msg\":\"Sketchy is busy\"}");
        return;
    }

    int status;
    if(fork() == 0){ 
        // Child process will return 0 from fork()
        status = system("./sketchy-driver job/null.ini");
        if(status != -1){
            //do something?
        }
        exit(0);
    }else{
        // Parent process will return a non-zero value from fork()
    }

    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"null\"}");
    mg_send_http_chunk(conn, "", 0);
}

static void handle_refill_call(struct mg_connection *conn) {

    if(!is_valid_job(conn)){
        return;
    }

    DriverState *state = driverState();
    if(state->statusCode == driverSatusCodeBusy){
        bs_printf(conn, "{ \"status\": \"failed\", \"call\" : \"start\", \"msg\":\"Sketchy is busy\"}");
        return;
    }

    int status;
    if(fork() == 0){ 
        // Child process will return 0 from fork()
        status = system("./sketchy-driver job/refill.ini");
        if(status != -1){
            //do something?
        }
        exit(0);
    }else{
        // Parent process will return a non-zero value from fork()
    }

    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"refill\"}");

}

static void handle_powerdown_call(struct mg_connection *conn) {

    if(!is_valid_job(conn)){
        return;
    }

    DriverState *state = driverState();
    if(state->statusCode == driverSatusCodeBusy){
        bs_printf(conn, "{ \"status\": \"failed\", \"call\" : \"start\", \"msg\":\"Sketchy is busy\"}");
        return;
    }

    int status;
    if(fork() == 0){ 
        // Child process will return 0 from fork()
        status = system("./sketchy-driver job/powerdown.ini");
        if(status != -1){
            //do something?
        }
        exit(0);
    }else{
        // Parent process will return a non-zero value from fork()
    }

    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"powerdown\"}");

}

static void handle_ini_call(struct mg_connection *conn){
    const char* json = Config_getJSON();
    bs_printf(conn, json);
}

static void handle_pause_call(struct mg_connection *conn) {
    DriverState *state = driverState();
    if(state->statusCode != driverSatusCodeBusy){
        bs_printf(conn, "{ \"status\": \"failed\" , \"call\" : \"pause\", \"msg\":\"Can not pause, machine is not running.\"}");
        return;
    }
    setCommand("pause",commandCodePause,0.0,0);
    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"pause\"}");
}

static void handle_resume_call(struct mg_connection *conn) {
    DriverState *state = driverState();
    if(state->statusCode != driverStatusCodePaused){
        bs_printf(conn, "{ \"status\": \"failed\" , \"call\" : \"resume\", \"msg\":\"Can not resume, machine is not paused.\"}");
        return;
    }
    setCommand("resume",commandCodeNone,0.0,0);
    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"resume\"}");
}

static void handle_stop_call(struct mg_connection *conn) {
    setCommand("stop",commandCodeStop,0.0,0);
    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"stop\"}");
}

static void handle_reset_call(struct mg_connection *conn) {
    system("ipcrm -M 1234");
    system("ipcrm -M 4567");
    shmCreate();
    bs_printf(conn, "{ \"status\": \"success\" , \"call\" : \"reset\"}");
}




static void ev_handler(struct mg_connection *conn, int ev, void *p) {
    if (ev == MG_EV_HTTP_REQUEST) {
        struct http_message *hm = (struct http_message *) p;
        
        if(mg_vcmp(&hm->uri, "/handle_post_request") == 0){
            handle_job_upload(conn, hm);
        }

        if(mg_vcmp(&hm->uri, "/handle_settings_update") == 0){
            handle_settings_update(conn, hm);
        }

        if(mg_vcmp(&hm->uri, "/api/resetshm") == 0){
            handle_reset_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/start") == 0){
            handle_start_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/null") == 0){
            handle_null_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/refill") == 0){
            handle_refill_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/powerdown") == 0){
            handle_powerdown_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/preview") == 0){
            handle_preview_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/preview-status") == 0){
            handle_preview_status_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/preview-abort") == 0){
            handle_preview_abort_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/ini") == 0){
            handle_ini_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/stop") == 0){
            handle_stop_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/status") == 0){
            handle_status_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/pause") == 0){
            handle_pause_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/resume") == 0){
            handle_resume_call(conn);
        }

        if(mg_vcmp(&hm->uri, "/api/svg_url") == 0){
            handle_svg_call(conn, hm);
        }

        if(mg_vcmp(&hm->uri, "/job/manifest.ini") == 0){
            // mg_send_file(conn, "job/manifest.ini", s_no_cache_header);
            mg_http_serve_file(conn, hm, "job/manifest.ini", mg_mk_str("text/plain"), mg_mk_str(""));
        }

        if(mg_vcmp(&hm->uri, "/preview-img") == 0){
            // mg_send_file(conn, "preview_image.png", s_no_cache_header);
            mg_http_serve_file(conn, hm, "preview_image.png", mg_mk_str("image/png"), mg_mk_str(""));
        }

        if(mg_vcmp(&hm->uri, "/job") == 0){
            char jobname[50];
            sprintf(jobname,"job/%s",Config_getJob());
            if(access(jobname, F_OK ) != -1 ) {
                // mg_send_file(conn, jobname, s_no_cache_header);
                mg_http_serve_file(conn, hm, jobname, mg_mk_str("image/svg+xml"), mg_mk_str(""));
            }
        }

        if(mg_vcmp(&hm->uri, "/") == 0){
	    char user[100], password[100];
	    size_t user_len, pass_len;
            int status = mg_get_http_basic_auth(hm, user, user_len, password, pass_len);
            printf("%s\n", user);
            printf("%s\n", password);
            mg_http_serve_file(conn, hm, "index.html", mg_mk_str("text/html"), mg_mk_str(""));
	}
    }
}


int main(void) {
  struct mg_mgr mgr;
  struct mg_connection *nc;
  struct mg_bind_opts bind_opts;
  const char *err;

  mg_mgr_init(&mgr, NULL);
  memset(&bind_opts, 0, sizeof(bind_opts));
  bind_opts.ssl_cert = s_ssl_cert;
  bind_opts.ssl_key = s_ssl_key;
  bind_opts.error_string = &err;

  read_settings_ini();
  shmCreate();

  printf("Starting SSL server on port %s, cert from %s, key from %s\n",
         s_http_port, bind_opts.ssl_cert, bind_opts.ssl_key);
  nc = mg_bind_opt(&mgr, s_http_port, ev_handler, bind_opts);
  if (nc == NULL) {
    printf("Failed to create listener: %s\n", err);
    return 1;
  }

  // Set up HTTP server parameters
  mg_set_protocol_http_websocket(nc);
  // s_http_server_opts.document_root = ".";  // Serve current directory
  // s_http_server_opts.enable_directory_listing = "yes";

  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}
#else
int main(void) {
  return 0;
}
#endif /* MG_ENABLE_SSL */
