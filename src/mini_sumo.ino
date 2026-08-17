unsigned long eskiZaman = 0;
unsigned long kacisBaslangic = 0;
int durum = 0;           
int kacisAsamasi = 0;   
unsigned long start_time = 0;
#define IR_BLOCK_TIME 0
#define SAG_STABLE_MS 10
#define SOL_STABLE_MS 10
#define ON_STABLE_MS 10
#define START_FILTER_WINDOW_MS 50

#define ADIM_SURESI 40       
#define DURMA_SURESI 400      
#define TAKTIK_180_SURESI 1500
#define KACIS_GERI_SURESI 150 
#define KACIS_DONUS_SURESI 190
#define PWM_FULL 255          
#define PWM_ARAMA 170         
const bool QTR_AKTIF = false ;  

const int sol_on = 5;
const int sol_arka = 3;
const int sag_arka = 6;
const int sag_on = 9;
int jsumo1 = A7, jsumo2 = A4, jsumo3 = A0; 
int QTR = A1, buton = 10, kumanda = 2;
int taktik_sol = 12, taktik_sag = 11;

int sol_veri, on_veri, sag_veri, qtr;
int taktik_phase = 0;

unsigned long sag_high_since = 0;
int sag_raw_last = 0;
unsigned long sol_high_since = 0;
int sol_raw_last = 0;
unsigned long on_high_since = 0;
int on_raw_last = 0;

int start_filter_enabled() {
  if (start_time == 0) return 0;
  return (millis() - start_time <= START_FILTER_WINDOW_MS) ? 1 : 0;
}

int sensor_filter_enabled() {
  return start_filter_enabled();    
}

int sag_sensor_stable(int raw) {
  unsigned long now = millis();

  if (!sensor_filter_enabled()) {
    sag_raw_last = 0;
    sag_high_since = 0;
    return raw ? 1 : 0;
  }
 
  if (raw) {
    if (!sag_raw_last) {
      sag_high_since = now;
    }
    sag_raw_last = 1;
    return (now - sag_high_since >= SAG_STABLE_MS) ? 1 : 0;
  }
 
  sag_raw_last = 0;
  sag_high_since = 0;
  return 0;
}

int sol_sensor_stable(int raw) {
  unsigned long now = millis();

  if (!sensor_filter_enabled()) {
    sol_raw_last = 0;
    sol_high_since = 0;
    return raw ? 1 : 0;
  }
 
  if (raw) {
    if (!sol_raw_last) {
      sol_high_since = now;
    }
    sol_raw_last = 1;
    return (now - sol_high_since >= SOL_STABLE_MS) ? 1 : 0;
  }
 
  sol_raw_last = 0;
  sol_high_since = 0;
  return 0;
}

int on_sensor_stable(int raw) {
  unsigned long now = millis();

  if (!sensor_filter_enabled()) {
    on_raw_last = 0;
    on_high_since = 0;
    return raw ? 1 : 0;
  }
 
  if (raw) {
    if (!on_raw_last) {
      on_high_since = now;
    }
    on_raw_last = 1;
    return (now - on_high_since >= ON_STABLE_MS) ? 1 : 0;
  }
 
  on_raw_last = 0;
  on_high_since = 0;
  return 0;
}

void setup() {
  
  pinMode(sol_on, OUTPUT); pinMode(sol_arka, OUTPUT);
  pinMode(sag_on, OUTPUT); pinMode(sag_arka, OUTPUT);
  pinMode(jsumo1, INPUT); pinMode(jsumo2, INPUT); pinMode(jsumo3, INPUT);
  pinMode(QTR, INPUT); pinMode(buton, INPUT); pinMode(kumanda, INPUT);
  pinMode(taktik_sol, INPUT); pinMode(taktik_sag, INPUT);

  delay(200);

 
  // --- KUMANDA BEKLEME (NON-BLOCKING + FİLTRELİ) ---
if (digitalRead(buton) == LOW) {
  taktik_phase = 1;
  while (digitalRead(kumanda) == HIGH) {
    
  }
  start_time = millis(); // IR parazit filtresi için başlangıç
}


  unsigned long t_basla = millis();
  while (millis() - t_basla < TAKTIK_180_SURESI) {
   
    sol_veri = sol_sensor_stable((analogRead(jsumo1) > 400) ? 1 : 0);
    on_veri = on_sensor_stable(digitalRead(jsumo2));
    sag_veri = sag_sensor_stable(digitalRead(jsumo3));

   
    if ((on_veri || sol_veri || sag_veri) && ( millis() > (start_time + IR_BLOCK_TIME))) {
      break; 
    }

    
    if (digitalRead(taktik_sol)) sol_don_pwm(PWM_FULL);
    else if (digitalRead(taktik_sag)) sag_don_pwm(PWM_FULL);
  }
  taktik_phase = 0;
  
  dur(); 
}

