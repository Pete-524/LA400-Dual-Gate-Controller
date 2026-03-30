#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"

using namespace esp_panel::board;

Board *board;

// ===== SCREENS =====
lv_obj_t *scr_main;
lv_obj_t *scr_settings;
lv_obj_t *scr_setup;
lv_obj_t *scr_diag;

// ===== UI =====
lv_obj_t *label_status;
lv_obj_t *label_data;
lv_obj_t *timer_label;

// ===== STATE =====
int g1=0,g2=0,c1=0,c2=0,state=0;
bool autoClose=false;
int autoDelay=30;
uint32_t autoStart=0;

// ===== COLORS =====
lv_color_t col_bg = lv_color_hex(0x121212);
lv_color_t col_text = lv_color_white();
lv_color_t col_open = lv_color_hex(0x00C853);
lv_color_t col_close = lv_color_hex(0xFFD600);
lv_color_t col_stop = lv_color_hex(0xD50000);

// ===== HELPERS =====
void setTextWhite(lv_obj_t *obj)
{
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
}

void styleScreen(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, col_bg, 0);
}

// ===== COMMAND =====
void sendCmd(const char *cmd){
    Serial1.println(cmd);
}

// ===== SERIAL =====
void handleSerial()
{
    if(!Serial1.available()) return;

    String s = Serial1.readStringUntil('\n');

    if(s.startsWith("STAT:"))
        sscanf(s.c_str(),"STAT:%d,%d,%d,%d,%d",&state,&g1,&g2,&c1,&c2);

    if(s.startsWith("FAULT:"))
        lv_label_set_text(label_status,"FAULT");
}

// ===== AUTO =====
void updateAuto()
{
    if(!autoClose)
    {
        lv_label_set_text(timer_label, "Auto: OFF");
        return;
    }

    int remaining = autoDelay - (millis() - autoStart) / 1000;

    if(remaining <= 0)
    {
        sendCmd("CLOSE");
        autoClose = false;
        lv_label_set_text(timer_label, "Auto: OFF");
    }
    else
    {
        lv_label_set_text_fmt(timer_label, "Auto Close: %ds", remaining);
    }
}

// ===== BUTTON =====
lv_obj_t* makeBtn(lv_obj_t *parent,const char* txt,int w,int h,int x,int y,lv_color_t color,const char* cmd)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn,w,h);
    lv_obj_align(btn,LV_ALIGN_TOP_LEFT,x,y);

    lv_obj_set_style_bg_color(btn,color,0);
    lv_obj_set_style_radius(btn,12,0);

    lv_obj_add_event_cb(btn,[](lv_event_t*e){
        Serial1.println((const char*)lv_event_get_user_data(e));
    },LV_EVENT_PRESSED,(void*)cmd);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl,txt);
    lv_obj_center(lbl);

    // ✅ FIX: WHITE TEXT
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

    return btn;
}

// ===== STATUS =====
void updateStatus()
{
    const char *txt="STOPPED";
    lv_color_t color = col_text;

    if(state==1){ txt="OPENING"; color=col_open; }
    if(state==2){ txt="CLOSING"; color=col_close; }
    if(state==3){ txt="REVERSING"; color=col_stop; }

    lv_label_set_text(label_status,txt);
    lv_obj_set_style_text_color(label_status,color,0);
}

// ===== MAIN =====
void createMain()
{
    scr_main = lv_obj_create(NULL);
    styleScreen(scr_main);

    label_status = lv_label_create(scr_main);
    setTextWhite(label_status);
    lv_obj_align(label_status,LV_ALIGN_TOP_MID,0,10);
    lv_obj_set_style_text_font(label_status,&lv_font_montserrat_30,0);

    label_data = lv_label_create(scr_main);
    setTextWhite(label_data);
    lv_obj_align(label_data,LV_ALIGN_TOP_MID,0,70);

    timer_label = lv_label_create(scr_main);
    setTextWhite(timer_label);
    lv_obj_align(timer_label, LV_ALIGN_TOP_MID, 0, 120);

    // initial text
    lv_label_set_text(timer_label, "Auto: OFF");

    makeBtn(scr_main,"OPEN",200,90,40,300,col_open,"OPEN");
    makeBtn(scr_main,"STOP",200,90,280,300,col_stop,"STOP");
    makeBtn(scr_main,"CLOSE",200,90,520,300,col_close,"CLOSE");

    // ===== AUTO BUTTON =====
    lv_obj_t *auto_btn = lv_btn_create(scr_main);
    lv_obj_set_size(auto_btn, 220, 70);
    lv_obj_align(auto_btn, LV_ALIGN_TOP_MID, 0, 220);

    // Initial color
    lv_obj_set_style_bg_color(auto_btn, lv_color_hex(0x444444), 0);

    lv_obj_t *auto_label = lv_label_create(auto_btn);
    lv_label_set_text(auto_label, "Auto Close");
    lv_obj_center(auto_label);
    lv_obj_set_style_text_color(auto_label, lv_color_white(), 0);

    // Toggle behavior
    lv_obj_add_event_cb(auto_btn, [](lv_event_t *e){

    autoClose = !autoClose;
    autoStart = millis();

    lv_obj_t *btn = lv_event_get_target(e);

    if(autoClose)
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x00C853), 0); // GREEN
    else
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x444444), 0); // GRAY

    }, LV_EVENT_PRESSED, NULL);

    // NAV
    makeBtn(scr_main,"SET",120,60,680,20,lv_color_hex(0x444444),"SET");
    lv_obj_add_event_cb(lv_obj_get_child(scr_main,-1),[](lv_event_t *e){
        lv_scr_load(scr_settings);
    },LV_EVENT_PRESSED,NULL);

    makeBtn(scr_main,"DIAG",120,60,20,20,lv_color_hex(0x444444),"DIAG");
    lv_obj_add_event_cb(lv_obj_get_child(scr_main,-1),[](lv_event_t *e){
        lv_scr_load(scr_diag);
    },LV_EVENT_PRESSED,NULL);

    makeBtn(scr_main,"SETUP",160,60,320,150,lv_color_hex(0x444444),"SETUP");
    lv_obj_add_event_cb(lv_obj_get_child(scr_main,-1),[](lv_event_t *e){
        lv_scr_load(scr_setup);
    },LV_EVENT_PRESSED,NULL);
}

