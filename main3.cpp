#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include "lnn_cekirdek.h"

Servo servoSag;
Servo servoSol;

#define SERVO_SAG_PIN 4                                                 
#define SERVO_SOL_PIN 27

WiFiUDP udp;

const char* ssid = "Xiaomi 15T";
const char* password = "yusuf2323";
const char* computer_ip = "10.202.114.18";
const uint16_t udp_port = 1234;

SemaphoreHandle_t veri_kilidi;

// MPU KALDIRILDI - sapma açısı sabit 0
volatile float gercek_sapma_acisi = 0.0f;

volatile float guncel_m_sol = 0.0f;
volatile float guncel_m_sag = 0.0f;
volatile int guncel_durum_id = 0;

Lnn_ag *beyin = NULL;

typedef enum {
    STATE_NAVIGATE,
    STATE_EVADE
} RobotState;

RobotState guncel_durum = STATE_NAVIGATE;

volatile float gercek_sol_mesafe = 200.0f;
volatile float gercek_orta_mesafe = 200.0f;
volatile float gercek_sag_mesafe = 200.0f;
volatile float radar_haritasi[19];

TaskHandle_t Gorev_Gozler;
TaskHandle_t Gorev_Beyin;

Adafruit_VL53L0X tof_sol = Adafruit_VL53L0X();
Adafruit_VL53L0X tof_sag = Adafruit_VL53L0X();

#define XSHUT_SOL 15
#define XSHUT_SAG  13

#define ENA 14
#define IN1 26
#define IN2 25
#define IN3 12
#define IN4 32
#define ENB 33

#define PWM_KANAL_SOL 4
#define PWM_KANAL_SAG 5
#define MOTOR_PWM_FREKANS 5000
#define MOTOR_PWM_COZUNURLUK 10

#define ADRES_SOL 0x30
#define ADRES_SAG 0x32

void Tof_Adreslerini_Ayarla() {
    pinMode(XSHUT_SOL, OUTPUT);
    pinMode(XSHUT_SAG, OUTPUT);
    digitalWrite(XSHUT_SOL, LOW);
    digitalWrite(XSHUT_SAG, LOW);
    delay(10);

    digitalWrite(XSHUT_SOL, HIGH);
    delay(10);
    if (!tof_sol.begin(ADRES_SOL)) {
        Serial.println("Sol lazerde hata var");
    }

    digitalWrite(XSHUT_SAG, HIGH);
    delay(10);
    if (!tof_sag.begin(ADRES_SAG)) {
        Serial.println("Sag lazerde hata var");
    }
}

