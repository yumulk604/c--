# Hastane Yönetim Sistemi

Metin2 socket communication yapısından esinlenerek geliştirilmiş basit bir hastane yönetim sistemi.

## Özellikler

- **Hasta Kaydı**: Yeni hasta kayıt sistemi (Ad, Soyad, TC No, Yaş, Telefon)
- **Randevu Sistemi**: Doktorlara randevu oluşturma ve listeleme
- **Doktor Yönetimi**: Mevcut doktorları listeleme
- **Hasta Sorgulama**: Hasta bilgilerini görüntüleme
- **Socket İletişim**: TCP/IP üzerinden client-server mimarisi

## Teknik Detaylar

- **Dil**: C++11
- **Platform**: Linux/FreeBSD (POSIX socket API)
- **Port**: 13000 (varsayılan)
- **Mimari**: Client-Server
- **Veri Depolama**: Bellek tabanlı (map yapıları)

## Derleme

```bash
make
```

Bu komut hem server hem de client'ı derler:
- `hospital_server` - Sunucu uygulaması
- `hospital_client` - İstemci uygulaması

## Kullanım

### Sunucuyu Başlatma

```bash
./hospital_server
```

Sunucu 13000 portunda dinlemeye başlar.

### İstemciyi Başlatma

```bash
./hospital_client [sunucu_ip] [port]
```

Örnekler:
```bash
./hospital_client                    # localhost:13000'e bağlan
./hospital_client 192.168.1.100     # 192.168.1.100:13000'e bağlan
./hospital_client 192.168.1.100 8080 # 192.168.1.100:8080'e bağlan
```

## İstemci Menüsü

1. **Hasta Kaydet**: Yeni hasta kaydı oluşturur
2. **Randevu Oluştur**: Mevcut hasta için doktor randevusu oluşturur
3. **Randevuları Listele**: Hastanın tüm randevularını gösterir
4. **Doktorları Listele**: Sistemdeki tüm doktorları gösterir
5. **Hasta Sorgula**: Hasta bilgilerini sorgular
0. **Çıkış**: Programdan çıkar

## Örnek Kullanım Senaryosu

1. Sunucuyu başlat: `./hospital_server`
2. İstemciyi başlat: `./hospital_client`
3. Önce doktorları listele (Menü 4)
4. Hasta kaydı oluştur (Menü 1)
5. Kayıt sonrası verilen hasta ID'sini not et
6. Bu ID ile randevu oluştur (Menü 2)
7. Randevuları listele (Menü 3)

## Örnek Doktorlar

Sistem varsayılan olarak 5 doktor ile başlar:

- ID 1: Dr. Mehmet Yılmaz (Kardiyoloji)
- ID 2: Dr. Ayşe Kaya (Nöroloji)
- ID 3: Dr. Ahmet Demir (Ortopedi)
- ID 4: Dr. Fatma Şahin (Göz Hastalıkları)
- ID 5: Dr. Can Öztürk (Dahiliye)

## Paket Yapısı

Sistem, Metin2 tarzı paket yapıları kullanır:

- `TPacketCGHandshake` / `TPacketGCHandshake`: Bağlantı doğrulama
- `TPacketCGRegisterPatient` / `TPacketGCRegisterPatient`: Hasta kaydı
- `TPacketCGCreateAppointment` / `TPacketGCCreateAppointment`: Randevu oluşturma
- `TPacketCGListAppointments` / `TPacketGCListAppointments`: Randevu listesi
- `TPacketCGListDoctors` / `TPacketGCListDoctors`: Doktor listesi
- `TPacketCGQueryPatient` / `TPacketGCQueryPatient`: Hasta sorgulama

## Limitasyonlar

- Veri kalıcı değildir (sunucu kapanınca veriler silinir)
- Tek thread'li yapı (aynı anda sadece bir client)
- Veritabanı bağlantısı yok
- Basit doğrulama mekanizması
- Çakışan randevu kontrolü yok

## Geliştirme Fikirleri

- [ ] MySQL/PostgreSQL veritabanı entegrasyonu
- [ ] Multi-threaded client desteği
- [ ] Randevu çakışma kontrolü
- [ ] Şifre ve yetkilendirme sistemi
- [ ] Doktor ekleme/silme işlemleri
- [ ] Hasta geçmişi ve tedavi kayıtları
- [ ] Ödeme ve fatura sistemi
- [ ] Reçete yönetimi

## Temizlik

```bash
make clean
```

## Lisans

Bu proje eğitim amaçlıdır.
