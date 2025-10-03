# Hastane Yönetim Sistemi - Kullanım Kılavuzu

## 🏥 Proje Hakkında

Bu proje, Metin2 oyun sunucusunun socket communication yapısından esinlenerek geliştirilmiş basit bir hastane yönetim sistemidir. FreeBSD ve Linux sistemlerde çalışacak şekilde C++11 ile yazılmıştır.

### Ne İçeriyor?

✅ **Hasta Kaydı**: TC No ile benzersiz hasta kayıt sistemi  
✅ **Randevu Yönetimi**: Doktorlara randevu alma ve listeleme  
✅ **Doktor Listesi**: Sistem doktorlarını görüntüleme  
✅ **Hasta Sorgulama**: Kayıtlı hasta bilgilerini görüntüleme  
✅ **Socket İletişim**: TCP/IP üzerinden client-server mimarisi  
✅ **Handshake Protokolü**: Metin2 tarzı bağlantı doğrulama  

### Teknik Özellikler

- **Dil**: C++11 standardı
- **Platform**: Linux/FreeBSD (POSIX sockets)
- **Port**: 13000 (değiştirilebilir)
- **Veri**: Bellek tabanlı (std::map kullanılıyor)
- **Mimari**: Client-Server
- **Paket Yapısı**: `#pragma pack(push, 1)` ile byte-perfect paketler

## 🚀 Kurulum ve Derleme

### Gereksinimler

- g++ derleyici (C++11 desteği ile)
- POSIX uyumlu işletim sistemi (Linux/FreeBSD)
- Make build aracı

### Derleme

```bash
cd hospital_server
make
```

Bu komut iki çalıştırılabilir dosya oluşturur:
- `hospital_server` - Sunucu uygulaması
- `hospital_client` - İstemci uygulaması

### Temizlik

```bash
make clean
```

## 📖 Kullanım

### 1. Sunucuyu Başlatma

```bash
./hospital_server
```

Çıktı:
```
Hastane sistemi başlatılıyor...
5 doktor yüklendi.
Hastane sunucusu 13000 portunda başlatıldı
İstemci bağlantıları bekleniyor...
```

### 2. İstemciyi Başlatma

Varsayılan (localhost:13000):
```bash
./hospital_client
```

Farklı sunucuya bağlanma:
```bash
./hospital_client 192.168.1.100        # Özel IP
./hospital_client 192.168.1.100 8080   # Özel IP ve port
```

## 💡 Örnek Kullanım Senaryosu

### Adım 1: Doktorları Görüntüleme

İstemciyi başlattıktan sonra:

```
Seçim: 4

=== DOKTOR LİSTESİ ===

Toplam 5 doktor:

ID: 1
  Ad: Dr. Mehmet Yılmaz
  Uzmanlık: Kardiyoloji
  Telefon: 0555-111-2233

ID: 2
  Ad: Dr. Ayşe Kaya
  Uzmanlık: Nöroloji
  Telefon: 0555-222-3344

ID: 3
  Ad: Dr. Ahmet Demir
  Uzmanlık: Ortopedi
  Telefon: 0555-333-4455

ID: 4
  Ad: Dr. Fatma Şahin
  Uzmanlık: Göz Hastalıkları
  Telefon: 0555-444-5566

ID: 5
  Ad: Dr. Can Öztürk
  Uzmanlık: Dahiliye
  Telefon: 0555-555-6677
```

### Adım 2: Yeni Hasta Kaydı

```
Seçim: 1

=== HASTA KAYIT ===
Ad: Mehmet
Soyad: Demir
TC No: 12345678901
Yaş: 45
Telefon: 0532-111-2233

✓ Hasta başarıyla kaydedildi. ID: 1
```

**ÖNEMLİ**: Hasta ID'sini not edin! (Bu örnekte: 1)

### Adım 3: Randevu Oluşturma

```
Seçim: 2

=== RANDEVU OLUŞTUR ===
Hasta ID: 1
Doktor ID: 1
Tarih (YYYY-MM-DD): 2025-10-15
Saat (HH:MM): 14:30
Şikayet: Göğüs ağrısı ve nefes darlığı

✓ Randevu başarıyla oluşturuldu. ID: 1
```

### Adım 4: Randevuları Listeleme

```
Seçim: 3

=== RANDEVU LİSTESİ ===
Hasta ID: 1

Toplam 1 randevu:

ID: 1
  Doktor: Dr. Mehmet Yılmaz
  Tarih: 2025-10-15 14:30
  Şikayet: Göğüs ağrısı ve nefes darlığı
```

### Adım 5: Hasta Bilgisi Sorgulama

```
Seçim: 5

=== HASTA SORGULA ===
Hasta ID: 1

Hasta Bilgileri:
  ID: 1
  Ad Soyad: Mehmet Demir
  TC No: 12345678901
  Yaş: 45
  Telefon: 0532-111-2233
  Kayıt Tarihi: Fri Oct  3 15:45:23 2025
```

## 🔧 Teknik Detaylar

### Paket Yapısı

Tüm paketler `#pragma pack(push, 1)` ile tanımlanmıştır, bu da byte-aligned paketler demektir:

