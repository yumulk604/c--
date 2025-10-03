# Hospital Management System

Bu proje, basit bir Metin2 sunucu kodundan esinlenerek geliştirilmiş bir hastane yönetim sistemidir. Socket programlama ve SQLite veritabanı kullanarak hasta, doktor ve randevu yönetimi sağlar.

## Özellikler

- **Hasta Yönetimi**: Hasta kayıt, arama ve güncelleme
- **Doktor Yönetimi**: Doktor kayıt ve listeleme
- **Randevu Sistemi**: Randevu oluşturma ve onaylama
- **Tıbbi Kayıtlar**: Hasta tıbbi geçmişini yönetme
- **Kullanıcı Doğrulama**: Basit login sistemi

## Gereksinimler

- C++11 uyumlu derleyici (g++)
- SQLite3 kütüphanesi
- POSIX socket desteği

## Kurulum

### FreeBSD için:

```bash
# SQLite3'ü yükleyin
pkg install -y sqlite3

# Projeyi derleyin
make

# Veya debug modunda derleyin
make debug
```

### Linux için:

```bash
# SQLite3'ü yükleyin (Ubuntu/Debian)
sudo apt-get install libsqlite3-dev

# Projeyi derleyin
make
```

## Kullanım

### Sunucuyu Başlatma:

```bash
./hospital_server
```

Sunucu 13000 portunda çalışmaya başlayacak ve şu mesajı gösterecek:
```
Hospital Management Server started on port 13000
Features: Patient Registration, Search, Appointments, Medical Records
```

### Client'ı Çalıştırma:

Başka bir terminal açarak:

```bash
g++ -std=c++11 -Wall -O2 hospital_client.cpp -o hospital_client
./hospital_client
```

### Örnek Kullanım Senaryosu:

1. **Sunucuyu başlatın**
2. **Client'ı çalıştırın**
3. **Login yapın** (Henüz kullanıcı sistemi tam çalışmıyor, sadece prototip)
4. **Hasta kaydedin**
5. **Doktor kaydedin**
6. **Randevu oluşturun**
7. **Tıbbi kayıt ekleyin**

## Protokol Yapısı

Sistem client-server mimarisi kullanır ve şu paket türlerini destekler:

### Client'dan Server'a:
- `HEADER_CS_LOGIN`: Kullanıcı girişi
- `HEADER_CS_PATIENT_REGISTER`: Hasta kayıt
- `HEADER_CS_PATIENT_SEARCH`: Hasta arama
- `HEADER_CS_DOCTOR_REGISTER`: Doktor kayıt
- `HEADER_CS_DOCTOR_LIST`: Doktor listesi
- `HEADER_CS_APPOINTMENT_BOOK`: Randevu oluşturma
- `HEADER_CS_MEDICAL_RECORD_ADD`: Tıbbi kayıt ekleme

### Server'dan Client'a:
- `HEADER_SC_LOGIN_RESPONSE`: Giriş yanıtı
- `HEADER_SC_PATIENT_LIST`: Hasta listesi
- `HEADER_SC_DOCTOR_LIST`: Doktor listesi
- `HEADER_SC_APPOINTMENT_CONFIRM`: Randevu onayı
- `HEADER_SC_ERROR`: Hata mesajları

## Veritabanı Şeması

Sistem otomatik olarak şu tabloları oluşturur:

- **users**: Kullanıcı hesapları
- **patients**: Hasta bilgileri
- **doctors**: Doktor bilgileri
- **appointments**: Randevu kayıtları
- **medical_records**: Tıbbi kayıtlar

## Güvenlik Notları

Bu kod eğitim amaçlıdır ve production ortamında kullanılmamalıdır:

- Şifreler düz metin olarak saklanır
- Kimlik doğrulama sistemi temel seviyededir
- Input validation eksiktir
- SQL injection koruması yoktur

## Geliştirme

Kod, orijinal Metin2 sunucu kodundan esinlenerek geliştirilmiştir ancak hastane yönetimi için uyarlanmıştır. Daha gelişmiş özellikler eklemek için:

1. Authentication sistemini geliştirin
2. Web arayüzü ekleyin
3. REST API desteği ekleyin
4. Güvenlik önlemlerini artırın
5. Test coverage ekleyin

## Lisans

Bu proje eğitim amaçlıdır ve özgürce kullanılabilir.