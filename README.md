# Hospital Management System

Bu proje, Metin2 kaynak kodlarından esinlenerek geliştirilmiş basit bir hastane yönetim sistemidir. Socket tabanlı client-server mimarisi kullanır.

## Özellikler

- **Hasta Kayıt Sistemi**: Yeni hasta kaydı ve giriş sistemi
- **Randevu Sistemi**: Doktor randevuları alma ve görüntüleme
- **Doktor Bilgileri**: Uzmanlık alanlarına göre doktor listesi
- **Tıbbi Kayıtlar**: Hasta geçmişi ve tedavi kayıtları
- **Socket Communication**: TCP/IP tabanlı iletişim protokolü

## Dosya Yapısı

- `hospital_packet.h`: Packet yapıları ve protokol tanımları
- `hospital_server.cpp`: Ana sunucu uygulaması
- `hospital_client.cpp`: Test client uygulaması
- `Makefile`: Derleme dosyası

## Derleme ve Çalıştırma

### Sunucuyu Derleme
```bash
make hospital_server
```

### Client'ı Derleme
```bash
make hospital_client
```

### Tümünü Derleme
```bash
make
```

### Temizleme
```bash
make clean
```

## Kullanım

### Sunucuyu Başlatma
```bash
./hospital_server
```
Sunucu port 13000'de çalışmaya başlar.

### Client'ı Çalıştırma
```bash
./hospital_client
```

## Client Menüsü

1. **Hasta Kaydı**: Yeni hasta bilgilerini sisteme kaydetme
2. **Hasta Girişi**: TC numarası ile hasta girişi
3. **Randevu Talebi**: Doktor için randevu alma
4. **Randevuları Görüntüleme**: Hastanın randevularını listeleme
5. **Doktor Listesi**: Mevcut doktorları görüntüleme
6. **Tıbbi Kayıtlar**: Hasta geçmişini görüntüleme

## Doktor Uzmanlık Alanları

- Genel Pratisyen
- Kardiyoloji
- Nöroloji
- Ortopedi
- Pediatri
- Dermatoloji

## Teknik Detaylar

- **Protokol**: TCP/IP Socket Communication
- **Packet Format**: Binary packet yapısı (Metin2 tarzı)
- **Veri Saklama**: In-memory (veritabanı bağlantısı yok)
- **Derleyici**: g++ (C++11 standardı)
- **Platform**: Linux/FreeBSD uyumlu

## Packet Yapıları

Sistem çeşitli packet türleri kullanır:
- Handshake packets (bağlantı kurulumu)
- Patient management packets (hasta yönetimi)
- Appointment packets (randevu sistemi)
- Doctor information packets (doktor bilgileri)
- Medical record packets (tıbbi kayıtlar)

## Örnek Kullanım Senaryosu

1. Client'ı başlatın
2. "Hasta Kaydı" seçeneğini seçin
3. Hasta bilgilerini girin
4. "Doktor Listesi" ile mevcut doktorları görün
5. "Randevu Talebi" ile randevu alın
6. "Randevuları Görüntüleme" ile randevularınızı kontrol edin

## Notlar

- Bu sistem eğitim amaçlıdır ve production kullanımı için uygun değildir
- Veri kalıcı değildir (sunucu kapatıldığında veriler kaybolur)
- Güvenlik önlemleri minimal seviyededir
- Gerçek bir hastane sisteminde veritabanı ve güvenlik önlemleri gerekir