void loop() {
  sensor_oku();
  hareket();
}

void sensor_oku() {
  qtr = QTR_AKTIF ? ((analogRead(QTR) < 600) ? 1 : 0) : 0;
  sol_veri = sol_sensor_stable((analogRead(jsumo1) > 400) ? 1 : 0);
  on_veri = on_sensor_stable(digitalRead(jsumo2));
  sag_veri = sag_sensor_stable(digitalRead(jsumo3));
}

void hareket() {
  if (!QTR_AKTIF) {
    kacisAsamasi = 0;
  }

  
  if (QTR_AKTIF && qtr == 1 && kacisAsamasi == 0) {
    kacisAsamasi = 1;
    kacisBaslangic = millis();
  }

  if (QTR_AKTIF && kacisAsamasi == 1) {
    geri_ilerle_pwm(PWM_FULL);
    if (millis() - kacisBaslangic > KACIS_GERI_SURESI) {
      kacisAsamasi = 2;
      kacisBaslangic = millis();
    }
    return;
  }

  if (QTR_AKTIF && kacisAsamasi == 2) {
    if (digitalRead(taktik_sol)) sol_don_pwm(PWM_FULL);
    else if (digitalRead(taktik_sag)) sag_don_pwm(PWM_FULL);

    if (on_veri || sol_veri || sag_veri || (millis() - kacisBaslangic > KACIS_DONUS_SURESI)) {
      kacisAsamasi = 0;
    }
    return;
  }


  if (on_veri) {
    on_ilerle_pwm(PWM_FULL); 
  } 
  else if (sol_veri) {
    
   
    digitalWrite(sol_on, LOW); digitalWrite(sol_arka, HIGH);
    digitalWrite(sag_on, HIGH); digitalWrite(sag_arka, LOW);
  } 
  else if (sag_veri) {
    
    
    digitalWrite(sol_on, HIGH); digitalWrite(sol_arka, LOW);
    digitalWrite(sag_on, LOW); digitalWrite(sag_arka, HIGH);
  } 
  
  else {
    tara_adimli();
  }
}

void on_ilerle_pwm(int hiz) { analogWrite(sol_on, 255); digitalWrite(sol_arka, 0); analogWrite(sag_on, 255); digitalWrite(sag_arka, 0); }
void geri_ilerle_pwm(int hiz) { digitalWrite(sol_on, 0); analogWrite(sol_arka, 255); digitalWrite(sag_on, 0); analogWrite(sag_arka, 255); }
void sol_don_pwm(int hiz) { digitalWrite(sol_on, 0); analogWrite(sol_arka, 255); analogWrite(sag_on, 255); digitalWrite(sag_arka, 0); }
void sag_don_pwm(int hiz) { analogWrite(sol_on, 255); digitalWrite(sol_arka, 0); digitalWrite(sag_on, 0); analogWrite(sag_arka, 255); }
void dur() { digitalWrite(sol_on, 255); digitalWrite(sol_arka, 255); digitalWrite(sag_on, 255); digitalWrite(sag_arka, 255); }

void tara_adimli() {
  unsigned long suAn = millis();
  if (durum == 0) { 
    dur();
    if (suAn - eskiZaman > DURMA_SURESI) { eskiZaman = suAn; durum = 1; }
  } else { 
    on_ilerle_pwm(PWM_ARAMA);
    if (suAn - eskiZaman > ADIM_SURESI) { eskiZaman = suAn; durum = 0; }
  }
}