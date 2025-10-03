# Hospital Management System

Bu proje, Metin2 sunucu kodlarından esinlenerek geliştirilmiş gelişmiş bir hastane yönetim sistemidir. Socket tabanlı iletişim ve TimescaleDB veritabanı kullanarak hasta kayıt, doktor randevu sistemi ve tıbbi kayıt yönetimi sağlar.

## Özellikler

- **Hasta Kayıt Sistemi**: Yeni hasta kaydı oluşturma
- **Hasta Arama**: İsim, telefon veya ID ile hasta arama
- **Randevu Sistemi**: Doktor randevu rezervasyonu
- **Tıbbi Kayıt**: Hasta muayene kayıtları ve tedavi bilgileri
- **Socket İletişimi**: TCP/IP tabanlı client-server mimarisi
- **TimescaleDB Entegrasyonu**: Zaman serisi veritabanı desteği
- **Admin Arayüzü**: Gelişmiş yönetim paneli
- **Hypertable Desteği**: Büyük veri setleri için optimize edilmiş tablolar

## Dosya Yapısı

- `hospital_packet.h`: Paket tanımları ve veri yapıları
- `hospital_db.h/cpp`: TimescaleDB veritabanı sınıfı
- `hospital_server.cpp`: Ana sunucu uygulaması
- `hospital_client.cpp`: Basit test istemcisi
- `hospital_admin.cpp`: Gelişmiş admin arayüzü
- `setup_timescaledb.sql`: Veritabanı kurulum scripti
- `setup_hospital.sh`: Otomatik kurulum scripti
- `Makefile`: Derleme dosyası

## Kurulum

### Otomatik Kurulum (Önerilen)

```bash
./setup_hospital.sh
```

Bu script:
- PostgreSQL ve TimescaleDB kurulumunu kontrol eder
- Veritabanı şemasını oluşturur
- Gerekli kütüphaneleri yükler
- Uygulamaları derler

### Manuel Kurulum

1. **TimescaleDB Kurulumu**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install postgresql postgresql-contrib
   sudo apt-get install timescaledb-postgresql-13
   
   # CentOS/RHEL
   sudo yum install postgresql postgresql-server
   sudo yum install timescaledb-postgresql-13
   ```

2. **Veritabanı Kurulumu**:
   ```bash
   sudo -u postgres psql -f setup_timescaledb.sql
   ```

3. **Derleme**:
   ```bash
   make all
   ```

## Kullanım

### 1. Sunucuyu Başlatma

```bash
./hospital_server
```

Sunucu TimescaleDB'ye bağlanır, şemayı oluşturur ve port 13000'de çalışmaya başlar:
```
Connecting to TimescaleDB...
Connected to TimescaleDB successfully
Creating database tables...
Database tables created successfully
Creating TimescaleDB hypertables...
TimescaleDB hypertables created successfully
Inserting sample data...
Sample data inserted successfully
Database initialized successfully!
Patients: 0
Appointments: 0
Medical Records: 0

Hospital Management Server started on port 13000
Available operations:
- Patient Registration (0x01)
- Patient Search (0x02)
- Appointment Booking (0x05)
- Medical Record Entry (0x06)
```

### 2. Admin Arayüzünü Çalıştırma (Önerilen)

```bash
./hospital_admin
```

Gelişmiş admin arayüzü:
```
=== Hospital Administration System ===
1. Database Statistics
2. Patient Management
3. Doctor Management
4. Appointment Management
5. Medical Records
6. Database Maintenance
7. Exit
```

### 3. Basit İstemciyi Çalıştırma

```bash
./hospital_client
```

Basit test istemcisi:
```
=== Hospital Management System ===
1. Register Patient
2. Search Patient
3. Book Appointment
4. Add Medical Record
5. Exit
```

## Paket Yapıları

### Hasta Kayıt Paketi (0x01)
```cpp
struct TPacketPatientRegister {
    uint8_t bHeader;
    char name[64];
    char surname[64];
    char phone[16];
    char address[128];
    uint8_t age;
    char gender;
    char blood_type[4];
};
```

### Hasta Arama Paketi (0x02)
```cpp
struct TPacketPatientSearch {
    uint8_t bHeader;
    char search_term[64];
    uint8_t search_type; // 0=name, 1=phone, 2=id
};
```

### Randevu Paketi (0x05)
```cpp
struct TPacketAppointmentBook {
    uint8_t bHeader;
    uint32_t patient_id;
    uint32_t doctor_id;
    uint8_t day_of_week;
    uint8_t hour;
    char reason[128];
};
```

### Tıbbi Kayıt Paketi (0x06)
```cpp
struct TPacketMedicalRecord {
    uint8_t bHeader;
    uint32_t patient_id;
    uint32_t doctor_id;
    char diagnosis[256];
    char treatment[256];
    char medication[128];
    uint32_t visit_date;
    uint8_t follow_up_required;
};
```

## Örnek Kullanım Senaryosu

1. **Hasta Kaydı**: Yeni bir hasta kaydı oluşturun
2. **Hasta Arama**: Kayıtlı hastayı arayın
3. **Randevu Alma**: Doktor ile randevu alın
4. **Tıbbi Kayıt**: Muayene sonuçlarını kaydedin

## Doktor Bilgileri

Sistem başlangıçta şu doktorlarla gelir:
- Dr. Smith (Cardiology) - ID: 1
- Dr. Johnson (Neurology) - ID: 2
- Dr. Williams (Pediatrics) - ID: 3
- Dr. Brown (Orthopedics) - ID: 4

Tüm doktorlar Pazartesi-Cuma 09:00-17:00 arası çalışır.

## Teknik Detaylar

- **Derleyici**: g++ (FreeBSD uyumlu)
- **C++ Standardı**: C++11
- **Socket**: AF_INET, SOCK_STREAM
- **Port**: 13000
- **Veri Tabanı**: TimescaleDB (PostgreSQL tabanlı zaman serisi veritabanı)
- **Kütüphaneler**: libpq (PostgreSQL client library)
- **Hypertables**: Randevu ve tıbbi kayıtlar için optimize edilmiş zaman serisi tabloları

## Temizleme

```bash
make clean
```

Bu komut derlenmiş dosyaları temizler.

## Veritabanı Yapısı

### Tablolar
- **patients**: Hasta bilgileri
- **doctors**: Doktor bilgileri  
- **appointments**: Randevu kayıtları (hypertable)
- **medical_records**: Tıbbi kayıtlar (hypertable)

### TimescaleDB Özellikleri
- **Hypertables**: Büyük veri setleri için otomatik parçalama
- **Time-series optimization**: Zaman bazlı sorgular için optimize edilmiş
- **Compression**: Veri sıkıştırma desteği
- **Continuous aggregates**: Önceden hesaplanmış toplamlar

## Notlar

- Bu sistem eğitim amaçlıdır ve gerçek hastane ortamında kullanılmamalıdır
- Veriler TimescaleDB'de kalıcı olarak saklanır
- Güvenlik önlemleri minimal seviyededir
- Çoklu istemci desteği sınırlıdır (tek seferde bir bağlantı)
- TimescaleDB kurulumu gereklidir