#!/bin/bash

# Hastane Sistemi Test Script
# Bu script basit bir test senaryosu gösterir

echo "========================================="
echo "  HASTANE SİSTEMİ TEST BAŞLATILIYOR"
echo "========================================="
echo ""

# Sunucunun çalıştığını kontrol et
if ! pgrep -x "hospital_server" > /dev/null; then
    echo "⚠️  Sunucu çalışmıyor. Lütfen önce sunucuyu başlatın:"
    echo "   ./hospital_server"
    echo ""
    echo "Ardından başka bir terminalde bu scripti çalıştırın."
    exit 1
fi

echo "✓ Sunucu çalışıyor"
echo ""
echo "Test için client'ı manuel olarak çalıştırın:"
echo "  ./hospital_client"
echo ""
echo "Örnek test adımları:"
echo "  1. Doktorları listele (4)"
echo "  2. Yeni hasta kaydet (1)"
echo "     - Ad: Ahmet"
echo "     - Soyad: Yılmaz"
echo "     - TC No: 12345678901"
echo "     - Yaş: 35"
echo "     - Telefon: 0555-123-4567"
echo "  3. Hasta ID'sini not et (örn: 1)"
echo "  4. Randevu oluştur (2)"
echo "     - Hasta ID: 1"
echo "     - Doktor ID: 1 (Dr. Mehmet Yılmaz - Kardiyoloji)"
echo "     - Tarih: 2025-10-15"
echo "     - Saat: 14:30"
echo "     - Şikayet: Göğüs ağrısı"
echo "  5. Randevuları listele (3)"
echo "  6. Hasta bilgisi sorgula (5)"
echo ""