void LazerOkumaGorevi(void * parameter) {
    int servo_aci = 0;
    int servo_yon = 4;

    for (;;) {
        // mpu.update() ve mpu.getAngleZ() KALDIRILDI
        // gercek_sapma_acisi sabit 0 kalır

        servoSag.write(servo_aci);
        servoSol.write(180 - servo_aci);

        vTaskDelay(15 / portTICK_PERIOD_MS);

        VL53L0X_RangingMeasurementData_t olcum_sol;
        VL53L0X_RangingMeasurementData_t olcum_sag;

        tof_sol.rangingTest(&olcum_sol, false);
        tof_sag.rangingTest(&olcum_sag, false);

        float anlik_sag = 200.0f;
        float anlik_sol = 200.0f;

        if (olcum_sol.RangeStatus != 4) anlik_sol = olcum_sol.RangeMilliMeter / 10;
        if (olcum_sag.RangeStatus != 4) anlik_sag = olcum_sag.RangeMilliMeter / 10;

        if (xSemaphoreTake(veri_kilidi, portMAX_DELAY) == pdTRUE) {
            if (servo_aci < 45) {
                gercek_sag_mesafe = anlik_sag;
                gercek_sol_mesafe = anlik_sol;
            }
            if (servo_aci >= 45 && servo_aci <= 90) {
                gercek_orta_mesafe = std::min(anlik_sag, anlik_sol);
            }
            radar_haritasi[servo_aci / 10] = anlik_sag;
            radar_haritasi[(180 - servo_aci) / 10] = anlik_sol;
            xSemaphoreGive(veri_kilidi);
        }

        servo_aci = servo_aci + servo_yon;
        if (servo_aci >= 90 || servo_aci <= 0) servo_yon = -servo_yon;

    }
}
void TelemetriGonder() {

    // Sadece ekrana yazma yavaş, sistem hızı değişmez
    static unsigned long son_yazma = 0;
    bool ekrana_yaz = (millis() - son_yazma >= 500);
    if (ekrana_yaz) son_yazma = millis();

    if (ekrana_yaz) {
        Serial.println("─────────────────────────────");

        Serial.print("SOL   : "); Serial.print(gercek_sol_mesafe,  1); Serial.println(" cm");
        Serial.print("ORTA  : "); Serial.print(gercek_orta_mesafe, 1); Serial.println(" cm");
        Serial.print("SAG   : "); Serial.print(gercek_sag_mesafe,  1); Serial.println(" cm");

        Serial.println("─────────────────────────────");

        Serial.print("DURUM : ");
        Serial.println(guncel_durum == STATE_NAVIGATE ? "NAVIGATE (Devriye)" : "EVADE   (Kacis!!!)");

        Serial.print("MOTOR : SOL="); Serial.print(guncel_m_sol, 2);
        Serial.print("  SAG=");       Serial.println(guncel_m_sag, 2);

        Serial.println("─────────────────────────────");

        Serial.println("NORONLAR:");
        Serial.print("  [0] Sol-Lidar  : "); Serial.println(beyin->noronlar[0].y, 3);
        Serial.print("  [1] Orta-Lidar : "); Serial.println(beyin->noronlar[1].y, 3);
        Serial.print("  [2] Sag-Lidar  : "); Serial.println(beyin->noronlar[2].y, 3);
        Serial.print("  [3] Sol-Motor  : "); Serial.println(beyin->noronlar[3].y, 3);
        Serial.print("  [4] Sag-Motor  : "); Serial.println(beyin->noronlar[4].y, 3);
        Serial.print("  [5] Pusula     : "); Serial.println(beyin->noronlar[5].y, 3);

        Serial.println("═════════════════════════════");
        Serial.println();

        Serial.println("─────────────────────────────");
Serial.print("SOL   : "); Serial.print(gercek_sol_mesafe,  1); Serial.println(" cm");
Serial.print("ORTA  : "); Serial.print(gercek_orta_mesafe, 1); Serial.println(" cm");
Serial.print("SAG   : "); Serial.print(gercek_sag_mesafe,  1); Serial.println(" cm");
Serial.println("─────────────────────────────");
Serial.print("DURUM : ");
Serial.println(guncel_durum == STATE_NAVIGATE ? "NAVIGATE (Devriye)" : "EVADE   (Kacis!!!)");
Serial.print("MOTOR : SOL="); Serial.print(guncel_m_sol, 2);
Serial.print("  SAG=");       Serial.println(guncel_m_sag, 2);
Serial.println("─────────────────────────────");
Serial.println("NORONLAR:");
Serial.print("  [0] Sol-Lidar  : "); Serial.println(beyin->noronlar[0].y, 3);
Serial.print("  [1] Orta-Lidar : "); Serial.println(beyin->noronlar[1].y, 3);
Serial.print("  [2] Sag-Lidar  : "); Serial.println(beyin->noronlar[2].y, 3);
Serial.print("  [3] Sol-Motor  : "); Serial.println(beyin->noronlar[3].y, 3);
Serial.print("  [4] Sag-Motor  : "); Serial.println(beyin->noronlar[4].y, 3);
Serial.print("  [5] Pusula     : "); Serial.println(beyin->noronlar[5].y, 3);
// YENİ - ham girdi değerleri
Serial.println("HAM GIRISLER:");
float n_sol_debug  = fmax(0.0f, (beyin->noronlar[0].y * 2.0f) - 1.0f);
float n_orta_debug = fmax(0.0f, (beyin->noronlar[1].y * 2.0f) - 1.0f);
float n_sag_debug  = fmax(0.0f, (beyin->noronlar[2].y * 2.0f) - 1.0f);
Serial.print("  n_sol  : "); Serial.println(n_sol_debug,  3);
Serial.print("  n_orta : "); Serial.println(n_orta_debug, 3);
Serial.print("  n_sag  : "); Serial.println(n_sag_debug,  3);
Serial.print("  noron3.x girdi: "); Serial.println((n_sol_debug * 8.0f) + (n_orta_debug * 4.0f) + (n_sag_debug * -1.0f), 3);
Serial.print("  noron4.x girdi: "); Serial.println((n_sag_debug * 8.0f) + (n_orta_debug * 4.0f) + (n_sol_debug * -1.0f), 3);
Serial.println("═════════════════════════════");
Serial.println();
    }

    // UDP her zaman gönderilir, ekran yavaşlamasından etkilenmez
    if (WiFi.status() == WL_CONNECTED) {
        char buf[256];
        int pos = 0;
        pos += snprintf(buf + pos, sizeof(buf) - pos, "<R:");
        for (int i = 0; i < 19; i++) {
            pos += snprintf(buf + pos, sizeof(buf) - pos,
                            "%.0f%s", radar_haritasi[i], i < 18 ? "," : "");
        }
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        "|P:%.1f|M:%.2f,%.2f|S:%d|N:",
                        gercek_sapma_acisi, guncel_m_sol, guncel_m_sag, guncel_durum_id);
        for (int i = 0; i < 6; i++) {
            pos += snprintf(buf + pos, sizeof(buf) - pos,
                            "%.2f%s", beyin->noronlar[i].y, i < 5 ? "," : "");
        }
        pos += snprintf(buf + pos, sizeof(buf) - pos, ">");
        udp.beginPacket(computer_ip, udp_port);
        udp.print(buf);
        udp.endPacket();
    }
}

