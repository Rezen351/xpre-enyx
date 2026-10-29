# Xpre-Enyx

Xpre-Enyx adalah robot pendamping dengan karakter OLED, sensor gerak,
sensor suhu, encoder, suara, dan notifikasi BLE.

## Menjalankan

```powershell
python -m platformio run
python -m platformio run -t upload --upload-port COM15
python -m platformio device monitor --port COM15 --baud 115200
```

Ganti `COM15` jika port board berubah. Tutup aplikasi lain yang sedang
menggunakan port tersebut sebelum upload atau membuka monitor.

## Perintah Serial

Ketik perintah lalu tekan Enter:

```text
help
status
test
```

Perintah yang tersedia:

| Perintah | Fungsi |
|---|---|
| `help` | Menampilkan daftar perintah |
| `status` | Menampilkan kondisi perangkat |
| `test` | Menjalankan semua pengujian |
| `test oled` | Menguji tampilan |
| `test mpu` | Menguji sensor gerak |
| `test dht` | Menguji sensor suhu |
| `test audio` | Menguji suara |
| `test encoder` | Menguji encoder dan tombol |
| `test touch` | Menguji sensor touch |
| `test i2c` | Memeriksa perangkat yang terhubung |
| `test stop` | Menghentikan pengujian |
| `retry` | Mengulangi pemeriksaan perangkat |
| `mood auto` | Mengaktifkan ekspresi otomatis |
| `mood happy` | Memilih ekspresi senang |
| `mood sleepy` | Memilih ekspresi mengantuk |
| `notify Halo!` | Menampilkan pesan |
| `sound on` / `sound off` | Mengaktifkan atau mematikan suara |
| `beep` | Memutar bunyi singkat |
| `menu` / `face` | Membuka menu atau kembali ke karakter |
| `reboot` | Menyalakan ulang board |

## Menggunakan karakter

Klik tombol encoder untuk membuka menu. Putar encoder untuk memilih pilihan,
lalu klik untuk menjalankannya. Tekan dan tahan tombol untuk kembali ke
tampilan karakter. Sentuh sensor touch untuk membuka menu dan membangunkan
karakter.

Karakter dapat berkedip, melirik, tersenyum, terkejut, mengantuk, dan
menampilkan pesan. Pengaturan ekspresi dan suara disimpan otomatis.

## BLE

Cari perangkat bernama `Xpre-Enyx` menggunakan aplikasi BLE. Kirim pesan
untuk menampilkannya pada karakter.

## Dokumentasi

- [Arsitektur](docs/arsitektur.md)
- [Validasi](docs/validasi.md)
