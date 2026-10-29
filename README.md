# Xpre-Enyx

Firmware robot pendamping ESP32-C3 Super Mini: karakter OLED bergaya robot,
sensor gerak MPU6050, suhu/kelembapan DHT11, rotary encoder, audio PWM, dan
notifikasi BLE. Program tetap berjalan bila sensor atau layar tidak tersedia.

## Konfigurasi saat ini

| Fungsi | GPIO / konfigurasi |
|---|---|
| OLED SSD1306 128x64 + MPU6050 SDA | GPIO8 |
| OLED + MPU6050 SCL | GPIO7 |
| Encoder CLK / DT / SW | GPIO2 / GPIO3 / GPIO4 |
| DHT11 DATA | GPIO5 |
| Audio PWM ke input amplifier | GPIO6 |
| USB Serial | 115200 baud |
| OLED / MPU | 0x3C atau 0x3D / 0x68 atau 0x69 |

Gunakan label **GPIO** board, bukan nomor urutan kaki. Semua GND disatukan.
GPIO9 tidak dipakai aplikasi. Konfigurasi terpusat di [include/Config.h](include/Config.h).
Modul yang diduga rusak sebaiknya dilepas dari daya dan bus sebelum pengujian
modul pengganti; jalur I2C yang tertahan LOW dapat mengganggu semua perangkat.

## Build, upload, dan monitor

```powershell
python -m platformio run
python -m platformio run -t upload --upload-port COM15
python -m platformio device monitor --port COM15 --baud 115200
```

Tutup aplikasi lain yang memakai COM15 sebelum upload atau membuka monitor.
Port dapat berubah setelah cabut/pasang USB. `platformio.ini` mengaktifkan
USB CDC saat boot; DTR/RTS monitor dibuat 0 agar pembukaan monitor tidak
sengaja menahan board pada mode download. Jika muncul `waiting for download`,
lepaskan BOOT dan tekan RESET.

## Mulai menguji tanpa OLED

Buka monitor, ketik `help` lalu Enter. Gunakan LF, CR, atau CRLF.

```text
status
test
```

`test` menjalankan pemeriksaan sekitar 14 detik: respons/inisialisasi I2C,
pola OLED ON/OFF/grid/checker, dua nada audio, pembacaan DHT11/MPU, dan
jendela 10 detik untuk memutar encoder serta menekan/melepas tombol.
Ketik `test stop` untuk membatalkan. Program tetap memproses Serial selama tes.

**Hasil ACK atau inisialisasi OLED bukan bukti panel menyala.** Pola layar
harus dilihat, audio harus didengar, encoder harus digerakkan. Sensor yang
tidak tersedia dilaporkan `UNAVAILABLE`/`NO VALID READING`; ini dapat berarti
kabel, daya, alamat, atau perangkat bermasalah, bukan diagnosis kerusakan pasti.

| Perintah | Fungsi |
|---|---|
| `help` | Daftar perintah |
| `status` | Status perangkat, data sensor, heap, BLE, uptime |
| `stream on` / `stream off` | Status otomatis setiap 2 detik |
| `test` / `test all` | Pengujian terpadu |
| `test oled` | Pola layar penuh, kosong, grid, checker |
| `test mpu` / `test dht` | Pembacaan sensor |
| `test audio` | Dua nada; mengikuti pengaturan mute |
| `test encoder` | Putar dan tekan tombol selama 10 detik |
| `test i2c` | Scan alamat 1..126, satu transaksi per iterasi |
| `test stop` | Hentikan pengujian |
| `retry` | Coba inisialisasi ulang OLED dan MPU sekarang |
| `mood auto` | Animasi otomatis; merespons gerak MPU bila tersedia |
| `mood normal/curious/happy/surprised/love/sleepy/wink` | Pilih satu ekspresi; gunakan satu nama setelah `mood` |
| `sensitivity 2` | Ambang gerak dalam m/s2; rentang 0.5..8 |
| `calibrate` | Set baseline magnitudo gravitasi; letakkan MPU diam |
| `sound on` / `sound off` | Aktifkan/mute audio |
| `beep` | Bunyi singkat, mengikuti mute |
| `notify Halo!` | Simulasi pesan masuk di layar selama 5 detik |
| `menu` / `face` | Buka menu / kembali ke karakter |
| `sleep` | Deep sleep; bangun lewat tombol GPIO4 atau RESET |
| `reboot` | Restart board |