void Motor_Sur(float sol_guc, float sag_guc) {
    int pwm_sol = fabs(sol_guc) * 1032;
    int pwm_sag = fabs(sag_guc) * 1032;

    if (sol_guc > 0)       { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); }
    else if (sol_guc < 0)  { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
    else                   { digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW); }

    if (sag_guc > 0)       { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
    else if (sag_guc < 0)  { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
    else                   { digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW); }

    ledcWrite(PWM_KANAL_SOL, pwm_sol);
    ledcWrite(PWM_KANAL_SAG, pwm_sag);
}

void LnnBeyinGorevi(void * parameter) {
    float dt = 0.03f;
    float m_sol, m_sag;

    for (;;) {
        float anlik_sol = 200.0f;
        float anlik_orta = 200.0f;
        float anlik_sag = 200.0f;

        if (xSemaphoreTake(veri_kilidi, portMAX_DELAY) == pdTRUE) {
            anlik_sol  = gercek_sol_mesafe;
            anlik_orta = gercek_orta_mesafe;
            anlik_sag  = gercek_sag_mesafe;
            xSemaphoreGive(veri_kilidi);
        }

        beyin->noronlar[0].x = 1.0f - (anlik_sol  / 80.0f);
        beyin->noronlar[1].x = 1.0f - (anlik_orta / 80.0f);
        beyin->noronlar[2].x = 1.0f - (anlik_sag  / 80.0f);
        beyin->noronlar[5].x = 0.5f; 

        float n_sol  = (beyin->noronlar[0].y * 2.0f) - 1.0f;
        float n_orta = (beyin->noronlar[1].y * 2.0f) - 1.0f;
        float n_sag  = (beyin->noronlar[2].y * 2.0f) - 1.0f;

        if (n_sol  < 0) n_sol  = 0.0f;
        if (n_orta < 0) n_orta = 0.0f;
        if (n_sag  < 0) n_sag  = 0.0f;

        switch (guncel_durum) {
            case STATE_NAVIGATE:
            // Sadece 35 cm yakınına engel gelirse kaçışa başla!
            if (anlik_sag < 50 || anlik_sol < 50 || anlik_orta < 50) {
                 guncel_durum = STATE_EVADE;
                 beyin->noronlar[3].durum = 0.0f;
                 beyin->noronlar[4].durum = 0.0f;
                 beyin->noronlar[3].y = 0.0f;
                 beyin->noronlar[4].y = 0.0f;
            }
            break;

             case STATE_EVADE:
                 
                 beyin->noronlar[3].x = (n_sag * 30.0f) - (n_sol * 20.0f) - (n_orta * 15.0f);;
                 beyin->noronlar[4].x = (n_sol * 30.0f) - (n_sag * 20.0f) - (n_orta * 15.0f);;

                 // Engel 35 cm'de0n uzağa gittiğinde devriyeye (düz gitmeye) dön
                 if (anlik_sol >= 50 && anlik_orta >= 50 && anlik_sag >= 50) {
                     guncel_durum = STATE_NAVIGATE;
                 }
                 break;
        }

        Lnn_step(beyin, dt);

        if (guncel_durum == STATE_NAVIGATE) {
            // HIZI GERİ 1.0 YAPTIK! Motorlar sürtünmeye takılıp spin atmayacak.
            m_sol = 1.0f;
            m_sag = 1.0f;
        } else {
            // Dışarıdan müdahale yok, sadece LNN'in kendi kararı
            m_sol = beyin->noronlar[3].y;
            m_sag = beyin->noronlar[4].y;

            if (m_sol > 0.05f && m_sol < 0.6f) m_sol = 0.6f;
            if (m_sol < -0.05f && m_sol > -0.6f) m_sol = -0.6f;
            
            if (m_sag > 0.05f && m_sag < 0.6f) m_sag = 0.6f;
            if (m_sag < -0.05f && m_sag > -0.6f) m_sag = -0.6f;
        }

        // Güçleri -1 ile +1 arasına kilitle
        if (m_sol >  1.0f) m_sol =  1.0f;
        if (m_sol < -1.0f) m_sol = -1.0f;
        if (m_sag >  1.0f) m_sag =  1.0f;
        if (m_sag < -1.0f) m_sag = -1.0f;

        guncel_m_sol    = m_sol;
        guncel_m_sag    = m_sag;
        guncel_durum_id = (guncel_durum == STATE_NAVIGATE) ? 0 : 1;

        Motor_Sur(m_sol, m_sag);
        TelemetriGonder();

        vTaskDelay(15 / portTICK_PERIOD_MS);
    }
}

