# Validasi firmware

Tanggal pengujian: 29 Oktober 2026. Board terhubung pada COM15,
teridentifikasi ESP32-C3 revisi 0.4, flash 4 MB, USB Serial/JTAG.

## Build dan upload

Build release PlatformIO berhasil, lalu firmware di-upload melalui esptool
dan hash flash diverifikasi. Profil board `esp32-c3-devkitm-1`, platform
Espressif32 7.1.3, Arduino ESP32 2.0.17. Integrasi BLE tetap muat pada
partisi aplikasi default: sekitar 81% flash dan 12.1% RAM statis.
Heap runtime saat pemeriksaan awal sekitar 193 KB; angka berubah saat BLE
terhubung dan bukan pengukuran kebocoran memori jangka panjang.

## Pengujian langsung

Hasil akhir: **18/18 pemeriksaan Serial**, **6/6 pemeriksaan BLE**, dan
**5/5 pemeriksaan pengaturan/recovery** lulus. Satu scan BLE pada pengulangan
setelah upload sempat timeout; scan ulang menemukan service proyek dan
pengujian BLE berikutnya lulus seluruhnya. Penyebab timeout tidak dipastikan.

[Hasil Serial](serial-test-results.json) menyimpan respons firmware dan
hasil parser, overflow/recovery buffer, CR/LF/CRLF, backspace, input tidak
valid, menu/face, notifikasi, status saat tes berjalan, pembatalan tes,
penyelesaian tes lengkap, dan stream status. Skrip gagal dengan exit code
nonzero bila ekspektasi tidak terpenuhi.

[Hasil BLE](ble-test-results.json) memuat enam pemeriksaan: advertising,
koneksi, service GATT, write pesan beserta acknowledgement notification,
penolakan pesan lebih dari 96 byte, dan advertising ulang setelah disconnect.
Pengujian dilakukan dari adapter Bluetooth Windows ke board; bukan integrasi
notifikasi sistem Android/iOS.

[Hasil pengaturan dan recovery](settings-test-results.json) memuat lima
pemeriksaan: scan I2C selesai, `retry` kembali merespons, perubahan pengaturan,
ketahanan pengaturan setelah restart, dan pemulihan nilai default. Nilai akhir:
`mood auto`, `sound on`, `sensitivity 2`.

## Kondisi hardware pada pengujian

| Komponen | Observasi |
|---|---|
| USB Serial | Perintah dan respons berhasil pada 115200 baud |
| BLE | Koneksi, write/notify, disconnect/re-advertise berhasil |
| SDA/SCL | HIGH/HIGH pada pembacaan status |
| OLED | UNAVAILABLE; tidak dapat memvalidasi gambar atau piksel |
| MPU6050 | UNAVAILABLE; gerak fisik belum tervalidasi |
| DHT11 | NO VALID READING; suhu/kelembapan fisik belum tervalidasi |
| Encoder | Posisi dan klik tetap 0; pengguna perlu memutar/menekan saat `test encoder` |
| Audio | Firmware meminta dua nada; suara perlu dikonfirmasi dengan mendengarkan |
| Deep sleep | Dikompilasi; siklus bangun GPIO4 belum diuji fisik |

Tidak terdeteksi atau tanpa data tidak membuktikan modul pasti rusak.
Gunakan wiring yang sesuai dan modul pengganti untuk validasi fisik.
Pemeriksaan ini bukan soak test jangka panjang dan tidak memvalidasi kualitas
animasi di panel rusak. Status OLED ACK pada sesi lama juga bukan bukti panel
sehat.

## Mengulang pengujian

Tutup Serial Monitor lain sebelum menjalankan skrip:

```powershell
python scripts/serial_smoke.py --port COM15
```

Skrip mengirim notifikasi tes dan menjalankan diagnostik penuh, termasuk nada
bila suara aktif. Pengujian tidak menekan encoder atau mengonfirmasi layar.

Opsional, untuk pengujian BLE di komputer dengan adapter Bluetooth aktif:

```powershell
python -m pip install --target .pio/test-tools bleak
python scripts/ble_smoke.py
```

Skrip BLE hanya menghubungi perangkat bernama Xpre-Enyx dengan service UUID
proyek. Hasil JSON ditulis ulang ketika skrip dijalankan. Pengujian NVS/restart
pada catatan ini dilakukan lewat perintah Serial, dengan nilai default
dipulihkan setelahnya.