Perintah huruf kecil, maksimal 127 karakter. Baris berlebih dibuang seluruhnya.
Backspace didukung. Ekspresi, sensitivitas, dan pengaturan suara disimpan di
NVS; baseline kalibrasi hanya berlaku sampai restart.

## Navigasi dan karakter

Klik encoder membuka menu. Putar untuk memilih Face, Emotion, Sensors,
Notifications, atau Sound. Klik memilih aksi: menampilkan wajah, memilih
mode ekspresi, kalibrasi sensor, atau toggle suara. Pada menu Emotion,
putar lalu klik untuk menyimpan. Tahan tombol 0.8 detik untuk kembali ke
wajah atau membuka menu. Arah putaran dapat dibalik dengan menukar CLK/DT.

Animasi menampilkan kedipan acak, lirikan, napas, penasaran, senang, terkejut,
hati, mengantuk, dan wink. Mode otomatis menggunakan variasi adegan 36 detik;
gerak MPU mengubah ekspresi bila datanya tersedia. Magnitudo gravitasi
dikurangi agar board diam tidak selalu dianggap bersemangat. Target animasi
sekitar 30 FPS, bergantung transfer I2C dan pembacaan sensor.

Setelah 2 menit tanpa interaksi, layar diredupkan dan mode otomatis mengantuk.
Ini **bukan deep sleep**: BLE dan Serial tetap aktif. Deep sleep hanya melalui
`sleep`; USB dan BLE terputus, dan notifikasi BLE tidak membangunkan board.

## Notifikasi BLE

Nama perangkat: `Xpre-Enyx`.

| Atribut | UUID |
|---|---|
| Service | `a6e40001-b5a3-f393-e0a9-e50e24dcca9e` |
| RX, write | `a6e40002-b5a3-f393-e0a9-e50e24dcca9e` |
| TX, read/notify | `a6e40003-b5a3-f393-e0a9-e50e24dcca9e` |

Hubungkan aplikasi BLE client, lalu tulis teks ASCII 1..96 byte ke RX.
Karakter menampilkan pesan selama 5 detik dan mengeluarkan nada jika suara
aktif. TX mengirim acknowledgement; pesan paling baru menggantikan pesan
sebelumnya. Teks BLE hanya diperlakukan sebagai pesan, bukan perintah Serial.
Advertising dimulai ulang setelah disconnect. Transfer ini belum memakai
pairing/autentikasi dan belum otomatis mengambil notifikasi sistem smartphone;
untuk itu diperlukan aplikasi pendamping/bridge di ponsel.

## Struktur kode

- `src/main.cpp`: entry point Arduino.
- `include/Config.h`: konfigurasi pin, interval, DHT, dan UUID.
- `src/Application.cpp`: koordinasi, menu, penyimpanan, dan perintah aplikasi.
- `src/Hardware.cpp`: MPU6050, DHT11, encoder debounce, dan audio.
- `src/XpreOLED.cpp`: renderer karakter, menu teks, dan pola tes.
- `src/DeviceBus.cpp`: probe I2C dengan pemeriksaan jalur LOW.
- `src/SerialConsole.cpp`: parser baris dengan buffer terbatas.
- `src/Diagnostics.cpp`: pengujian bertahap tanpa delay panjang.
- `src/BleLink.cpp`: layanan BLE dan queue pesan ke main loop.

Lihat [arsitektur dan wiring](docs/arsitektur.md) untuk rincian perilaku,
serta [catatan validasi](docs/validasi.md) untuk hasil pengujian firmware.