void katman_yok_et(Lnn_ag* k) {
    if (k != NULL) {
        if (k->noronlar != NULL) free(k->noronlar);
        free(k);
    }
}

void setup() {
    Serial.begin(115200);

    veri_kilidi = xSemaphoreCreateMutex();
    if (veri_kilidi == NULL) {
        Serial.println("Kritik Hata: Mutex olusturulamadi!");
        while (1);
    }

    katman_yok_et(beyin);

    Wire.begin(21, 19);
    Wire.setClock(400000);

    // MPU BAŞLATMA BLOĞU TAMAMEN KALDIRILDI


    WiFi.begin(ssid, password);
    Serial.print("Wifi baglaniliyor");
 if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nBaglandi. IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hedef IP: ");
    Serial.println(computer_ip);
    Serial.print("Hedef Port: ");
    Serial.println(udp_port);
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
} else {
    Serial.println("\nWifi bulunamadi, telsiz kapatiliyor.");
    WiFi.mode(WIFI_OFF);
}

    pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
    pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

    ledcSetup(PWM_KANAL_SOL, MOTOR_PWM_FREKANS, MOTOR_PWM_COZUNURLUK);
    ledcSetup(PWM_KANAL_SAG, MOTOR_PWM_FREKANS, MOTOR_PWM_COZUNURLUK);
    ledcAttachPin(ENA, PWM_KANAL_SOL);
    ledcAttachPin(ENB, PWM_KANAL_SAG);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    servoSag.setPeriodHertz(50);
    servoSol.setPeriodHertz(50);
    servoSag.attach(SERVO_SAG_PIN, 500, 2400);
    servoSol.attach(SERVO_SOL_PIN, 500, 2400);

    Serial.println("Sistem baslatiliyor...");
    Tof_Adreslerini_Ayarla();

    Serial.println("LNN Beyni olusturuluyor...");
    beyin = katman_olustur(6);
    Lnn_Yapilandir(beyin, 0, 3, 0.2f, 1.0f,  0.0f);
    Lnn_Yapilandir(beyin, 3, 5, 0.05f, 100.0f,  -5.0f);
    Lnn_Yapilandir(beyin, 5, 6, 0.2f, 1.0f,  0.0f);


// Hemen ardından test hareketi:
Serial.println("Servo test basliyor...");
servoSag.write(0);
servoSol.write(180);
delay(1000);
servoSag.write(90);
servoSol.write(90);
delay(1000);
servoSag.write(180);
servoSol.write(0);
delay(1000);
Serial.println("Servo test bitti");

    xTaskCreatePinnedToCore(LazerOkumaGorevi, "Gozcu", 16384, NULL, 1, &Gorev_Gozler, 0);
    xTaskCreatePinnedToCore(LnnBeyinGorevi,   "Beyin", 16384, NULL, 1, &Gorev_Beyin,  1);
}

void loop() {
    vTaskDelete(NULL);
}