// ===== SETTINGS =====
void createSettings()
{
    scr_settings = lv_obj_create(NULL);
    styleScreen(scr_settings);

    lv_obj_t *lbl = lv_label_create(scr_settings);
    setTextWhite(lbl);
    lv_obj_align(lbl,LV_ALIGN_TOP_MID,0,60);
    lv_label_set_text_fmt(lbl,"%d sec",autoDelay);

    lv_obj_t *slider = lv_slider_create(scr_settings);
    lv_obj_set_width(slider,500);
    lv_obj_align(slider,LV_ALIGN_TOP_MID,0,120);

    lv_slider_set_range(slider,15,120);
    lv_slider_set_value(slider,autoDelay,LV_ANIM_OFF);

    lv_obj_add_event_cb(slider,[](lv_event_t *e){
        autoDelay = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
    },LV_EVENT_VALUE_CHANGED,NULL);

    makeBtn(scr_settings,"BACK",120,60,20,400,lv_color_hex(0x555555),"BACK");
    lv_obj_add_event_cb(lv_obj_get_child(scr_settings,-1),[](lv_event_t *e){
        lv_scr_load(scr_main);
    },LV_EVENT_PRESSED,NULL);
}

// ===== SETUP =====
void createSetup()
{
    scr_setup = lv_obj_create(NULL);
    styleScreen(scr_setup);

    makeBtn(scr_setup,"G1 OPEN",180,70,100,100,col_open,"G1_OPEN");
    makeBtn(scr_setup,"G1 CLOSE",180,70,500,100,col_close,"G1_CLOSE");

    makeBtn(scr_setup,"SET G1 OPEN",180,70,100,200,lv_color_hex(0x00ACC1),"SET_G1_OPEN");
    makeBtn(scr_setup,"SET G1 CLOSE",180,70,500,200,lv_color_hex(0x00ACC1),"SET_G1_CLOSE");

    makeBtn(scr_setup,"G2 OPEN",180,70,100,300,col_open,"G2_OPEN");
    makeBtn(scr_setup,"G2 CLOSE",180,70,500,300,col_close,"G2_CLOSE");

    makeBtn(scr_setup,"SET G2 OPEN",180,70,100,400,lv_color_hex(0x00ACC1),"SET_G2_OPEN");
    makeBtn(scr_setup,"SET G2 CLOSE",180,70,500,400,lv_color_hex(0x00ACC1),"SET_G2_CLOSE");

    makeBtn(scr_setup,"BACK",120,60,20,20,lv_color_hex(0x555555),"BACK");
    lv_obj_add_event_cb(lv_obj_get_child(scr_setup,-1),[](lv_event_t *e){
        lv_scr_load(scr_main);
    },LV_EVENT_PRESSED,NULL);
}

// ===== DIAG =====
void createDiag()
{
    scr_diag = lv_obj_create(NULL);
    styleScreen(scr_diag);

    lv_obj_t *label = lv_label_create(scr_diag);
    setTextWhite(label);
    lv_obj_align(label,LV_ALIGN_CENTER,0,0);
    lv_label_set_text(label,"Diagnostics");

    makeBtn(scr_diag,"BACK",120,60,20,400,lv_color_hex(0x555555),"BACK");
    lv_obj_add_event_cb(lv_obj_get_child(scr_diag,-1),[](lv_event_t *e){
        lv_scr_load(scr_main);
    },LV_EVENT_PRESSED,NULL);
}

// ===== SETUP =====
void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200);

    board = new Board();
    board->init();
    board->begin();

    lvgl_port_init(board->getLCD(), board->getTouch());

    createMain();
    createSettings();
    createSetup();
    createDiag();

    lv_scr_load(scr_main);
}

// ===== LOOP =====
void loop()
{
    lv_timer_handler();

    handleSerial();
    updateAuto();
    updateStatus();

    lv_label_set_text_fmt(label_data,
        "G1:%d%%  G2:%d%%\nM1:%d  M2:%d",
        g1,g2,c1,c2);

    delay(5);
}