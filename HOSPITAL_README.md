# Hospital Management System

Bu proje, Metin2 sunucu kodlarından esinlenerek geliştirilmiş basit bir hastane yönetim sistemidir. Socket tabanlı iletişim kullanarak hasta kayıt, doktor randevu sistemi ve tıbbi kayıt yönetimi sağlar.

## Özellikler

- **Hasta Kayıt Sistemi**: Yeni hasta kaydı oluşturma
- **Hasta Arama**: İsim, telefon veya ID ile hasta arama
- **Randevu Sistemi**: Doktor randevu rezervasyonu
- **Tıbbi Kayıt**: Hasta muayene kayıtları ve tedavi bilgileri
- **Socket İletişimi**: TCP/IP tabanlı client-server mimarisi

## Dosya Yapısı

- `hospital_packet.h`: Paket tanımları ve veri yapıları
- `hospital_server.cpp`: Ana sunucu uygulaması
- `hospital_client.cpp`: İstemci uygulaması
- `Makefile`: Derleme dosyası

## Derleme

```bash
make all
```

Bu komut hem sunucuyu hem de istemciyi derler.

## Kullanım

### 1. Sunucuyu Başlatma

```bash
./hospital_server
```

Sunucu port 13000'de çalışmaya başlar ve şu mesajı gösterir:
```
Hospital Management Server started on port 13000
Available operations:
- Patient Registration (0x01)
- Patient Search (0x02)
- Appointment Booking (0x05)
- Medical Record Entry (0x06)
```

### 2. İstemciyi Çalıştırma

```bash
./hospital_client
```

İstemci sunucuya bağlanır ve menü gösterir:
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
- **Veri Tabanı**: Bellek içi (in-memory) basit yapılar

## Temizleme

```bash
make clean
```

Bu komut derlenmiş dosyaları temizler.

## Notlar

- Bu sistem eğitim amaçlıdır ve gerçek hastane ortamında kullanılmamalıdır
- Veriler sunucu kapatıldığında kaybolur (kalıcı depolama yok)
- Güvenlik önlemleri minimal seviyededir
- Çoklu istemci desteği sınırlıdır (tek seferde bir bağlantı)