#### Handshake
```cpp
struct TPacketCGHandshake {
    uint8_t  bHeader;        // 0xFF
    uint32_t dwHandshake;    // Rastgele sayı
    uint32_t dwTime;         // Unix timestamp
    int32_t  lDelta;         // Zaman farkı
};
```

#### Hasta Kaydı
```cpp
struct TPacketCGRegisterPatient {
    uint8_t bHeader;         // 0x01
    char szName[32];         // Hasta adı
    char szSurname[32];      // Soyad
    char szTCNo[12];         // TC Kimlik No
    uint8_t bAge;            // Yaş
    char szPhone[16];        // Telefon
};
```

### Bağlantı Akışı

1. **Client bağlanır** → TCP connection
2. **Server handshake gönderir** → TPacketGCHandshake
3. **Client handshake yanıtlar** → TPacketCGHandshake (aynı handshake değeri ile)
4. **Doğrulama başarılı** → Komut döngüsü başlar
5. **Client komut gönderir** → Header ile paket seçimi
6. **Server işler ve yanıtlar** → İlgili GC paketi

### Veri Yapıları

```cpp
// Server tarafında bellek içi depolama
std::map<uint32_t, Patient> g_patients;           // ID → Hasta
std::map<uint32_t, Doctor> g_doctors;             // ID → Doktor
std::map<uint32_t, Appointment> g_appointments;   // ID → Randevu
std::map<std::string, uint32_t> g_tcnoToPatientID; // TC No → Hasta ID
```

## ⚠️ Bilinen Limitasyonlar

1. **Veri Kalıcılığı Yok**: Server kapatıldığında tüm veriler kaybolur
2. **Tek Thread**: Aynı anda sadece bir client bağlanabilir
3. **Veritabanı Yok**: Tüm veriler RAM'de tutulur
4. **Güvenlik**: Şifreleme veya authentication yok
5. **Randevu Çakışması**: Aynı saate birden fazla randevu alınabilir
6. **Hata İyileştirme**: Minimal error handling

## 🎯 Geliştirme Fikirleri

### Kolay Seviye
- [ ] Randevu silme özelliği
- [ ] Hasta güncelleme (telefon, adres değişikliği)
- [ ] Randevu çakışma kontrolü
- [ ] Log dosyası oluşturma

### Orta Seviye
- [ ] Multi-threading (pthread veya std::thread)
- [ ] SQLite veritabanı entegrasyonu
- [ ] Basit şifre sistemi
- [ ] Randevu hatırlatma sistemi

### İleri Seviye
- [ ] MySQL/PostgreSQL desteği
- [ ] TLS/SSL şifreleme
- [ ] Web arayüzü (REST API)
- [ ] Doktor müsaitlik takvimi
- [ ] Reçete ve tedavi geçmişi
- [ ] Ödeme ve fatura sistemi
- [ ] E-Nabız entegrasyonu

## 📝 Hata Ayıklama

### Sunucu başlatılamıyor

```bash
# Port zaten kullanımda
sudo netstat -tlnp | grep 13000
sudo kill -9 <PID>

# Veya farklı port kullan
# Server kodunda addr.sin_port = htons(13000); satırını değiştir
```

### Client bağlanamıyor

```bash
# Firewall kontrolü
sudo iptables -L -n | grep 13000

# Sunucu çalışıyor mu?
ps aux | grep hospital_server
```

### Derleme hataları

```bash
# C++11 desteği
g++ --version  # 4.8.1 veya üzeri olmalı

# Manuel derleme
g++ -std=c++11 -Wall -O2 -c hospital_server.cpp
g++ -std=c++11 -Wall -O2 -o hospital_server hospital_server.o
```

## 📚 Kaynak Kod Yapısı

```
hospital_server/
├── packet.h              # Paket tanımlamaları
├── hospital_server.cpp   # Server implementasyonu
├── hospital_client.cpp   # Client implementasyonu
├── Makefile              # Build scripti
├── README.md             # İngilizce döküman
├── KULLANIM.md           # Türkçe kullanım kılavuzu
└── test_example.sh       # Test scripti
```

## 🎓 Öğrenme Kaynakları

Bu projeden şunları öğrenebilirsiniz:

1. **Socket Programming**: TCP/IP server-client mimarisi
2. **Binary Protocols**: Byte-level veri paketleme
3. **State Management**: Bellek içi veri yönetimi
4. **C++ STL**: map, vector kullanımı
5. **POSIX APIs**: Unix socket programlama

## 🤝 Katkıda Bulunma

Projeyi geliştirmek isterseniz:

1. Kodu fork'layın
2. Yeni özellik ekleyin
3. Test edin
4. Pull request gönderin

## 📞 Destek

Sorunlarınız için:
- Kod içindeki yorumları okuyun
- README.md'deki teknik detaylara bakın
- Metin2 kaynak kodlarını inceleyin (benzer yapı)

## 🏆 Başarılar

Projeyi tamamladıysanız tebrikler! Şimdi:

✅ Socket programming'i öğrendiniz  
✅ Client-Server mimarisini anladınız  
✅ Binary protocol tasarladınız  
✅ Metin2 tarzı yapıyı keşfettiniz  

Bir sonraki adım: Veritabanı ekleyerek gerçek bir uygulama yapın!
