# Arsitektur dan wiring

## Platform

ESP32-C3 Super Mini memakai profil PlatformIO `esp32-c3-devkitm-1`, Arduino,
USB CDC saat boot, dan Serial 115200. OLED 1,3 inci 128x64 menggunakan
controller **SH1106** melalui library Adafruit SH110X agar offset kolom
internalnya benar.
Sensor lingkungan yang dipilih adalah **DHT11**.

| Modul | Sambungan |
|---|---|
| OLED SH1106 128x64 | VCC 3.3V, GND, SDA GPIO8, SCL GPIO7 |
| MPU6050 | VCC 3.3V, GND, SDA GPIO8, SCL GPIO7; AD0 LOW=0x68/HIGH=0x69 |
| DHT11 | VCC 3.3V, GND, DATA GPIO5; pull-up DATA bila modul belum memilikinya |
| Encoder | CLK GPIO2, DT GPIO3, SW GPIO4, common/GND ke GND; input pull-up aktif |
| Sensor touch digital | OUT GPIO10, VCC 3.3V, GND; aktif HIGH |
| Amplifier | Input sinyal PWM GPIO6 dan GND bersama; supply mengikuti modul amplifier |
| Speaker | Output amplifier; tidak langsung digerakkan dari GPIO6 |

Pull-up I2C ke 3.3V diperlukan; umumnya tersedia pada modul, atau pasang
sekitar 4.7 kohm sesuai rangkaian. GPIO2 adalah pin strapping: encoder tidak
boleh memaksakan kondisi boot yang salah saat reset. GPIO9 dibiarkan untuk
BOOT. GPIO18/19 tidak dipakai aplikasi agar USB native tetap tersedia.
Tabel menggunakan nomor GPIO, bukan nomor kaki fisik Super Mini.

## Alur runtime

1. USB Serial, I2C 400 kHz/timeout 25 ms, input encoder, touch, DHT, dan audio dimulai.
2. Pengaturan NVS dibaca dan divalidasi. OLED/MPU dicoba pada alamat alternatif.
3. BLE advertising aktif; setiap iterasi melayani Serial, BLE queue, input,
   sensor, diagnostik, pemulihan perangkat, dan tampilan.
4. MPU dibaca setiap 100 ms, DHT setiap 2.5 detik, retry modul gagal setiap
   10 detik. Kegagalan perangkat tidak masuk loop berhenti permanen.
5. Status rutin hanya dikirim jika `stream on`; perintah tetap tersedia.

Transaksi I2C dilewati bila SDA/SCL terbaca LOW saat idle. Library tetap
memakai timeout untuk membatasi transaksi yang gagal. Background retry hanya
menargetkan OLED/MPU; scan seluruh bus dilakukan atas permintaan `test i2c`.
Tidak ada klaim bahwa ACK mengidentifikasi chip atau membuktikan panel sehat.

## Input, tampilan, dan audio

Encoder dibaca lewat tabel transisi quadrature, empat transisi per langkah.
Tombol memiliki debounce 30 ms dan long press 800 ms. Polling dapat kehilangan
langkah bila encoder diputar sangat cepat selama transfer layar atau DHT.

Sensor touch digital dibaca pada GPIO10 dengan debounce 30 ms. Sentuhan baru
membuka menu, membangunkan karakter, dan menghasilkan bunyi singkat. Sensor
TTP223 dengan keluaran aktif HIGH sesuai konfigurasi; sensor aktif LOW perlu
mengubah `Config::touchActiveHigh`.

Renderer OLED menggunakan framebuffer Adafruit SH110X, animasi mata halus, mood,
menu lima halaman, notifikasi lima detik, dan empat pola diagnostik. Target
frame interval 33 ms; frame rate aktual dipengaruhi perangkat. Setelah idle
120 detik, layar diredupkan dan mode Auto mengantuk. Interaksi membangunkan
mode idle. `sleep` merupakan deep sleep eksplisit; GPIO4 LOW atau RESET
membangunkan board, sedangkan BLE tidak aktif saat deep sleep.

Audio memakai LEDC channel 0 pada GPIO6, dengan deadline untuk menghentikan
nada tanpa delay panjang. Suara karakter otomatis dikompresi mono 8 kHz IMA
ADPCM 4-bit di `include/AutoAudioAsset.h` (sekitar 144 kB untuk satu siklus
36 detik) dan diputar melalui task nonblocking. Titik mulai audio mengikuti
fase animasi OLED, jadi urutan ekspresi dan ketukan tetap selaras. Perintah
`audio auto` memulai siklus dari fase saat ini dan `audio stop` menghentikannya.
Ketika `Mood::Auto` berubah karena akselerometer atau idle, pemutar berpindah
langsung ke segmen mood terkait dan mengulang segmen tersebut sampai mood
berubah lagi. Mood manual juga memakai segmen audio ekspresinya sendiri.
Mute dihormati termasuk pada pengujian audio.

## Sensor dan pengaturan

MPU menggunakan rentang 8 g, gyro 500 derajat/detik, filter 21 Hz. Gerakan
adalah selisih magnitudo percepatan dan baseline gravitasi, diratakan secara
eksponensial. Ambang default 2 m/s2; dua kali ambang memicu ekspresi terkejut.
`calibrate` menetapkan baseline magnitudo pada posisi diam, bukan kalibrasi
bias enam sumbu. Baseline tidak disimpan setelah restart.

DHT memakai library Adafruit dan validasi nilai finite; pembacaan gagal tidak
menampilkan data lama sebagai pembacaan valid. Ubah `Config::dhtType` ke 22
bila perangkat di kemudian hari diganti DHT22.

NVS menyimpan mood, suara, dan sensitivitas hanya ketika pengguna mengubahnya.
Sensor yang belum tersambung bukan alasan memblokir menu atau konsol.

## BLE dan konsol

BLE callback menyalin maksimal 96 byte ke queue satu elemen. Main loop yang
merender dan membunyikan audio, sehingga callback BLE tidak menyentuh bus
I2C atau buffer tampilan. Pesan terbaru menggantikan pesan lama. Status
koneksi dilindungi critical section, advertising dipulihkan setelah disconnect.
Daftar UUID dan batasan integrasi ponsel tersedia di README utama.

Serial menerima CR, LF, CRLF, dan backspace, dengan buffer 128 byte. Baris
yang terlalu panjang ditolak sampai delimiter berikutnya. Maksimal 64 byte
diproses per iterasi untuk menjaga layanan lain. Tidak ada eksekusi perintah
Serial dari BLE. Perintah destruktif konfigurasi seperti reset NVS tidak
disediakan; `reboot` dan `sleep` harus diketik eksplisit.

## Diagnostik

`Diagnostics` memegang state pengujian, bukan loop delay. Tes terpadu
menampilkan pola 1 detik masing-masing lalu memberi 10 detik pengujian input
dan dua nada. Saat tes, animasi/menu tidak menimpa pola. Serial, sensor,
dan pencatatan encoder tetap berjalan. `test stop` mengembalikan tampilan.
Pengujian penuh tetap selesai bila OLED/MPU tidak tersedia. Tidak ada ADC
feedback speaker atau feedback optik layar, sehingga kedua hasil tersebut
memerlukan observasi pengguna.
