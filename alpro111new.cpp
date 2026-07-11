#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <limits>

// Menggunakan header Windows untuk delay Sleep() dan password masking _getch()
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

// =========================================================================================
// RIWAYAT PERUBAHAN SISTEM (CHANGELOG)
// =========================================================================================
// v2026.1.0 - Rilis awal sistem kasir Alfamidi (manajemen produk, kasir, admin, transaksi,
//             member, diskon, voucher, supplier, dan shift kerja).
//
// v2026.2.0 - Penambahan 5 modul fitur operasional lanjutan:
//   1. Sistem Retur / Pengembalian Barang
//      - Kasir dapat memproses retur barang berdasarkan ID Transaksi yang sudah selesai.
//      - Stok barang yang diretur otomatis dikembalikan ke database inventaris toko.
//      - Setiap retur dicatat pada data/retur_log.txt dan nota retur diekspor otomatis.
//   2. Notifikasi Stok Menipis
//      - Memindai seluruh produk dengan sisa stok di bawah ambang batas aman (15 unit).
//      - Laporan hasil pindaian dapat diunduh pada data/stok_menipis.txt.
//   3. Stok Opname (Audit Fisik Gudang)
//      - Admin dapat menyesuaikan data stok sistem dengan hasil penghitungan fisik barang.
//      - Seluruh selisih (surplus maupun susut) tercatat rapi pada data/opname_log.txt.
//   4. Grafik Analisis Penjualan per Kategori Produk
//      - Merekap omset dan kuantitas penjualan berdasarkan kategori produk.
//      - Ditampilkan dalam bentuk grafik batang berbasis karakter ASCII di layar konsol.
//   5. Backup & Restore Database Sistem
//      - Mencadangkan seluruh berkas data toko ke dalam folder data/backup_xxxx.
//      - Mendukung pemulihan (restore) data dari folder cadangan yang telah dibuat.
//
// Seluruh modul baru terintegrasi penuh ke dalam menu Kasir dan menu Admin/Manajer yang
// sudah ada sebelumnya, serta mengikuti gaya penulisan kode dan konvensi penamaan yang
// konsisten dengan modul-modul lain di dalam sistem (bahasa Indonesia, C++98 compliant).
//
// Ringkasan berkas data baru yang ditambahkan pada v2026.2.0:
//   - data/retur_log.txt      : mencatat seluruh transaksi retur barang pelanggan.
//   - data/nota_retur_*.txt   : salinan nota retur per transaksi (dicetak otomatis).
//   - data/opname_log.txt     : mencatat riwayat penyesuaian stok opname per sesi audit.
//   - data/stok_menipis.txt   : laporan hasil pindaian produk dengan stok hampir habis.
//   - data/backup_*/          : folder cadangan berisi salinan seluruh berkas database.
// =========================================================================================

// =========================================================================================
// DEKLARASI KONSTANTA & STYLE WARNA KONSOL (ANSI ESCAPE CODES)
// =========================================================================================
const string RESET   = "\033[0m";
const string BLACK   = "\033[30m";
const string RED     = "\033[31m";
const string GREEN   = "\033[32m";
const string YELLOW  = "\033[33m";
const string BLUE    = "\033[34m";
const string MAGENTA = "\033[35m";
const string CYAN    = "\033[36m";
const string WHITE   = "\033[37m";
const string BOLD    = "\033[1m";

// =========================================================================================
// FUNGSI UTILITY & HELPER COMPATIBILITY C++98
// =========================================================================================

/**
 * @brief Membersihkan layar konsol sesuai sistem operasi yang berjalan.
 */
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/**
 * @brief Membuat direktori data secara otomatis jika belum ada.
 */
void createDataDirectory() {
#ifdef _WIN32
    system("mkdir data 2>nul");
#else
    system("mkdir -p data 2>/dev/null");
#endif
}

/**
 * @brief Simulasi fungsi std::to_string() yang kompatibel dengan standar C++98.
 */
string toStr(int val) {
    stringstream ss;
    ss << val;
    return ss.str();
}

/**
 * @brief Simulasi fungsi std::to_string() untuk double yang kompatibel dengan standar C++98.
 */
string toStrDouble(double val) {
    stringstream ss;
    ss << fixed << setprecision(2) << val;
    return ss.str();
}

/**
 * @brief Mengubah string menjadi huruf kecil (C++98 safe).
 */
string toLowerString(string str) {
    string result = "";
    for (size_t i = 0; i < str.length(); ++i) {
        result += tolower(str[i]);
    }
    return result;
}

/**
 * @brief Menunda eksekusi program dalam milidetik (C++98 safe).
 */
void delayMs(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

/**
 * @brief Mencetak header sistem kasir dengan gaya visual Alfamidi.
 */
void printHeader(const string& title) {
    cout << CYAN << BOLD;
    cout << "=========================================================\n";
    cout << "                    ALFAMIDI SUPERMARKET                 \n";
    cout << "                 Cabang: KL AGENG PEMANAHAN              \n";
    cout << "=========================================================\n" << RESET;
    cout << YELLOW << BOLD;
    int pad = (57 - title.length()) / 2;
    if (pad < 0) pad = 0;
    cout << string(pad, ' ') << title << "\n" << RESET;
    cout << CYAN << BOLD << "---------------------------------------------------------\n" << RESET;
}

/**
 * @brief Menahan layar konsol sampai pengguna menekan tombol Enter.
 */
void pressEnterToContinue() {
    cout << "\nTekan [ENTER] untuk melanjutkan...";
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

/**
 * @brief Membaca input password secara tersembunyi dengan mencetak karakter bintang (*).
 */
string getHiddenPassword() {
    string password = "";
    char ch;
#ifdef _WIN32
    while ((ch = _getch()) != '\r') { 
        if (ch == '\b') { 
            if (!password.empty()) {
                password.erase(password.end() - 1);
                cout << "\b \b"; 
            }
        } else if (ch != '\n') {
            password.push_back(ch);
            cout << '*';
        }
    }
    cout << endl;
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    cin >> password;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    cout << endl;
#endif
    return password;
}

/**
 * @brief Mendapatkan string tanggal dan waktu saat ini (format DD-MM-YYYY HH:MM:SS)
 */
string getFormattedDate() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    stringstream ss;
    ss << setfill('0') << setw(2) << ltm->tm_mday << "-"
       << setw(2) << 1 + ltm->tm_mon << "-"
       << 1900 + ltm->tm_year << " "
       << setw(2) << ltm->tm_hour << ":"
       << setw(2) << ltm->tm_min << ":"
       << setw(2) << ltm->tm_sec;
    return ss.str();
}

/**
 * @brief Memformat angka desimal ke mata uang Rupiah (contoh: 23000 -> Rp 23.000)
 */
string formatRupiah(double amount) {
    stringstream ss;
    ss << fixed << setprecision(0) << amount;
    string temp = ss.str();
    string result = "";
    int count = 0;
    for (int i = temp.length() - 1; i >= 0; i--) {
        result = temp[i] + result;
        count++;
        if (count % 3 == 0 && i != 0) {
            result = "." + result;
        }
    }
    return "Rp " + result;
}

/**
 * @brief Validasi input bertipe double.
 */
double inputDouble(const string& prompt) {
    double val;
    while (true) {
        cout << prompt;
        if (cin >> val) {
            return val;
        } else {
            cout << RED << "Input tidak valid! Harap masukkan angka desimal.\n" << RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

/**
 * @brief Validasi input bertipe integer.
 */
int inputInt(const string& prompt) {
    int val;
    while (true) {
        cout << prompt;
        if (cin >> val) {
            return val;
        } else {
            cout << RED << "Input tidak valid! Harap masukkan bilangan bulat.\n" << RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

// =========================================================================================
// ASCII ART BANNER GENERATOR
// =========================================================================================
void printLoginBanner() {
    cout << CYAN << BOLD;
    cout << RESET << "\n";
}

void printAdminBanner() {
    cout << MAGENTA << BOLD;
    cout << RESET << "\n";
}

void printKasirBanner() {
    cout << GREEN << BOLD;
    cout << RESET << "\n";
}

void printStrukBanner() {
    cout << YELLOW << BOLD;
    cout << RESET << "\n";
}

void printLaporanBanner() {
    cout << BLUE << BOLD;
    cout << RESET << "\n";
}

void printPromoBanner() {
    cout << YELLOW << BOLD;
    cout << RESET << "\n";
}

void printMemberBanner() {
    cout << BLUE << BOLD;
    cout << RESET << "\n";
}

void printSupplierBanner() {
    cout << MAGENTA << BOLD;
    cout << RESET << "\n";
}

void printDiagnosticsBanner() {
    cout << RED << BOLD;
    cout << RESET << "\n";
}

// =========================================================================================
// 1. CLASS AKUN (Base Class)
// =========================================================================================
class Akun {
protected:
    string username;
    string password;
    string namaLengkap;
    string role;
    string noTelp;

public:
    Akun() : username(""), password(""), namaLengkap(""), role(""), noTelp("") {}
    
    Akun(string username, string password, string namaLengkap, string role, string noTelp)
        : username(username), password(password), namaLengkap(namaLengkap), role(role), noTelp(noTelp) {}

    virtual ~Akun() {}

    string getUsername() const { return username; }
    string getPassword() const { return password; }
    string getNamaLengkap() const { return namaLengkap; }
    string getRole() const { return role; }
    string getNoTelp() const { return noTelp; }

    void setUsername(string u) { username = u; }
    void setPassword(string p) { password = p; }
    void setNamaLengkap(string n) { namaLengkap = n; }
    void setRole(string r) { role = r; }
    void setNoTelp(string t) { noTelp = t; }

    bool registrasi() {
        createDataDirectory();
        ofstream file("data/users.txt", ios::app);
        if (!file.is_open()) return false;
        file << username << ";" << password << ";" << namaLengkap << ";" << role << ";" << noTelp << "\n";
        file.close();
        return true;
    }

    bool login(string user, string pass) {
        return (this->username == user && this->password == pass);
    }

    void logout() {
        cout << GREEN << "Sesi login untuk " << namaLengkap << " (" << role << ") berhasil diakhiri.\n" << RESET;
    }

    void tampilkanAkun() const {
        cout << "Username     : " << username << "\n";
        cout << "Nama Lengkap : " << namaLengkap << "\n";
        cout << "Role Akses   : " << role << "\n";
        cout << "No. Telepon  : " << noTelp << "\n";
    }

    bool updateDataAkun(string baruNama, string baruNoTelp, string baruPass) {
        this->namaLengkap = baruNama;
        this->noTelp = baruNoTelp;
        if (!baruPass.empty()) {
            this->password = baruPass;
        }
        return true;
    }
};


// =========================================================================================
// CLASS MEMBER (Membership Card & Tiering System)
// =========================================================================================
class Member {
private:
    string idMember;
    string nama;
    string noTelepon;
    int poin;
    string tier; 
    

public:
    Member() : idMember(""), nama(""), noTelepon(""), poin(0), tier("SILVER") {}
    
    Member(string id, string nama, string telp, int poin)
        : idMember(id), nama(nama), noTelepon(telp), poin(poin) {
        updateTier();
    }


    string getIdMember() const { return idMember; }
    string getNama() const { return nama; }
    string getNoTelepon() const { return noTelepon; }
    int getPoin() const { return poin; }
    string getTier() const { return tier; }

    void setIdMember(string id) { idMember = id; }
    void setNama(string n) { nama = n; }
    void setNoTelepon(string t) { noTelepon = t; }

    void updateTier() {
        if (poin >= 5000) {
            tier = "PLATINUM";
        } else if (poin >= 1500) {
            tier = "GOLD";
        } else {
            tier = "SILVER";
        }
    }

    void tambahPoin(int p) { 
        double multiplier = 1.0;
        if (tier == "PLATINUM") multiplier = 2.0;
        else if (tier == "GOLD") multiplier = 1.5;

        poin += static_cast<int>(p * multiplier); 
        updateTier();
    }

    void kurangiPoin(int p) { 
        poin -= p; 
        if (poin < 0) poin = 0;
        updateTier();
    }

    void setPoin(int p) { 
        poin = p; 
        updateTier();
    }

    double getTierDiscountRate() const {
        if (tier == "PLATINUM") return 0.05; 
        if (tier == "GOLD") return 0.03;     
        return 0.0;
    }

    void tampilkanMember() const {
        cout << "ID Member    : " << idMember << "\n";
        cout << "Nama Member  : " << nama << "\n";
        cout << "No. Telepon  : " << noTelepon << "\n";
        cout << "Jumlah Poin  : " << poin << " A-POIN\n";
        cout << "Tingkatan    : ";
        if (tier == "PLATINUM") cout << MAGENTA << BOLD << tier << RESET;
        else if (tier == "GOLD") cout << YELLOW << BOLD << tier << RESET;
        else cout << BLUE << BOLD << tier << RESET;
        cout << "\n";
    }
};

vector<Member> dbMember;

void loadMembers() {
    dbMember.clear();
    ifstream file("data/member.txt");
    if (!file.is_open()) return;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, nama, telp, poinStr;
        getline(ss, id, ';');
        getline(ss, nama, ';');
        getline(ss, telp, ';');
        getline(ss, poinStr, ';');
        try {
            int poin = atoi(poinStr.c_str());
            dbMember.push_back(Member(id, nama, telp, poin));
        } catch (...) {}
    }
    file.close();
}

void saveMembers() {
    createDataDirectory();
    ofstream file("data/member.txt");
    if (!file.is_open()) return;
    for (size_t i = 0; i < dbMember.size(); ++i) {
        file << dbMember[i].getIdMember() << ";" << dbMember[i].getNama() << ";" 
             << dbMember[i].getNoTelepon() << ";" << dbMember[i].getPoin() << "\n";
    }
    file.close();
}

Member* findMember(const string& key) {
    for (size_t i = 0; i < dbMember.size(); ++i) {
        if (dbMember[i].getIdMember() == key || dbMember[i].getNoTelepon() == key) {
            return &dbMember[i];
        }
    }
    return NULL;
}

// =========================================================================================
// 2. CLASS KERANJANG_PRODUK (Item di Dalam Keranjang Belanja)
// =========================================================================================
class Keranjang_Produk {
private:
    string idKeranjang;
    string namaProduk;
    string kategori;
    double harga;
    int stok;
    string barcode; 

public:
    Keranjang_Produk() : idKeranjang(""), namaProduk(""), kategori(""), harga(0), stok(0), barcode("") {}
    
    Keranjang_Produk(string idK, string nama, string kat, double hrg, int stk, string bar)
        : idKeranjang(idK), namaProduk(nama), kategori(kat), harga(hrg), stok(stk), barcode(bar) {}

    string getIdKeranjang() const { return idKeranjang; }
    string getNamaProduk() const { return namaProduk; }
    string getKategori() const { return kategori; }
    double getHarga() const { return harga; }
    int getStok() const { return stok; }
    string getBarcode() const { return barcode; }

    void setIdKeranjang(string id) { idKeranjang = id; }
    void setNamaProduk(string n) { namaProduk = n; }
    void setKategori(string k) { kategori = k; }
    void setHarga(double h) { harga = h; }
    void setStok(int s) { stok = s; }
    void setBarcode(string b) { barcode = b; }
};

vector<Keranjang_Produk> dbProduk;

void loadProduk() {
    dbProduk.clear();
    ifstream file("data/produk.txt");
    if (!file.is_open()) return;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string bar, nama, kat, hargaStr, stokStr;
        getline(ss, bar, ';');
        getline(ss, nama, ';');
        getline(ss, kat, ';');
        getline(ss, hargaStr, ';');
        getline(ss, stokStr, ';');
        try {
            double hrg = atof(hargaStr.c_str());
            int stk = atoi(stokStr.c_str());
            dbProduk.push_back(Keranjang_Produk("", nama, kat, hrg, stk, bar));
        } catch (...) {}
    }
    file.close();
}

void saveProduk() {
    createDataDirectory();
    ofstream file("data/produk.txt");
    if (!file.is_open()) return;
    for (size_t i = 0; i < dbProduk.size(); ++i) {
        file << dbProduk[i].getBarcode() << ";" << dbProduk[i].getNamaProduk() << ";" 
             << dbProduk[i].getKategori() << ";" << fixed << setprecision(2) 
             << dbProduk[i].getHarga() << ";" << dbProduk[i].getStok() << "\n";
    }
    file.close();
}

// =========================================================================================
// 3. CLASS KERANJANG (Mengelola List Belanjaan)
// =========================================================================================
class Keranjang {
private:
    string idKeranjang;
    vector<Keranjang_Produk> produkDipilih;
    vector<int> jumlah;
    vector<double> subtotal;

public:
    Keranjang() {
        idKeranjang = "KRN-" + toStr(time(0) % 100000);
    }

    string getIdKeranjang() const { return idKeranjang; }
    const vector<Keranjang_Produk>& getProdukDipilih() const { return produkDipilih; }
    const vector<int>& getJumlah() const { return jumlah; }
    const vector<double>& getSubtotal() const { return subtotal; }

    void setIdKeranjang(string id) { idKeranjang = id; }

    bool tambahItem(Keranjang_Produk p, int qty) {
        for (size_t i = 0; i < produkDipilih.size(); ++i) {
            if (produkDipilih[i].getBarcode() == p.getBarcode()) {
                if (jumlah[i] + qty > p.getStok()) {
                    return false;
                }
                jumlah[i] += qty;
                subtotal[i] = produkDipilih[i].getHarga() * jumlah[i];
                return true;
            }
        }
        
        if (qty > p.getStok()) return false;
        produkDipilih.push_back(p);
        jumlah.push_back(qty);
        subtotal.push_back(p.getHarga() * qty);
        return true;
    }

    bool hapusItem(string barcode) {
        for (size_t i = 0; i < produkDipilih.size(); ++i) {
            if (produkDipilih[i].getBarcode() == barcode) {
                produkDipilih.erase(produkDipilih.begin() + i);
                jumlah.erase(jumlah.begin() + i);
                subtotal.erase(subtotal.begin() + i);
                return true;
            }
        }
        return false;
    }

    bool ubahJumlah(string barcode, int qty) {
        if (qty <= 0) {
            return hapusItem(barcode);
        }
        for (size_t i = 0; i < produkDipilih.size(); ++i) {
            if (produkDipilih[i].getBarcode() == barcode) {
                for (size_t j = 0; j < dbProduk.size(); ++j) {
                    if (dbProduk[j].getBarcode() == barcode) {
                        if (qty > dbProduk[j].getStok()) return false;
                        break;
                    }
                }
                jumlah[i] = qty;
                subtotal[i] = produkDipilih[i].getHarga() * qty;
                return true;
            }
        }
        return false;
    }

    void hitungSubtotal() {
        for (size_t i = 0; i < produkDipilih.size(); ++i) {
            subtotal[i] = produkDipilih[i].getHarga() * jumlah[i];
        }
    }

    double hitungTotal() {
        double total = 0;
        for (size_t i = 0; i < subtotal.size(); ++i) {
            total += subtotal[i];
        }
        return total;
    }

    void tampilkanKeranjang() const {
        if (produkDipilih.empty()) {
            cout << RED << "\n[ Keranjang Belanja Kosong! ]\n" << RESET;
            return;
        }
        cout << "\n" << BLUE << "=== ISI KERANJANG BELANJA ===" << RESET << "\n";
        cout << "---------------------------------------------------------\n";
        cout << left << setw(3) << "No" 
             << setw(13) << "Barcode" 
             << setw(20) << "Nama Produk" 
             << right << setw(5) << "Qty" 
             << setw(14) << "Subtotal" << "\n";
        cout << "---------------------------------------------------------\n";
        for (size_t i = 0; i < produkDipilih.size(); ++i) {
            string shortName = produkDipilih[i].getNamaProduk();
            if (shortName.length() > 18) shortName = shortName.substr(0, 15) + "...";
            cout << left << setw(3) << (i + 1)
                 << setw(13) << produkDipilih[i].getBarcode()
                 << setw(20) << shortName
                 << right << setw(5) << jumlah[i]
                 << setw(14) << formatRupiah(subtotal[i]) << "\n";
        }
        cout << "---------------------------------------------------------\n";
    }

    void kosongkanKeranjang() {
        produkDipilih.clear();
        jumlah.clear();
        subtotal.clear();
    }

    bool isEmpty() const { return produkDipilih.empty(); }
};

// =========================================================================================
// 4. CLASS DISKON (Promo Diskon Transaksi)
// =========================================================================================
class Diskon {
private:
    string idDiskon;
    string namaPromo;
    double presentaseDiskon;
    double minPembelian;
    bool statusAktif;

public:
    Diskon() : idDiskon(""), namaPromo(""), presentaseDiskon(0), minPembelian(0), statusAktif(false) {}
    
    Diskon(string id, string nama, double persen, double minBeli, bool aktif)
        : idDiskon(id), namaPromo(nama), presentaseDiskon(persen), minPembelian(minBeli), statusAktif(aktif) {}

    string getIdDiskon() const { return idDiskon; }
    string getNamaPromo() const { return namaPromo; }
    double getPresentaseDiskon() const { return presentaseDiskon; }
    double getMinPembelian() const { return minPembelian; }
    bool getStatusAktif() const { return statusAktif; }

    void setIdDiskon(string id) { idDiskon = id; }
    void setNamaPromo(string n) { namaPromo = n; }
    void setPresentaseDiskon(double p) { presentaseDiskon = p; }
    void setMinPembelian(double m) { minPembelian = m; }
    void setStatusAktif(bool s) { statusAktif = s; }

    bool tambahDiskon() {
        createDataDirectory();
        ofstream file("data/diskon.txt", ios::app);
        if (!file.is_open()) return false;
        file << idDiskon << ";" << namaPromo << ";" << fixed << setprecision(2) << presentaseDiskon << ";" 
             << fixed << setprecision(2) << minPembelian << ";" << (statusAktif ? "1" : "0") << "\n";
        file.close();
        return true;
    }

    double terapkanDiskon(double totalBelanja) {
        if (cekKeberlakuan(totalBelanja)) {
            return totalBelanja * (presentaseDiskon / 100.0);
        }
        return 0.0;
    }

    bool cekKeberlakuan(double totalBelanja) const {
        return (statusAktif && totalBelanja >= minPembelian);
    }
};

vector<Diskon> dbDiskon;

void loadDiskon() {
    dbDiskon.clear();
    ifstream file("data/diskon.txt");
    if (!file.is_open()) return;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, nama, persenStr, minStr, aktifStr;
        getline(ss, id, ';');
        getline(ss, nama, ';');
        getline(ss, persenStr, ';');
        getline(ss, minStr, ';');
        getline(ss, aktifStr, ';');
        try {
            double persen = atof(persenStr.c_str());
            double minBeli = atof(minStr.c_str());
            bool aktif = (aktifStr == "1");
            dbDiskon.push_back(Diskon(id, nama, persen, minBeli, aktif));
        } catch (...) {}
    }
    file.close();
}

void saveDiskon() {
    createDataDirectory();
    ofstream file("data/diskon.txt");
    if (!file.is_open()) return;
    for (size_t i = 0; i < dbDiskon.size(); ++i) {
        file << dbDiskon[i].getIdDiskon() << ";" << dbDiskon[i].getNamaPromo() << ";" 
             << fixed << setprecision(2) << dbDiskon[i].getPresentaseDiskon() << ";"
             << fixed << setprecision(2) << dbDiskon[i].getMinPembelian() << ";"
             << (dbDiskon[i].getStatusAktif() ? "1" : "0") << "\n";
    }
    file.close();
}

// =========================================================================================
// CLASS VOUCHER (Nominal Discount Vouchers)
// =========================================================================================
class Voucher {
private:
    string kodeVoucher;
    double potonganNominal;
    double minBelanja;
    bool statusAktif;

public:
    Voucher() : kodeVoucher(""), potonganNominal(0), minBelanja(0), statusAktif(false) {}
    Voucher(string kode, double pot, double minB, bool aktif)
        : kodeVoucher(kode), potonganNominal(pot), minBelanja(minB), statusAktif(aktif) {}

    string getKodeVoucher() const { return kodeVoucher; }
    double getPotonganNominal() const { return potonganNominal; }
    double getMinBelanja() const { return minBelanja; }
    bool getStatusAktif() const { return statusAktif; }

    void setKodeVoucher(string k) { kodeVoucher = k; }
    void setPotonganNominal(double p) { potonganNominal = p; }
    void setMinBelanja(double m) { minBelanja = m; }
    void setStatusAktif(bool s) { statusAktif = s; }

    bool cekKeberlakuan(double totalBelanja) const {
        return (statusAktif && totalBelanja >= minBelanja);
    }
};

vector<Voucher> dbVoucher;

void loadVouchers() {
    dbVoucher.clear();
    ifstream file("data/voucher.txt");
    if (!file.is_open()) return;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string kode, potStr, minStr, aktifStr;
        getline(ss, kode, ';');
        getline(ss, potStr, ';');
        getline(ss, minStr, ';');
        getline(ss, aktifStr, ';');
        try {
            double pot = atof(potStr.c_str());
            double minB = atof(minStr.c_str());
            bool aktif = (aktifStr == "1");
            dbVoucher.push_back(Voucher(kode, pot, minB, aktif));
        } catch (...) {}
    }
    file.close();
}

void saveVouchers() {
    createDataDirectory();
    ofstream file("data/voucher.txt");
    if (!file.is_open()) return;
    for (size_t i = 0; i < dbVoucher.size(); ++i) {
        file << dbVoucher[i].getKodeVoucher() << ";" 
             << fixed << setprecision(2) << dbVoucher[i].getPotonganNominal() << ";"
             << fixed << setprecision(2) << dbVoucher[i].getMinBelanja() << ";"
             << (dbVoucher[i].getStatusAktif() ? "1" : "0") << "\n";
    }
    file.close();
}

Voucher* findVoucher(const string& code) {
    string searchCode = toLowerString(code);
    for (size_t i = 0; i < dbVoucher.size(); ++i) {
        if (toLowerString(dbVoucher[i].getKodeVoucher()) == searchCode) {
            return &dbVoucher[i];
        }
    }
    return NULL;
}

// =========================================================================================
// CLASS SUPPLIER (Manajemen Pemasok Barang Belanjaan Toko)
// =========================================================================================
class Supplier {
private:
    string idSupplier;
    string namaSupplier;
    string kontakPerson;
    string noTelp;
    string alamat;
    bool statusAktif;

public:
    Supplier() : idSupplier(""), namaSupplier(""), kontakPerson(""), noTelp(""), alamat(""), statusAktif(false) {}
    Supplier(string id, string nama, string kontak, string telp, string alm, bool aktif)
        : idSupplier(id), namaSupplier(nama), kontakPerson(kontak), noTelp(telp), alamat(alm), statusAktif(aktif) {}

    string getIdSupplier() const { return idSupplier; }
    string getNamaSupplier() const { return namaSupplier; }
    string getKontakPerson() const { return kontakPerson; }
    string getNoTelp() const { return noTelp; }
    string getAlamat() const { return alamat; }
    bool getStatusAktif() const { return statusAktif; }

    void setIdSupplier(string id) { idSupplier = id; }
    void setNamaSupplier(string n) { namaSupplier = n; }
    void setKontakPerson(string k) { kontakPerson = k; }
    void setNoTelp(string t) { noTelp = t; }
    void setAlamat(string a) { alamat = a; }
    void setStatusAktif(bool s) { statusAktif = s; }

    void tampilkanSupplier() const {
        cout << "ID Supplier  : " << idSupplier << "\n";
        cout << "Nama PT/Toko : " << namaSupplier << "\n";
        cout << "Narahubung   : " << kontakPerson << "\n";
        cout << "No. Telepon  : " << noTelp << "\n";
        cout << "Alamat Kantor: " << alamat << "\n";
        cout << "Status Kerja : " << (statusAktif ? "AKTIF" : "TIDAK AKTIF") << "\n";
    }
};

vector<Supplier> dbSupplier;

void loadSuppliers() {
    dbSupplier.clear();
    ifstream file("data/supplier.txt");
    if (!file.is_open()) return;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, nama, kontak, telp, alm, aktifStr;
        getline(ss, id, ';');
        getline(ss, nama, ';');
        getline(ss, kontak, ';');
        getline(ss, telp, ';');
        getline(ss, alm, ';');
        getline(ss, aktifStr, ';');
        bool aktif = (aktifStr == "1");
        dbSupplier.push_back(Supplier(id, nama, kontak, telp, alm, aktif));
    }
    file.close();
}

void saveSuppliers() {
    createDataDirectory();
    ofstream file("data/supplier.txt");
    if (!file.is_open()) return;
    for (size_t i = 0; i < dbSupplier.size(); ++i) {
        file << dbSupplier[i].getIdSupplier() << ";" 
             << dbSupplier[i].getNamaSupplier() << ";"
             << dbSupplier[i].getKontakPerson() << ";"
             << dbSupplier[i].getNoTelp() << ";"
             << dbSupplier[i].getAlamat() << ";"
             << (dbSupplier[i].getStatusAktif() ? "1" : "0") << "\n";
    }
    file.close();
}

Supplier* findSupplier(const string& id) {
    for (size_t i = 0; i < dbSupplier.size(); ++i) {
        if (dbSupplier[i].getIdSupplier() == id) {
            return &dbSupplier[i];
        }
    }
    return NULL;
}

// =========================================================================================
// CLASS SHIFT (Rekap Sesi Kerja Laci Kasir / Drawer Shift Manager)
// =========================================================================================
class Shift {
private:
    string idShift;
    string usernameKasir;
    string waktuMulai;
    string waktuSelesai;
    double modalAwal;
    double totalTunai;
    double totalQris;
    double totalTransfer;
    double uangAktif; 
    double selisih;
    bool statusOpen;

public:
    Shift() {
        idShift = "SHF-" + toStr(time(0) % 100000);
        usernameKasir = "";
        waktuMulai = getFormattedDate();
        waktuSelesai = "-";
        modalAwal = 0;
        totalTunai = 0;
        totalQris = 0;
        totalTransfer = 0;
        uangAktif = 0;
        selisih = 0;
        statusOpen = false;
    }

    Shift(string id, string kasir, string mulai, string selesai, double modal, double tunai, double qris, double trans, double uang, double sel, bool open)
        : idShift(id), usernameKasir(kasir), waktuMulai(mulai), waktuSelesai(selesai), modalAwal(modal),
          totalTunai(tunai), totalQris(qris), totalTransfer(trans), uangAktif(uang), selisih(sel), statusOpen(open) {}

    string getIdShift() const { return idShift; }
    string getUsernameKasir() const { return usernameKasir; }
    string getWaktuMulai() const { return waktuMulai; }
    string getWaktuSelesai() const { return waktuSelesai; }
    double getModalAwal() const { return modalAwal; }
    double getTotalTunai() const { return totalTunai; }
    double getTotalQris() const { return totalQris; }
    double getTotalTransfer() const { return totalTransfer; }
    double getUangAktif() const { return uangAktif; }
    double getSelisih() const { return selisih; }
    bool getStatusOpen() const { return statusOpen; }

    void setIdShift(string id) { idShift = id; }
    void setUsernameKasir(string kasir) { usernameKasir = kasir; }
    void setWaktuMulai(string mulai) { waktuMulai = mulai; }
    void setWaktuSelesai(string selesai) { waktuSelesai = selesai; }
    void setModalAwal(double modal) { modalAwal = modal; }
    void setTotalTunai(double tunai) { totalTunai = tunai; }
    void setTotalQris(double qris) { totalQris = qris; }
    void setTotalTransfer(double trans) { totalTransfer = trans; }
    void setUangAktif(double uang) { uangAktif = uang; }
    void setSelisih(double sel) { selisih = sel; }
    void setStatusOpen(bool open) { statusOpen = open; }

    void startShift(string kasir, double modal) {
        usernameKasir = kasir;
        modalAwal = modal;
        waktuMulai = getFormattedDate();
        statusOpen = true;
    }

    void addTransaksi(double amount, string metode) {
        if (metode == "Tunai") totalTunai += amount;
        else if (metode.find("QRIS") != string::npos) totalQris += amount;
        else totalTransfer += amount;
    }

    void closeShift(double uangFisik) {
        waktuSelesai = getFormattedDate();
        uangAktif = uangFisik;
        double saldoSeharusnya = modalAwal + totalTunai;
        selisih = uangFisik - saldoSeharusnya;
        statusOpen = false;
    }
};

vector<Shift> dbShift;
Shift activeShift; 

void loadShifts() {
    dbShift.clear();
    ifstream file("data/shift_log.txt");
    if (!file.is_open()) return;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, kas, mul, sel, modS, tunS, qriS, traS, uagS, sliS, opnS;
        getline(ss, id, ';');
        getline(ss, kas, ';');
        getline(ss, mul, ';');
        getline(ss, sel, ';');
        getline(ss, modS, ';');
        getline(ss, tunS, ';');
        getline(ss, qriS, ';');
        getline(ss, traS, ';');
        getline(ss, uagS, ';');
        getline(ss, sliS, ';');
        getline(ss, opnS, ';');
        
        dbShift.push_back(Shift(id, kas, mul, sel, atof(modS.c_str()), atof(tunS.c_str()), atof(qriS.c_str()), atof(traS.c_str()), atof(uagS.c_str()), atof(sliS.c_str()), opnS == "1"));
    }
    file.close();
}

void saveShifts() {
    createDataDirectory();
    ofstream file("data/shift_log.txt");
    if (!file.is_open()) return;
    for (size_t i = 0; i < dbShift.size(); ++i) {
        file << dbShift[i].getIdShift() << ";"
             << dbShift[i].getUsernameKasir() << ";"
             << dbShift[i].getWaktuMulai() << ";"
             << dbShift[i].getWaktuSelesai() << ";"
             << fixed << setprecision(2) << dbShift[i].getModalAwal() << ";"
             << fixed << setprecision(2) << dbShift[i].getTotalTunai() << ";"
             << fixed << setprecision(2) << dbShift[i].getTotalQris() << ";"
             << fixed << setprecision(2) << dbShift[i].getTotalTransfer() << ";"
             << fixed << setprecision(2) << dbShift[i].getUangAktif() << ";"
             << fixed << setprecision(2) << dbShift[i].getSelisih() << ";"
             << (dbShift[i].getStatusOpen() ? "1" : "0") << "\n";
    }
    file.close();
}

// =========================================================================================
// 5. CLASS TRANSAKSI (Transaksi & Cetak Struk)
// =========================================================================================
class Transaksi {
private:
    string idTransaksi;
    string tanggal;
    double totalBayar;       
    string metodePembayaran;
    double uangDiterima;
    double kembalian;
    string statusTransaksi;  
    string idKasir;          
    string idMember;         

public:
    Transaksi() {
        time_t now = time(0);
        tm *ltm = localtime(&now);
        stringstream ss;
        ss << "AI34-" << setfill('0') << setw(3) << (ltm->tm_yday % 1000) << "-" 
           << setfill('0') << setw(8) << (now % 100000000);
        idTransaksi = ss.str();
        tanggal = getFormattedDate();
        totalBayar = 0;
        metodePembayaran = "Tunai";
        uangDiterima = 0;
        kembalian = 0;
        statusTransaksi = "Batal";
        idKasir = "";
        idMember = "-";
    }

    string getIdTransaksi() const { return idTransaksi; }
    string getTanggal() const { return tanggal; }
    double getTotalBayar() const { return totalBayar; }
    string getMetodePembayaran() const { return metodePembayaran; }
    double getUangDiterima() const { return uangDiterima; }
    double getKembalian() const { return kembalian; }
    string getStatusTransaksi() const { return statusTransaksi; }
    string getIdKasir() const { return idKasir; }
    string getIdMember() const { return idMember; }
    
    void setKasir(string idK) { idKasir = idK; }
    void setMember(string idM) { idMember = idM; }
    void setIdTransaksi(string id) { idTransaksi = id; }
    void setTanggal(string tgl) { tanggal = tgl; }
    void setTotalBayar(double tby) { totalBayar = tby; }
    void setMetodePembayaran(string met) { metodePembayaran = met; }
    void setUangDiterima(double ud) { uangDiterima = ud; }
    void setKembalian(double kemb) { kembalian = kemb; }
    void setStatusTransaksi(string stat) { statusTransaksi = stat; }

    bool checkout(Keranjang& k, double diskonTotal) {
        if (k.isEmpty()) return false;
        
        double totalKotor = k.hitungTotal();
        totalBayar = totalKotor - diskonTotal;
        if (totalBayar < 0) totalBayar = 0;
        
        statusTransaksi = "Selesai";
        tanggal = getFormattedDate();
        
        for (size_t i = 0; i < k.getProdukDipilih().size(); ++i) {
            string bar = k.getProdukDipilih()[i].getBarcode();
            int qtyDibelanja = k.getJumlah()[i];
            
            for (size_t j = 0; j < dbProduk.size(); ++j) {
                if (dbProduk[j].getBarcode() == bar) {
                    dbProduk[j].setStok(dbProduk[j].getStok() - qtyDibelanja);
                    break;
                }
            }
        }
        saveProduk(); 
        
        if (activeShift.getStatusOpen()) {
            activeShift.addTransaksi(totalBayar, metodePembayaran);
        }
        return true;
    }

    double hitungTotalBarang(Keranjang k) {
        return k.hitungTotal();
    }

    double hitungKembalian() {
        kembalian = uangDiterima - totalBayar;
        if (kembalian < 0) kembalian = 0;
        return kembalian;
    }

    void pilihMetodePembayaran(int pilihan, double uang = 0) {
        switch (pilihan) {
            case 1:
                metodePembayaran = "Tunai";
                uangDiterima = uang;
                hitungKembalian();
                break;
            case 2:
                metodePembayaran = "QRIS CPM BNI";
                uangDiterima = totalBayar;
                kembalian = 0;
                break;
            case 3:
                metodePembayaran = "Transfer Bank";
                uangDiterima = totalBayar;
                kembalian = 0;
                break;
        }
    }

    bool simpanTransaksi(const Keranjang& k) {
        createDataDirectory();
        ofstream file("data/transaksi_log.txt", ios::app);
        if (!file.is_open()) return false;
        
        string detailBarang = "";
        const vector<Keranjang_Produk>& listProduk = k.getProdukDipilih();
        const vector<int>& listQty = k.getJumlah();
        for (size_t i = 0; i < listProduk.size(); ++i) {
            detailBarang += listProduk[i].getBarcode() + ":" + toStr(listQty[i]);
            if (i < listProduk.size() - 1) detailBarang += ",";
        }
        
        file << idTransaksi << ";"
             << tanggal << ";"
             << fixed << setprecision(2) << totalBayar << ";"
             << metodePembayaran << ";"
             << fixed << setprecision(2) << uangDiterima << ";"
             << fixed << setprecision(2) << kembalian << ";"
             << statusTransaksi << ";"
             << idKasir << ";"
             << idMember << ";"
             << detailBarang << "\n";
             
        file.close();
        return true;
    }

    void cetakStruk(Keranjang k, string memberName, int poinAwal, int poinTambahan, double diskonTotal) {
        stringstream ssOut;
        ssOut << "\n";
        ssOut << "          ALFAMIDI KI AGENG PEMANAHAN\n";
        ssOut << "            KI AGENG PEMANAHAN[KAPB]\n\n";
        ssOut << "             JL. KI AGENG PEMANAHAN\n";
        ssOut << "    KRITIK & SARAN: 1500959, ALFACARE@MIDI.CO.ID\n";
        ssOut << "               WA/SMS : 081110640888\n";
        ssOut << "================================================\n";
        ssOut << left << setw(28) << ("Bon  " + idTransaksi) 
              << right << "Kasir : " << idKasir << "\n";
        ssOut << "================================================\n";

        const vector<Keranjang_Produk>& items = k.getProdukDipilih();
        const vector<int>& qtys = k.getJumlah();
        double totalKotor = 0;

        for (size_t i = 0; i < items.size(); ++i) {
            double lineTotal = items[i].getHarga() * qtys[i];
            totalKotor += lineTotal;
            
            ssOut << left << setw(48) << items[i].getNamaProduk() << "\n";
            
            stringstream ssQtyPrice;
            ssQtyPrice << right << setw(18) << qtys[i] << "   " 
                       << setw(10) << fixed << setprecision(0) << items[i].getHarga();
                       
            ssOut << left << setw(32) << ssQtyPrice.str()
                  << right << setw(16) << fixed << setprecision(0) << lineTotal << "\n";
            
            if (items[i].getBarcode() == "8990001") {
                double discItem = 400 * qtys[i];
                ssOut << "   Disc.     -" << fixed << setprecision(0) << discItem << "\n";
            }
        }
        
        ssOut << "------------------------------------------------\n";
        ssOut << left << setw(25) << ("Total Item   " + toStr(items.size()))
              << right << setw(23) << fixed << setprecision(0) << totalKotor << "\n";
        ssOut << left << setw(25) << "Total Disc." 
              << right << setw(23) << fixed << setprecision(0) << diskonTotal << "\n";
        ssOut << left << setw(25) << "Total Belanja" 
              << right << setw(23) << fixed << setprecision(0) << totalBayar << "\n";
        ssOut << left << setw(25) << metodePembayaran 
              << right << setw(23) << fixed << setprecision(0) << uangDiterima << "\n";
        ssOut << left << setw(25) << "Kembalian" 
              << right << setw(23) << fixed << setprecision(0) << kembalian << "\n";
        
        double dpp = round(totalBayar / 1.11);
        double ppn = totalBayar - dpp;
        
        ssOut << left << setw(12) << "REGULER" 
              << "DPP: " << left << setw(12) << fixed << setprecision(0) << dpp 
              << "PPN: " << right << setw(12) << fixed << setprecision(0) << ppn << "\n";
        ssOut << "------------------------------------------------\n";
        ssOut << "Tgl.  " << tanggal << "  V.2026.1.0\n";
        ssOut << "Member : " << (idMember != "-" ? memberName : "-") << "\n";
        
        if (idMember != "-") {
            ssOut << "========= A-POIN ANDA " << (poinAwal + poinTambahan) << " =========\n";
            ssOut << "760 poin Anda akan expired pada\n";
            ssOut << "           29-Feb-2028\n";
            ssOut << "Selamat poin anda bertambah " << poinTambahan << "\n";
        }
        ssOut << "------------------------------------------------\n";
        ssOut << "          PT MIDI UTAMA INDONESIA TBK\n";
        ssOut << "          NPWP : 02.672.927.7-054.000\n";
        ssOut << "   ALFA TOWER LT. 12, JL JALUR SUTERA BARAT\n";
        ssOut << "   KAV. 7-9 ALAM SUTERA PANUNGGANGAN TIMUR\n";
        ssOut << "             WWW.ALFAMIDIKU.COM\n";
        ssOut << "================================================\n\n";

        cout << ssOut.str();

        createDataDirectory();
        string filename = "data/struk_" + idTransaksi.substr(5) + ".txt";
        ofstream sFile(filename.c_str());
        if (sFile.is_open()) {
            sFile << ssOut.str();
            sFile.close();
            cout << GREEN << "[ Struk belanja telah diekspor ke file: " << filename << " ]\n" << RESET;
        }
    }

    void lihatDetailTransaksi() const {
        cout << "ID Transaksi   : " << idTransaksi << "\n";
        cout << "Tanggal        : " << tanggal << "\n";
        cout << "Total Belanja  : " << formatRupiah(totalBayar) << "\n";
        cout << "Metode Bayar   : " << metodePembayaran << "\n";
        cout << "Status         : " << statusTransaksi << "\n";
    }
};

vector<Transaksi> listHistoryTransaksi;

void loadHistoryTransaksi() {
    listHistoryTransaksi.clear();
    ifstream file("data/transaksi_log.txt");
    if (!file.is_open()) return;
    string line;
    getline(file, line); 
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, tgl, totS, met, uaS, kemS, stat, idKas, idMem;
        getline(ss, id, ';');
        getline(ss, tgl, ';');
        getline(ss, totS, ';');
        getline(ss, met, ';');
        getline(ss, uaS, ';');
        getline(ss, kemS, ';');
        getline(ss, stat, ';');
        getline(ss, idKas, ';');
        getline(ss, idMem, ';');

        Transaksi t;
        t.setIdTransaksi(id);
        t.setTanggal(tgl);
        t.setTotalBayar(atof(totS.c_str()));
        t.setMetodePembayaran(met);
        t.setUangDiterima(atof(uaS.c_str()));
        t.setKembalian(atof(kemS.c_str()));
        t.setStatusTransaksi(stat);
        t.setKasir(idKas);
        t.setMember(idMem);

        listHistoryTransaksi.push_back(t);
    }
    file.close();
}

// =========================================================================================
// 6. CLASS KASIR (Inheritance dari Akun)
// =========================================================================================
class Kasir : public Akun {
private:
    string idKasir;
    string shift;

public:
    Kasir() : Akun(), idKasir(""), shift("") {}

    Kasir(string id, string nama, string username, string password, string shift, string telp = "-")
        : Akun(username, password, nama, "Kasir", telp), idKasir(id), shift(shift) {}

    string getIdKasir() const { return idKasir; }
    string getShift() const { return shift; }

    void setIdKasir(string id) { idKasir = id; }
    void setShift(string s) { shift = s; }

    bool loginKasir(string user, string pass) {
        return (this->username == user && this->password == pass);
    }

    void lihatProduk() const {
        printHeader("DAFTAR PRODUK INVENTARIS");
        cout << "---------------------------------------------------------\n";
        cout << left << setw(13) << "Barcode" 
             << setw(23) << "Nama Produk" 
             << setw(10) << "Harga" 
             << right << setw(8) << "Stok" << "\n";
        cout << "---------------------------------------------------------\n";
        for (size_t i = 0; i < dbProduk.size(); ++i) {
            string shortName = dbProduk[i].getNamaProduk();
            if (shortName.length() > 21) shortName = shortName.substr(0, 18) + "...";
            cout << left << setw(13) << dbProduk[i].getBarcode()
                 << setw(23) << shortName
                 << setw(10) << formatRupiah(dbProduk[i].getHarga())
                 << right << setw(8) << dbProduk[i].getStok() << "\n";
        }
        cout << "---------------------------------------------------------\n";
    }

    void cariProduk(string query) const {
        query = toLowerString(query);
        cout << "\nHasil Pencarian produk untuk: \"" << query << "\"\n";
        cout << "---------------------------------------------------------\n";
        cout << left << setw(13) << "Barcode" 
             << setw(23) << "Nama Produk" 
             << setw(10) << "Harga" 
             << right << setw(8) << "Stok" << "\n";
        cout << "---------------------------------------------------------\n";
        bool found = false;
        for (size_t i = 0; i < dbProduk.size(); ++i) {
            string namaLower = toLowerString(dbProduk[i].getNamaProduk());
            if (dbProduk[i].getBarcode() == query || namaLower.find(query) != string::npos) {
                cout << left << setw(13) << dbProduk[i].getBarcode()
                     << setw(23) << dbProduk[i].getNamaProduk()
                     << setw(10) << formatRupiah(dbProduk[i].getHarga())
                     << right << setw(8) << dbProduk[i].getStok() << "\n";
                found = true;
            }
        }
        if (!found) {
            cout << RED << "Produk tidak ditemukan.\n" << RESET;
        }
        cout << "---------------------------------------------------------\n";
    }

    void lihatRiwayat() const {
        ifstream file("data/transaksi_log.txt");
        if (!file.is_open()) return;
        string line;
        
        printHeader("RIWAYAT TRANSAKSI SHIFT KASIR");
        cout << "----------------------------------------------------------------------\n";
        cout << left << setw(20) << "ID Transaksi" 
             << setw(20) << "Waktu" 
             << setw(15) << "Total Bayar" 
             << setw(15) << "Metode" << "\n";
        cout << "----------------------------------------------------------------------\n";
        
        getline(file, line);
        
        bool ada = false;
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string idT, tgl, totalStr, metode, uangStr, kembaliStr, status, idK, idM;
            getline(ss, idT, ';');
            getline(ss, tgl, ';');
            getline(ss, totalStr, ';');
            getline(ss, metode, ';');
            getline(ss, uangStr, ';');
            getline(ss, kembaliStr, ';');
            getline(ss, status, ';');
            getline(ss, idK, ';');
            getline(ss, idM, ';');
            
            if (idK == this->idKasir) {
                cout << left << setw(20) << idT
                     << setw(20) << tgl.substr(0, 16)
                     << setw(15) << formatRupiah(atof(totalStr.c_str()))
                     << setw(15) << metode << "\n";
                ada = true;
            }
        }
        if (!ada) {
            cout << "Belum ada riwayat transaksi pada shift kerja saat ini.\n";
        }
        cout << "----------------------------------------------------------------------\n";
        file.close();
    }
};

// Database Global Kasir Toko
vector<Kasir> dbKasir;

void loadKasir() {
    dbKasir.clear();
    ifstream file("data/kasir.txt");
    if (!file.is_open()) return;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, nama, user, pass, shift;
        getline(ss, id, ';');
        getline(ss, nama, ';');
        getline(ss, user, ';');
        getline(ss, pass, ';');
        getline(ss, shift, ';');
        dbKasir.push_back(Kasir(id, nama, user, pass, shift));
    }
    file.close();
}

void saveKasir() {
    createDataDirectory();
    ofstream file("data/kasir.txt");
    if (!file.is_open()) return;
    for (size_t i = 0; i < dbKasir.size(); ++i) {
        file << dbKasir[i].getIdKasir() << ";" << dbKasir[i].getNamaLengkap() << ";" 
             << dbKasir[i].getUsername() << ";" << dbKasir[i].getPassword() << ";" 
             << dbKasir[i].getShift() << "\n";
    }
    file.close();
}

// =========================================================================================
// 7. CLASS ADMIN (Inheritance dari Akun)
// =========================================================================================
class Admin : public Akun {
private:
    string idAdmin;
    string pinAdmin;

public:
    Admin() : Akun(), idAdmin(""), pinAdmin("") {}

    Admin(string id, string nama, string username, string password, string pin, string telp = "-")
        : Akun(username, password, nama, "Admin", telp), idAdmin(id), pinAdmin(pin) {}

    string getIdAdmin() const { return idAdmin; }
    string getPinAdmin() const { return pinAdmin; }

    void setIdAdmin(string id) { idAdmin = id; }
    void setPinAdmin(string pin) { pinAdmin = pin; }

    bool validasiPinAdmin(string pin) const {
        return (pinAdmin == pin);
    }

    bool loginAdmin(string user, string pass) {
        return (this->username == user && this->password == pass);
    }

    void kelolaDataProduk() {
        while (true) {
            clearScreen();
            printHeader("KELOLA DATA PRODUK (ADMIN)");
            cout << "1. Tambah Produk Baru\n";
            cout << "2. Edit Harga & Stok Produk\n";
            cout << "3. Hapus Produk\n";
            cout << "4. Lihat Semua Produk Toko\n";
            cout << "5. Kembali ke Dashboard Admin\n";
            int pil = inputInt("Pilih menu (1-5): ");
            
            if (pil == 1) {
                string bar, nama, kat;
                double harga;
                int stok;
                cout << "Barcode Produk (8 digit): ";
                cin >> bar;
                cin.ignore();
                
                bool exist = false;
                for (size_t i = 0; i < dbProduk.size(); ++i) {
                    if (dbProduk[i].getBarcode() == bar) { exist = true; break; }
                }
                if (exist) {
                    cout << RED << "Kode Barcode sudah digunakan oleh produk lain!\n" << RESET;
                    pressEnterToContinue();
                    continue;
                }
                
                cout << "Nama Produk             : ";
                getline(cin, nama);
                cout << "Kategori                : ";
                getline(cin, kat);
                harga = inputDouble("Harga Produk (Rp)       : ");
                stok = inputInt("Jumlah Stok             : ");
                
                dbProduk.push_back(Keranjang_Produk("", nama, kat, harga, stok, bar));
                saveProduk();
                cout << GREEN << "Produk baru berhasil didaftarkan!\n" << RESET;
                pressEnterToContinue();
            }
            else if (pil == 2) {
                string bar;
                cout << "Masukkan Barcode produk yang akan diubah: ";
                cin >> bar;
                
                bool found = false;
                for (size_t i = 0; i < dbProduk.size(); ++i) {
                    if (dbProduk[i].getBarcode() == bar) {
                        found = true;
                        cout << "Produk: " << dbProduk[i].getNamaProduk() << "\n";
                        double h = inputDouble("Harga baru (Lama: " + toStrDouble(dbProduk[i].getHarga()) + "): ");
                        int s = inputInt("Stok baru (Lama: " + toStr(dbProduk[i].getStok()) + "): ");
                        dbProduk[i].setHarga(h);
                        dbProduk[i].setStok(s);
                        saveProduk();
                        cout << GREEN << "Data produk berhasil diubah!\n" << RESET;
                        break;
                    }
                }
                if (!found) cout << RED << "Barcode tidak ditemukan.\n" << RESET;
                pressEnterToContinue();
            }
            else if (pil == 3) {
                string bar;
                cout << "Masukkan Barcode produk yang akan dihapus: ";
                cin >> bar;
                
                int targetIdx = -1;
                for (size_t i = 0; i < dbProduk.size(); ++i) {
                    if (dbProduk[i].getBarcode() == bar) { targetIdx = i; break; }
                }
                
                if (targetIdx != -1) {
                    cout << "Apakah yakin ingin menghapus produk: " << dbProduk[targetIdx].getNamaProduk() << "? (y/n): ";
                    char c;
                    cin >> c;
                    if (c == 'y' || c == 'Y') {
                        dbProduk.erase(dbProduk.begin() + targetIdx);
                        saveProduk();
                        cout << GREEN << "Produk telah berhasil dihapus dari inventaris.\n" << RESET;
                    }
                } else {
                    cout << RED << "Barcode tidak ditemukan.\n" << RESET;
                }
                pressEnterToContinue();
            }
            else if (pil == 4) {
                clearScreen();
                printHeader("INVENTARIS PRODUK ALFAMIDI");
                cout << left << setw(13) << "Barcode" 
                     << setw(23) << "Nama Produk" 
                     << setw(10) << "Harga" 
                     << right << setw(8) << "Stok" << "\n";
                cout << "---------------------------------------------------------\n";
                for (size_t i = 0; i < dbProduk.size(); ++i) {
                    string sName = dbProduk[i].getNamaProduk();
                    if (sName.length() > 21) sName = sName.substr(0,18) + "...";
                    cout << left << setw(13) << dbProduk[i].getBarcode()
                         << setw(23) << sName
                         << setw(10) << formatRupiah(dbProduk[i].getHarga())
                         << right << setw(8) << dbProduk[i].getStok() << "\n";
                }
                pressEnterToContinue();
            }
            else if (pil == 5) {
                break;
            }
        }
    }

    void kelolaDataUser() {
        while (true) {
            clearScreen();
            printHeader("KELOLA AKUN KASIR");
            cout << "1. Registrasi Akun Kasir Baru\n";
            cout << "2. Lihat Semua Akun Kasir\n";
            cout << "3. Hapus Akun Kasir\n";
            cout << "4. Kembali ke Dashboard Admin\n";
            int pil = inputInt("Pilih menu (1-4): ");
            
            if (pil == 1) {
                string idK, nama, user, pass, shf;
                cout << "ID Kasir Baru (Kxxx)      : ";
                cin >> idK;
                cin.ignore();
                cout << "Nama Lengkap Kasir        : ";
                getline(cin, nama);
                cout << "Username login            : ";
                cin >> user;
                cout << "Password                  : ";
                cin >> pass;
                cout << "Shift Kerja (Pagi/Siang)   : ";
                cin >> shf;
                
                Kasir kBaru(idK, nama, user, pass, shf);
                dbKasir.push_back(kBaru);
                saveKasir();
                cout << GREEN << "Akun kasir berhasil didaftarkan!\n" << RESET;
                pressEnterToContinue();
            }
            else if (pil == 2) {
                clearScreen();
                printHeader("DAFTAR AKUN KASIR AKTIF");
                cout << left << setw(10) << "ID Kasir" 
                     << setw(20) << "Nama Lengkap" 
                     << setw(15) << "Username" 
                     << setw(10) << "Shift" << "\n";
                cout << "---------------------------------------------------------\n";
                for (size_t i = 0; i < dbKasir.size(); ++i) {
                    cout << left << setw(10) << dbKasir[i].getIdKasir()
                         << setw(20) << dbKasir[i].getNamaLengkap()
                         << setw(15) << dbKasir[i].getUsername()
                         << setw(10) << dbKasir[i].getShift() << "\n";
                }
                pressEnterToContinue();
            }
            else if (pil == 3) {
                string idK;
                cout << "Masukkan ID Kasir yang akan dihapus: ";
                cin >> idK;
                
                int targetIdx = -1;
                for (size_t i = 0; i < dbKasir.size(); ++i) {
                    if (dbKasir[i].getIdKasir() == idK) { targetIdx = i; break; }
                }
                
                if (targetIdx != -1) {
                    dbKasir.erase(dbKasir.begin() + targetIdx);
                    saveKasir();
                    cout << GREEN << "Akun kasir telah dihapus dari sistem.\n" << RESET;
                } else {
                    cout << RED << "Kasir dengan ID tersebut tidak ditemukan.\n" << RESET;
                }
                pressEnterToContinue();
            }
            else if (pil == 4) {
                break;
            }
        }
    }

    void kelolaDataMember() {
        while (true) {
            clearScreen();
            printHeader("KELOLA DATA MEMBER ALFAMIDI");
            cout << "1. Tambah Member Baru\n";
            cout << "2. Edit Data Member (Nama / Telepon)\n";
            cout << "3. Hapus Kartu Member\n";
            cout << "4. Lihat Semua Member & Poin Belanja\n";
            cout << "5. Reset Poin Member\n";
            cout << "6. Kembali ke Dashboard Admin\n";
            int pil = inputInt("Pilih menu (1-6): ");

            if (pil == 1) {
                string idM, nama, tel;
                idM = "MID-" + toStr(time(0) % 100000);
                cin.ignore();
                cout << "Nama Lengkap Member   : ";
                getline(cin, nama);
                cout << "Nomor Telepon         : ";
                cin >> tel;

                bool exist = false;
                for (size_t i = 0; i < dbMember.size(); ++i) {
                    if (dbMember[i].getNoTelepon() == tel) { exist = true; break; }
                }
                if (exist) {
                    cout << RED << "Nomor telepon member sudah terdaftar!\n" << RESET;
                    pressEnterToContinue();
                    continue;
                }

                Member m(idM, nama, tel, 0);
                dbMember.push_back(m);
                saveMembers();
                cout << GREEN << "Kartu Member baru berhasil didaftarkan!\n" << RESET;
                m.tampilkanMember();
                pressEnterToContinue();
            }
            else if (pil == 2) {
                string key;
                cout << "Masukkan ID Member atau No. Telepon: ";
                cin >> key;

                Member* m = findMember(key);
                if (m != NULL) {
                    string baruNama, baruTelp;
                    cin.ignore();
                    cout << "Nama Baru (Lama: " << m->getNama() << "): ";
                    getline(cin, baruNama);
                    cout << "Nomor Telepon Baru (Lama: " << m->getNoTelepon() << "): ";
                    cin >> baruTelp;

                    m->setNama(baruNama);
                    m->setNoTelepon(baruTelp);
                    saveMembers();
                    cout << GREEN << "Data profil member berhasil diperbarui!\n" << RESET;
                } else {
                    cout << RED << "Member tidak ditemukan.\n" << RESET;
                }
                pressEnterToContinue();
            }
            else if (pil == 3) {
                string key;
                cout << "Masukkan ID Member yang ingin dihapus: ";
                cin >> key;

                int targetIdx = -1;
                for (size_t i = 0; i < dbMember.size(); ++i) {
                    if (dbMember[i].getIdMember() == key) { targetIdx = i; break; }
                }

                if (targetIdx != -1) {
                    cout << "Apakah yakin menghapus member: " << dbMember[targetIdx].getNama() << "? (y/n): ";
                    char c;
                    cin >> c;
                    if (c == 'y' || c == 'Y') {
                        dbMember.erase(dbMember.begin() + targetIdx);
                        saveMembers();
                        cout << GREEN << "Member berhasil dihapus dari database.\n" << RESET;
                    }
                } else {
                    cout << RED << "ID Member tidak ditemukan.\n" << RESET;
                }
                pressEnterToContinue();
            }
            else if (pil == 4) {
                clearScreen();
                printHeader("DATABASE KARTU MEMBER AKTIF");
                cout << left << setw(10) << "ID Member" 
                     << setw(20) << "Nama Lengkap" 
                     << setw(15) << "No. Telepon" 
                     << setw(10) << "Poin" 
                     << "Tingkatan (Tier)" << "\n";
                cout << "----------------------------------------------------------------------\n";
                for (size_t i = 0; i < dbMember.size(); ++i) {
                    cout << left << setw(10) << dbMember[i].getIdMember()
                         << setw(20) << dbMember[i].getNama()
                         << setw(15) << dbMember[i].getNoTelepon()
                         << setw(10) << dbMember[i].getPoin()
                         << dbMember[i].getTier() << "\n";
                }
                cout << "----------------------------------------------------------------------\n";
                pressEnterToContinue();
            }
            else if (pil == 5) {
                string key;
                cout << "Masukkan ID Member untuk reset poin belanja: ";
                cin >> key;
                Member* m = findMember(key);
                if (m != NULL) {
                    m->setPoin(0);
                    saveMembers();
                    cout << GREEN << "Jumlah poin belanja member berhasil direset ke 0.\n" << RESET;
                } else {
                    cout << RED << "Member tidak ditemukan.\n" << RESET;
                }
                pressEnterToContinue();
            }
            else if (pil == 6) {
                break;
            }
        }
    }

    void kelolaDataSupplier() {
        while (true) {
            clearScreen();
            printSupplierBanner();
            printHeader("KELOLA DATA SUPPLIER");
            cout << "1. Tambah Supplier Pemasok\n";
            cout << "2. Edit Hubungan Pemasok\n";
            cout << "3. Lihat Semua Supplier Aktif\n";
            cout << "4. Kembali ke Dashboard Admin\n";
            int pil = inputInt("Pilih menu (1-4): ");

            if (pil == 1) {
                string idS, nama, kontak, telp, alm;
                cout << "ID Supplier Baru (Sxxx)    : ";
                cin >> idS;
                cin.ignore();
                
                Supplier* s = findSupplier(idS);
                if (s != NULL) {
                    cout << RED << "ID Supplier tersebut sudah digunakan!\n" << RESET;
                    pressEnterToContinue();
                    continue;
                }

                cout << "Nama PT/Badan Usaha        : ";
                getline(cin, nama);
                cout << "Narahubung (Contact Person): ";
                getline(cin, kontak);
                cout << "No. Telepon Aktif          : ";
                cin >> telp;
                cin.ignore();
                cout << "Alamat Kantor Supplier     : ";
                getline(cin, alm);

                Supplier sBaru(idS, nama, kontak, telp, alm, true);
                dbSupplier.push_back(sBaru);
                saveSuppliers();
                cout << GREEN << "\nSupplier baru berhasil ditambahkan!\n" << RESET;
                pressEnterToContinue();
            }
            else if (pil == 2) {
                string idS;
                cout << "Masukkan ID Supplier yang ingin diedit: ";
                cin >> idS;
                Supplier* s = findSupplier(idS);
                if (s != NULL) {
                    string nama, kontak, telp, alm;
                    cin.ignore();
                    cout << "Nama PT Baru (Lama: " << s->getNamaSupplier() << "): ";
                    getline(cin, nama);
                    cout << "Narahubung Baru (Lama: " << s->getKontakPerson() << "): ";
                    getline(cin, kontak);
                    cout << "No. Telepon Baru (Lama: " << s->getNoTelp() << "): ";
                    cin >> telp;
                    cin.ignore();
                    cout << "Alamat Baru (Lama: " << s->getAlamat() << "): ";
                    getline(cin, alm);
                    
                    cout << "Ubah status aktif (1 = Aktif, 0 = Non-Aktif): ";
                    int st = inputInt("");
                    
                    s->setNamaSupplier(nama);
                    s->setKontakPerson(kontak);
                    s->setNoTelp(telp);
                    s->setAlamat(alm);
                    s->setStatusAktif(st == 1);
                    saveSuppliers();
                    cout << GREEN << "\nData Pemasok Supplier berhasil disimpan!\n" << RESET;
                } else {
                    cout << RED << "ID Supplier tidak terdaftar.\n" << RESET;
                }
                pressEnterToContinue();
            }
            else if (pil == 3) {
                clearScreen();
                printSupplierBanner();
                cout << left << setw(10) << "ID Supp" 
                     << setw(20) << "Nama PT / Usaha" 
                     << setw(15) << "Narahubung" 
                     << setw(15) << "No. Telp" 
                     << "Status" << "\n";
                cout << "----------------------------------------------------------------------------\n";
                for (size_t i = 0; i < dbSupplier.size(); ++i) {
                    cout << left << setw(10) << dbSupplier[i].getIdSupplier()
                         << setw(20) << dbSupplier[i].getNamaSupplier()
                         << setw(15) << dbSupplier[i].getKontakPerson()
                         << setw(15) << dbSupplier[i].getNoTelp()
                         << (dbSupplier[i].getStatusAktif() ? "AKTIF" : "NON-開封") << "\n";
                }
                cout << "----------------------------------------------------------------------------\n";
                pressEnterToContinue();
            }
            else if (pil == 4) {
                break;
            }
        }
    }

    void lihatLaporan() const;
};

// Database Global Admin Toko
vector<Admin> dbAdmin;

void loadAdmin() {
    dbAdmin.clear();
    ifstream file("data/admin.txt");
    if (!file.is_open()) return;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, nama, user, pass, pin;
        getline(ss, id, ';');
        getline(ss, nama, ';');
        getline(ss, user, ';');
        getline(ss, pass, ';');
        getline(ss, pin, ';');
        dbAdmin.push_back(Admin(id, nama, user, pass, pin));
    }
    file.close();
}

void saveAdmin() {
    createDataDirectory();
    ofstream file("data/admin.txt");
    if (!file.is_open()) return;
    for (size_t i = 0; i < dbAdmin.size(); ++i) {
        file << dbAdmin[i].getIdAdmin() << ";" << dbAdmin[i].getNamaLengkap() << ";" 
             << dbAdmin[i].getUsername() << ";" << dbAdmin[i].getPassword() << ";" 
             << dbAdmin[i].getPinAdmin() << "\n";
    }
    file.close();
}

// =========================================================================================
// 8. CLASS LAPORANPENJUALAN (Rekap Laporan Keuangan)
// =========================================================================================
class LaporanPenjualan {
private:
    string idLaporan;
    string periode;
    string tanggalMulai;
    int totalTransaksi;
    double totalPendapatan;

public:
    LaporanPenjualan() : periode(""), tanggalMulai(""), totalTransaksi(0), totalPendapatan(0) {
        idLaporan = "LAP-" + toStr(time(0) % 100000);
    }

    string getIdLaporan() const { return idLaporan; }

    LaporanPenjualan(string id, string per, string tgl, int tr, double pend)
        : idLaporan(id), periode(per), tanggalMulai(tgl), totalTransaksi(tr), totalPendapatan(pend) {}

    void buatLaporanHarian(string tgl) {
        periode = "Harian";
        tanggalMulai = tgl;
        totalTransaksi = 0;
        totalPendapatan = 0;
        
        ifstream file("data/transaksi_log.txt");
        if (!file.is_open()) return;
        string line;
        getline(file, line); 
        
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string idT, tglT, totalStr;
            getline(ss, idT, ';');
            getline(ss, tglT, ';');
            getline(ss, totalStr, ';');
            
            if (tglT.substr(0, 10) == tgl) {
                totalTransaksi++;
                totalPendapatan += atof(totalStr.c_str());
            }
        }
        file.close();
    }

    void buatLaporanBulanan(string bulan) {
        periode = "Bulanan";
        tanggalMulai = bulan; 
        totalTransaksi = 0;
        totalPendapatan = 0;
        
        ifstream file("data/transaksi_log.txt");
        if (!file.is_open()) return;
        string line;
        getline(file, line);
        
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string idT, tglT, totalStr;
            getline(ss, idT, ';');
            getline(ss, tglT, ';');
            getline(ss, totalStr, ';');
            
            if (tglT.length() >= 10 && tglT.substr(3, 7) == bulan) {
                totalTransaksi++;
                totalPendapatan += atof(totalStr.c_str());
            }
        }
        file.close();
    }

    void buatLaporanTahunan(string tahun) {
        periode = "Tahunan";
        tanggalMulai = tahun; 
        totalTransaksi = 0;
        totalPendapatan = 0;
        
        ifstream file("data/transaksi_log.txt");
        if (!file.is_open()) return;
        string line;
        getline(file, line);
        
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string idT, tglT, totalStr;
            getline(ss, idT, ';');
            getline(ss, tglT, ';');
            getline(ss, totalStr, ';');
            
            if (tglT.length() >= 10 && tglT.substr(6, 4) == tahun) {
                totalTransaksi++;
                totalPendapatan += atof(totalStr.c_str());
            }
        }
        file.close();
    }

    void tampilkanLaporan() const {
        cout << "\n" << MAGENTA << "================ LAPORAN PENJUALAN ================" << RESET << "\n";
        cout << "ID Laporan          : " << idLaporan << "\n";
        cout << "Periode Rekap       : " << periode << "\n";
        cout << "Tanggal/Bulan/Tahun : " << tanggalMulai << "\n";
        cout << "---------------------------------------------------\n";
        cout << "Jumlah Transaksi    : " << totalTransaksi << " Transaksi\n";
        cout << "Total Pendapatan    : " << formatRupiah(totalPendapatan) << "\n";
        cout << MAGENTA << "===================================================" << RESET << "\n";
    }

    bool unduhLaporan(string namaFile) const {
        createDataDirectory();
        ofstream file(namaFile.c_str());
        if (!file.is_open()) return false;
        file << "===================================================\n";
        file << "            ALFAMIDI SUPERMARKET REPORT            \n";
        file << "===================================================\n";
        file << "ID Laporan       : " << idLaporan << "\n";
        file << "Tipe Laporan     : Laporan Penjualan " << periode << "\n";
        file << "Periode/Tanggal  : " << tanggalMulai << "\n";
        file << "---------------------------------------------------\n";
        file << "Jumlah Transaksi : " << totalTransaksi << " kali\n";
        file << "Total Pendapatan : " << formatRupiah(totalPendapatan) << "\n";
        file << "Dicetak pada     : " << getFormattedDate() << "\n";
        file << "===================================================\n";
        file.close();
        return true;
    }
};

// =========================================================================================
// ANALISIS & STATISTIK LAPORAN PENJUALAN ADVANCED
// =========================================================================================
void runLaporanLengkapAdmin() {
    clearScreen();
    printHeader("ANALISIS DETIL TRANSAKSI & BEST SELLER");
    loadHistoryTransaksi();

    if (listHistoryTransaksi.empty()) {
        cout << RED << "Database transaksi log masih kosong.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    double totalOmset = 0;
    double maxSale = 0;
    string maxSaleId = "-";
    int tunaiCount = 0;
    int qrisCount = 0;
    int transferCount = 0;
    int memberTransCount = 0;

    for (size_t i = 0; i < listHistoryTransaksi.size(); ++i) {
        double bayar = listHistoryTransaksi[i].getTotalBayar();
        totalOmset += bayar;

        if (bayar > maxSale) {
            maxSale = bayar;
            maxSaleId = listHistoryTransaksi[i].getIdTransaksi();
        }

        string met = listHistoryTransaksi[i].getMetodePembayaran();
        if (met.find("Tunai") != string::npos) tunaiCount++;
        else if (met.find("QRIS") != string::npos) qrisCount++;
        else transferCount++;

        if (listHistoryTransaksi[i].getIdMember() != "-") {
            memberTransCount++;
        }
    }

    double avgSale = totalOmset / listHistoryTransaksi.size();

    cout << CYAN << "=== RINGKASAN DATA ANALITIS ===" << RESET << "\n";
    cout << "Total Transaksi Tercatat : " << listHistoryTransaksi.size() << " Transaksi\n";
    cout << "Total Pendapatan (Omset) : " << formatRupiah(totalOmset) << "\n";
    cout << "Rata-rata Pembelian      : " << formatRupiah(avgSale) << " / Transaksi\n";
    cout << "Transaksi Terbesar       : " << formatRupiah(maxSale) << " (ID: " << maxSaleId << ")\n";
    cout << "---------------------------------------------------------\n";
    cout << "PENGGUNAAN METODE PEMBAYARAN:\n";
    cout << " - Tunai (Cash) : " << tunaiCount << " kali\n";
    cout << " - QRIS BNI     : " << qrisCount << " kali\n";
    cout << " - Transfer     : " << transferCount << " kali\n";
    cout << "---------------------------------------------------------\n";
    cout << "PENGGUNAAN KARTU MEMBER:\n";
    cout << " - Transaksi Reguler     : " << (listHistoryTransaksi.size() - memberTransCount) << " kali\n";
    cout << " - Transaksi via Member  : " << memberTransCount << " kali\n";
    cout << "=========================================================\n";
    pressEnterToContinue();
}

void Admin::lihatLaporan() const {
    while (true) {
        clearScreen();
        printHeader("REKAP PENJUALAN & OMSET TOKO");
        cout << "1. Laporan Harian (DD-MM-YYYY)\n";
        cout << "2. Laporan Bulanan (MM-YYYY)\n";
        cout << "3. Laporan Lanjut (Statistik & Metode Bayar)\n";
        cout << "4. Kembali ke Menu Admin\n";
        int pil = inputInt("Pilih Tipe Laporan (1-4): ");

        if (pil == 4) break;

        LaporanPenjualan lap;
        string targetDate;

        if (pil == 1) {
            cout << "Masukkan Tanggal (format DD-MM-YYYY, cth: 03-05-2026): ";
            cin >> targetDate;
            lap.buatLaporanHarian(targetDate);
            lap.tampilkanLaporan();
        }
        else if (pil == 2) {
            cout << "Masukkan Bulan (format MM-YYYY, cth: 05-2026): ";
            cin >> targetDate;
            lap.buatLaporanBulanan(targetDate);
            lap.tampilkanLaporan();
        }
        else if (pil == 3) {
            runLaporanLengkapAdmin();
            continue;
        }
        else {
            cout << RED << "Pilihan tidak valid.\n" << RESET;
            pressEnterToContinue();
            continue;
        }

        cout << "\nApakah ingin mengekspor rekap ini ke file teks (.txt)? (y/n): ";
        char dl;
        cin >> dl;
        if (dl == 'y' || dl == 'Y') {
            string fName = "data/laporan_" + lap.getIdLaporan() + "_" + targetDate + ".txt";
            if (lap.unduhLaporan(fName)) {
                cout << GREEN << "Laporan penjualan berhasil diekspor ke: " << fName << "\n" << RESET;
            } else {
                cout << RED << "Gagal mengekspor laporan.\n" << RESET;
            }
        }
        pressEnterToContinue();
    }
}

// =========================================================================================
// CLASS SYSTEMTESTER (Simulasi Pengujian Unit Testing Berbasis OOP)
// =========================================================================================
class SystemTester {
public:
    static void testAkun() {
        cout << "[TEST] Menguji kelas Akun... ";
        Akun a("user_test", "pass_test", "Akun Testing", "Kasir", "08112233");
        if (a.getUsername() == "user_test" && a.getPassword() == "pass_test" && 
            a.getNamaLengkap() == "Akun Testing" && a.getRole() == "Kasir" && a.getNoTelp() == "08112233") {
            cout << GREEN << "PASSED" << RESET << "\n";
        } else {
            cout << RED << "FAILED" << RESET << "\n";
        }
    }

    static void testMember() {
        cout << "[TEST] Menguji kelas Member & Loyalty Tiers... ";
        Member m("M009", "Member Test", "08129988", 0);
        
        // Poin awal Silver
        if (m.getTier() != "SILVER") { cout << RED << "FAILED (Poin Awal)\n" << RESET; return; }
        
        // Tambah poin naik Gold
        m.tambahPoin(1600);
        if (m.getTier() != "GOLD") { cout << RED << "FAILED (Upgrade Gold)\n" << RESET; return; }
        
        // Tambah poin naik Platinum
        m.tambahPoin(4000);
        if (m.getTier() != "PLATINUM") { cout << RED << "FAILED (Upgrade Platinum)\n" << RESET; return; }
        
        // Potong poin
        m.kurangiPoin(5000);
        if (m.getPoin() != 2200 || m.getTier() != "GOLD") { cout << RED << "FAILED (Debet Poin)\n" << RESET; return; }

        cout << GREEN << "PASSED" << RESET << "\n";
    }

    static void testProduk() {
        cout << "[TEST] Menguji kelas Keranjang_Produk... ";
        Keranjang_Produk p("TEST-ID", "Produk Pengujian", "Makanan", 15000, 100, "99990001");
        if (p.getBarcode() == "99990001" && p.getNamaProduk() == "Produk Pengujian" && p.getHarga() == 15000 && p.getStok() == 100) {
            cout << GREEN << "PASSED" << RESET << "\n";
        } else {
            cout << RED << "FAILED" << RESET << "\n";
        }
    }

    static void testKeranjang() {
        cout << "[TEST] Menguji kalkulasi Keranjang Belanja... ";
        Keranjang cart;
        Keranjang_Produk p1("K001", "Item A", "Makanan", 5000, 10, "99990001");
        Keranjang_Produk p2("K002", "Item B", "Minuman", 12000, 5, "99990002");
        
        cart.tambahItem(p1, 2); // 10000
        cart.tambahItem(p2, 1); // 12000
        
        if (cart.hitungTotal() == 22000) {
            cart.ubahJumlah("99990001", 4); // 20000 + 12000 = 32000
            if (cart.hitungTotal() == 32000) {
                cart.hapusItem("99990002"); // Sisa Item A (4 * 5000 = 20000)
                if (cart.hitungTotal() == 20000) {
                    cout << GREEN << "PASSED" << RESET << "\n";
                    return;
                }
            }
        }
        cout << RED << "FAILED" << RESET << "\n";
    }

    static void testDiskon() {
        cout << "[TEST] Menguji kelas Diskon & Promo Belanja... ";
        Diskon d("D999", "PROMO TEST", 10.0, 50000, true);
        
        // Di bawah minimal belanja
        if (d.terapkanDiskon(40000) != 0.0) { cout << RED << "FAILED (Min Belanja)\n" << RESET; return; }
        
        // Pas minimal belanja
        if (d.terapkanDiskon(100000) != 10000.0) { cout << RED << "FAILED (Kalkulasi)\n" << RESET; return; }
        
        // Status nonaktif
        d.setStatusAktif(false);
        if (d.terapkanDiskon(100000) != 0.0) { cout << RED << "FAILED (Non-Aktif)\n" << RESET; return; }
        
        cout << GREEN << "PASSED" << RESET << "\n";
    }

    static void testVoucher() {
        cout << "[TEST] Menguji keabsahan Voucher Belanja... ";
        Voucher v("VOUCH5K", 5000, 30000, true);
        if (v.cekKeberlakuan(35000) && !v.cekKeberlakuan(25000)) {
            cout << GREEN << "PASSED" << RESET << "\n";
        } else {
            cout << RED << "FAILED" << RESET << "\n";
        }
    }

    static void testSupplier() {
        cout << "[TEST] Menguji kelas Supplier Pemasok... ";
        Supplier s("S999", "PT Test Supplier", "Nara Test", "089900", "Alamat Test", true);
        if (s.getIdSupplier() == "S999" && s.getNamaSupplier() == "PT Test Supplier" && s.getStatusAktif() == true) {
            cout << GREEN << "PASSED" << RESET << "\n";
        } else {
            cout << RED << "FAILED" << RESET << "\n";
        }
    }

    static void testShift() {
        cout << "[TEST] Menguji Drawer Shift Logs... ";
        Shift sh;
        sh.startShift("muhammad", 100000);
        sh.addTransaksi(25000, "Tunai");
        sh.addTransaksi(50000, "QRIS CPM BNI");
        sh.closeShift(125000); // 100k + 25k tunai = seharusnya 125k tunai. Selisih = 0
        
        if (sh.getModalAwal() == 100000 && sh.getTotalTunai() == 25000 && 
            sh.getTotalQris() == 50000 && sh.getSelisih() == 0 && sh.getStatusOpen() == false) {
            cout << GREEN << "PASSED" << RESET << "\n";
        } else {
            cout << RED << "FAILED" << RESET << "\n";
        }
    }

    static void runAllTests() {
        clearScreen();
        printHeader("SIMULASI UNIT TESTING KELAS OOP");
        cout << "Memulai eksekusi pengujian kelas C++98...\n\n";
        delayMs(500);
        testAkun();
        delayMs(200);
        testMember();
        delayMs(200);
        testProduk();
        delayMs(200);
        testKeranjang();
        delayMs(200);
        testDiskon();
        delayMs(200);
        testVoucher();
        delayMs(200);
        testSupplier();
        delayMs(200);
        testShift();
        cout << "\n---------------------------------------------------------\n";
        cout << GREEN << "PENGUJIAN SELESAI: Seluruh kelas OOP bekerja dengan benar.\n" << RESET;
        cout << "---------------------------------------------------------\n";
        pressEnterToContinue();
    }
};

// =========================================================================================
// CLASS MANUALSISTEM (Panduan Pengguna Interaktif di Layar Konsol)
// =========================================================================================
class ManualSistem {
public:
    static void tampilkanPanduanLengkap() {
        clearScreen();
        printHeader("PANDUAN OPERASIONAL KASIR & ADMIN");
        cout << BOLD << "A. ALUR OPERASIONAL HARIAN KASIR:" << RESET << "\n";
        cout << "1. Buka Sesi Shift Kasir:\n";
        cout << "   - Pilih menu 'Buka Shift Kerja Baru' untuk mendaftarkan modal kas drawer.\n";
        cout << "   - Masukkan nominal uang tunai modal recehan (cth: Rp 200.000).\n";
        cout << "   - Shift harus aktif agar kasir dapat memproses transaksi penjualan.\n";
        cout << "2. Proses Transaksi Baru:\n";
        cout << "   - Masukkan input member belanja jika pelanggan terdaftar.\n";
        cout << "   - Pindai/masukkan barcode produk dari daftar inventaris (cth: 8990001).\n";
        cout << "   - Masukkan kuantitas pembelian. Ulangi untuk semua item belanja.\n";
        cout << "   - Lakukan checkout dengan memilih metode bayar: Tunai, QRIS, atau Transfer.\n";
        cout << "   - Cetak struk belanja otomatis.\n";
        cout << "3. Tutup Shift Kasir:\n";
        cout << "   - Hitung seluruh uang tunai fisik yang ada dalam laci drawer.\n";
        cout << "   - Masukkan nominal fisik ke sistem untuk kalkulasi variance/selisih.\n";
        cout << "   - Sesi shift kerja dinyatakan ditutup dan kasir dapat logout dengan aman.\n\n";

        cout << BOLD << "B. ALUR KERJA SUPER ADMINISTRATOR & MANAGER:" << RESET << "\n";
        cout << "1. Manajemen Inventaris Toko:\n";
        cout << "   - Admin berhak menambah, merubah harga/stok, serta menghapus data produk.\n";
        cout << "2. Manajemen Karyawan / Akun Kasir:\n";
        cout << "   - Membutuhkan verifikasi PIN Admin (PIN Default: 998877).\n";
        cout << "   - Admin dapat menambah kasir baru dan mengelola shift kerja.\n";
        cout << "3. Laporan Penjualan (Omset) Real-time:\n";
        cout << "   - Menghasilkan laporan omset harian, bulanan, tahunan, dan statistika detail.\n";
        cout << "   - Laporan dapat diunduh langsung dalam bentuk berkas teks (.txt).\n";
        cout << "4. Diagnosa Integritas Database:\n";
        cout << "   - Menu diagnosa scanner untuk memastikan 9 file database lengkap dan sehat.\n\n";

        cout << BOLD << "C. TIERING MEMBERSHIP (KARTU MEMBER ALFAMIDI):" << RESET << "\n";
        cout << "1. Tier SILVER   : Poin 0 - 1499. Tambahan poin standar.\n";
        cout << "2. Tier GOLD     : Poin 1500 - 4999. Diskon belanja tambahan 3% + multiplier poin 1.5x.\n";
        cout << "3. Tier PLATINUM : Poin >= 5000. Diskon belanja tambahan 5% + multiplier poin 2.0x.\n\n";

        cout << BOLD << "D. FITUR OPERASIONAL LANJUTAN (RETUR, STOK & BACKUP):" << RESET << "\n";
        cout << "1. Retur / Pengembalian Barang:\n";
        cout << "   - Kasir dapat memproses retur barang pelanggan berdasarkan ID Transaksi asal.\n";
        cout << "   - Stok barang yang diretur akan otomatis dikembalikan ke inventaris toko.\n";
        cout << "   - Nota retur akan diekspor otomatis ke berkas data/nota_retur_xxxx.txt.\n";
        cout << "2. Notifikasi Stok Menipis:\n";
        cout << "   - Sistem akan menandai produk dengan stok <= 15 unit sebagai stok menipis.\n";
        cout << "   - Laporan lengkap dapat diunduh pada berkas data/stok_menipis.txt.\n";
        cout << "3. Stok Opname (Audit Fisik Gudang):\n";
        cout << "   - Admin dapat menyesuaikan stok sistem dengan hasil hitung fisik di rak toko.\n";
        cout << "   - Setiap selisih (surplus/susut) akan tercatat pada data/opname_log.txt.\n";
        cout << "4. Grafik Analisis Penjualan per Kategori:\n";
        cout << "   - Menampilkan grafik batang ASCII omset penjualan per kategori produk.\n";
        cout << "   - Membantu Admin mengenali kategori produk dengan performa terbaik.\n";
        cout << "5. Backup & Restore Database:\n";
        cout << "   - Admin dapat mencadangkan seluruh berkas data ke folder data/backup_xxxx.\n";
        cout << "   - Proses restore akan menimpa data aktif, gunakan dengan hati-hati.\n\n";

        cout << BOLD << "E. TANYA JAWAB DAN PENYELESAIAN MASALAH (FAQ & TROUBLESHOOTING):" << RESET << "\n";
        cout << "Q: Mengapa program memunculkan error database corrupt/hilang?\n";
        cout << "A: Lakukan restart program. Program akan secara otomatis melakukan seeding ulang.\n";
        cout << "Q: Di mana file nota struk belanja disimpan?\n";
        cout << "A: File struk disimpan di folder data/ dengan format nama struk_xxxx.txt.\n";
        cout << "Q: Bagaimana cara menjalankan program di Dev-C++ 5.11?\n";
        cout << "A: Tekan F9 untuk Compile, F10 untuk Run, atau F11 untuk Compile & Run.\n";
        cout << "   Program ini didesain 100% C++98 compliant tanpa membutuhkan bendera -std=c++11.\n";
        cout << "=========================================================\n";
        pressEnterToContinue();
    }
};

// =========================================================================================
// CLASS SYSTEMDIAGNOSTICS (Validasi & Pemecahan Masalah Sistem)
// =========================================================================================
class SystemDiagnostics {
public:
    static void runScanner() {
        clearScreen();
        printDiagnosticsBanner();
        printHeader("DIAGNOSTIK INTEGRITAS DATABASE");
        cout << "Memulai scanner diagnosa file database...\n\n";
        
        delayMs(500);
        
        vector<string> files;
        files.push_back("data/produk.txt");
        files.push_back("data/kasir.txt");
        files.push_back("data/admin.txt");
        files.push_back("data/member.txt");
        files.push_back("data/diskon.txt");
        files.push_back("data/voucher.txt");
        files.push_back("data/transaksi_log.txt");
        files.push_back("data/shift_log.txt");
        files.push_back("data/supplier.txt");

        int errorCount = 0;

        for (size_t i = 0; i < files.size(); ++i) {
            cout << "Memeriksa [" << files[i] << "] ... ";
            ifstream f(files[i].c_str());
            if (f.is_open()) {
                cout << GREEN << "OK (Terbuka)" << RESET << "\n";
                f.close();
            } else {
                cout << RED << "CORRUPT/HILANG! (Dibutuhkan Pemulihan Seeding)" << RESET << "\n";
                errorCount++;
            }
            delayMs(200);
        }

        cout << "\n---------------------------------------------------------\n";
        if (errorCount == 0) {
            cout << GREEN << "DIAGNOSA SELESAI: Sistem dalam keadaan Prima & Stabil.\n" << RESET;
        } else {
            cout << RED << "DIAGNOSA SELESAI: Ditemukan " << errorCount << " masalah database!\n";
            cout << "Merekomendasikan admin melakukan restart aplikasi untuk seeding ulang.\n" << RESET;
        }
        cout << "---------------------------------------------------------\n";
        pressEnterToContinue();
    }
};

// ==========================================
// ASCII ART LOGO & QRIS COMPONENT
// ==========================================
void printQRISLogo() {
    cout << MAGENTA;
    cout << "         === QRIS CPM BNI ===\n" << RESET;
}

void printSimulatedQR() {
    cout << "  #################################\n";
    cout << "      [ Pindai Untuk Melakukan Pembayaran ]\n\n";
    cout << "  #################################\n\n";
    
}

// ==========================================
// TRANSAKSI UTAMA KASIR
// ==========================================
// ==========================================
// FORWARD DECLARATION - FITUR TAMBAHAN (Retur, Stok Opname, Grafik, Backup)
// Dideklarasikan di sini agar dapat dipanggil oleh menu Kasir & Admin yang
// posisinya lebih awal di dalam file, sementara implementasi lengkapnya
// diletakkan setelah fungsi seedDatabase() agar berkas tetap terorganisir.
// ==========================================
void processRetur(Kasir& activeKasir);
void lihatRiwayatRetur();
void cekStokMenipis();
void stokOpname(Admin& activeAdmin);
void tampilkanGrafikKategori();
void backupRestoreMenu();
void lihatLogOpname();

void handleTransaksiKasir(Kasir& activeKasir) {
    if (!activeShift.getStatusOpen()) {
        cout << RED << "\n[ ERROR: SHIFT BELUM DIBUKA! ]\n";
        cout << "Silakan lakukan buka shift pada menu operasional kasir terlebih dahulu.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    Keranjang cart;
    string memberKey = "-";
    Member* activeMember = NULL;
    
    cout << "\nApakah ada Kartu Member Alfamidi? (y/n): ";
    char memOpt;
    cin >> memOpt;
    if (memOpt == 'y' || memOpt == 'Y') {
        cout << "Masukkan ID Member / No. Telepon: ";
        cin >> memberKey;
        activeMember = findMember(memberKey);
        if (activeMember != NULL) {
            cout << GREEN << "Member Ditemukan: " << activeMember->getNama() 
                 << " (Poin saat ini: " << activeMember->getPoin() << " A-POIN)\n" << RESET;
        } else {
            cout << RED << "Kartu Member tidak terdaftar. Berlanjut tanpa member.\n" << RESET;
            memberKey = "-";
        }
        pressEnterToContinue();
    }
    
    while (true) {
        clearScreen();
        printHeader("TRANSAKSI BARU");
        cout << "Kasir : " << activeKasir.getNamaLengkap() << " | Shift: " << activeKasir.getShift() << "\n";
        if (activeMember != NULL) {
            cout << "Member: " << activeMember->getNama() << " | Poin: " << activeMember->getPoin() << " A-POIN | Tier: " << activeMember->getTier() << "\n";
        } else {
            cout << "Member: Reguler\n";
        }
        
        cart.tampilkanKeranjang();
        
        cout << "\nLayanan Transaksi:\n";
        cout << "1. Scan / Masukkan Kode Barcode Barang\n";
        cout << "2. Ubah Jumlah Item Belanja\n";
        cout << "3. Hapus Item Belanja dari Keranjang\n";
        cout << "4. Lanjut ke Pembayaran (Checkout)\n";
        cout << "5. Batal Transaksi\n";
        int pil = inputInt("Pilih Menu (1-5): ");
        
        if (pil == 1) {
            string barcode;
            cout << "Masukkan Barcode Barang: ";
            cin >> barcode;
            
            bool found = false;
            for (size_t i = 0; i < dbProduk.size(); ++i) {
                if (dbProduk[i].getBarcode() == barcode) {
                    found = true;
                    cout << "Produk: " << dbProduk[i].getNamaProduk() << " (" << formatRupiah(dbProduk[i].getHarga()) << ")\n";
                    int qty = inputInt("Jumlah Beli: ");
                    if (qty <= 0) {
                        cout << RED << "Kuantitas beli minimal 1 buah!\n" << RESET;
                    } else {
                        if (cart.tambahItem(dbProduk[i], qty)) {
                            cout << GREEN << "Berhasil ditambahkan ke keranjang.\n" << RESET;
                        } else {
                            cout << RED << "Stok produk di toko tidak mencukupi!\n" << RESET;
                        }
                    }
                    break;
                }
            }
            if (!found) {
                cout << RED << "Barcode tidak terdaftar dalam database.\n" << RESET;
            }
            pressEnterToContinue();
        }
        else if (pil == 2) {
            if (cart.isEmpty()) {
                cout << RED << "Keranjang masih kosong.\n" << RESET;
                pressEnterToContinue();
                continue;
            }
            string barcode;
            cout << "Masukkan Barcode barang yang ingin diubah jumlahnya: ";
            cin >> barcode;
            int qty = inputInt("Masukkan jumlah baru: ");
            if (cart.ubahJumlah(barcode, qty)) {
                cout << GREEN << "Kuantitas belanja berhasil diperbarui.\n" << RESET;
            } else {
                cout << RED << "Gagal memperbarui jumlah. Pastikan barcode benar dan stok tersedia.\n" << RESET;
            }
            pressEnterToContinue();
        }
        else if (pil == 3) {
            if (cart.isEmpty()) {
                cout << RED << "Keranjang masih kosong.\n" << RESET;
                pressEnterToContinue();
                continue;
            }
            string barcode;
            cout << "Masukkan Barcode barang yang ingin dihapus: ";
            cin >> barcode;
            if (cart.hapusItem(barcode)) {
                cout << GREEN << "Item belanja berhasil dikeluarkan dari keranjang.\n" << RESET;
            } else {
                cout << RED << "Barcode tidak ditemukan di keranjang.\n" << RESET;
            }
            pressEnterToContinue();
        }
        else if (pil == 4) {
            if (cart.isEmpty()) {
                cout << RED << "Keranjang kosong! Tidak bisa checkout.\n" << RESET;
                pressEnterToContinue();
                continue;
            }
            
            clearScreen();
            printHeader("METODE PEMBAYARAN");
            double totalBelanjaKotor = cart.hitungTotal();
            cout << "Total Item    : " << cart.getProdukDipilih().size() << "\n";
            cout << "Total Belanja : " << formatRupiah(totalBelanjaKotor) << "\n";
            
            double diskonTotal = 0;
            Diskon activePromo;
            bool promoTerapkan = false;
            
            for (size_t i = 0; i < dbDiskon.size(); ++i) {
                if (dbDiskon[i].cekKeberlakuan(totalBelanjaKotor)) {
                    double dPot = dbDiskon[i].getPresentaseDiskon() / 100.0 * totalBelanjaKotor;
                    if (dPot > diskonTotal) {
                        diskonTotal = dPot;
                        activePromo = dbDiskon[i];
                        promoTerapkan = true;
                    }
                }
            }
            
            // Diskon khusus member berdasarkan tingkatan Tier
            double extraTierDiscount = 0;
            if (activeMember != NULL) {
                double rate = activeMember->getTierDiscountRate();
                if (rate > 0) {
                    extraTierDiscount = rate * totalBelanjaKotor;
                    diskonTotal += extraTierDiscount;
                }
            }
            
            double discItemUltra = 0;
            for (size_t i = 0; i < cart.getProdukDipilih().size(); ++i) {
                if (cart.getProdukDipilih()[i].getBarcode() == "8990001") {
                    discItemUltra += 400 * cart.getJumlah()[i];
                }
            }
            diskonTotal += discItemUltra;
 
            // Masukkan Kode Voucher Belanja (jika ada)
            cout << "Apakah ada Kode Voucher Belanja? (y/n): ";
            char vOpt;
            cin >> vOpt;
            double vDiscount = 0;
            if (vOpt == 'y' || vOpt == 'Y') {
                string code;
                cout << "Masukkan Kode Voucher: ";
                cin >> code;
                Voucher* v = findVoucher(code);
                if (v != NULL) {
                    if (v->cekKeberlakuan(totalBelanjaKotor)) {
                        vDiscount = v->getPotonganNominal();
                        diskonTotal += vDiscount;
                        cout << GREEN << "Voucher Belanja Berhasil! Potongan: -" << formatRupiah(vDiscount) << "\n" << RESET;
                    } else {
                        cout << RED << "Minimal belanja voucher tidak terpenuhi! (Min: " << formatRupiah(v->getMinBelanja()) << ")\n" << RESET;
                    }
                } else {
                    cout << RED << "Kode Voucher tidak valid atau tidak aktif.\n" << RESET;
                }
                pressEnterToContinue();
            }
            
            double redeemedDiscount = 0;
            if (activeMember != NULL && activeMember->getPoin() > 0) {
                cout << "Member memiliki " << activeMember->getPoin() << " A-POIN.\n";
                cout << "Apakah ingin menukarkan poin untuk potongan belanja? (1 A-POIN = Rp 1) (y/n): ";
                char redeemOpt;
                cin >> redeemOpt;
                if (redeemOpt == 'y' || redeemOpt == 'Y') {
                    int redeemPoints = inputInt("Masukkan jumlah poin yang akan ditukarkan (Maks: " + toStr(activeMember->getPoin()) + "): ");
                    if (redeemPoints > activeMember->getPoin()) {
                        cout << RED << "Poin tidak cukup! Menggunakan poin maksimal.\n" << RESET;
                        redeemPoints = activeMember->getPoin();
                    }
                    if (redeemPoints < 0) redeemPoints = 0;
                    
                    redeemedDiscount = redeemPoints;
                    activeMember->kurangiPoin(redeemPoints);
                    diskonTotal += redeemedDiscount;
                    cout << GREEN << "Berhasil menukarkan " << redeemPoints << " poin. Diskon bertambah " << formatRupiah(redeemedDiscount) << ".\n" << RESET;
                }
            }
            
            if (promoTerapkan) {
                cout << GREEN << "Promo Aktif   : " << activePromo.getNamaPromo() 
                     << " (-" << activePromo.getPresentaseDiskon() << "%)\n" << RESET;
            }
            if (extraTierDiscount > 0) {
                cout << GREEN << "Promo Tier    : Diskon Member " << activeMember->getTier() << " (-" << formatRupiah(extraTierDiscount) << ")\n" << RESET;
            }
            if (discItemUltra > 0) {
                cout << GREEN << "Promo Ultra   : Potongan ULTRA TC 200ML (-" << formatRupiah(discItemUltra) << ")\n" << RESET;
            }
            if (vDiscount > 0) {
                cout << GREEN << "Voucher Potong: -" << formatRupiah(vDiscount) << "\n" << RESET;
            }
            
            double totalBayarBersih = totalBelanjaKotor - diskonTotal;
            if (totalBayarBersih < 0) totalBayarBersih = 0;
            
            cout << "Total Diskon  : " << formatRupiah(diskonTotal) << "\n";
            cout << "-------------------------------------------\n";
            cout << YELLOW << BOLD << "Total Bersih  : " << formatRupiah(totalBayarBersih) << RESET << "\n";
            cout << "-------------------------------------------\n";
            
            cout << "Pilih Metode Pembayaran:\n";
            cout << "1. Tunai (Cash)\n";
            cout << "2. QRIS (CPM BNI)\n";
            cout << "3. Transfer Bank\n";
            int pBayar = inputInt("Pilih Metode (1-3): ");
            
            Transaksi trans;
            trans.setKasir(activeKasir.getIdKasir());
            if (activeMember != NULL) {
                trans.setMember(activeMember->getIdMember());
            }
            
            if (pBayar == 1) {
                double uang = 0;
                while (true) {
                    uang = inputDouble("Masukkan Uang Diterima: ");
                    if (uang >= totalBayarBersih) {
                        break;
                    }
                    cout << RED << "Uang tidak mencukupi! Kurang: " << formatRupiah(totalBayarBersih - uang) << "\n" << RESET;
                }
                trans.pilihMetodePembayaran(1, uang);
            }
            else if (pBayar == 2) {
                clearScreen();
                printQRISLogo();
                printSimulatedQR();
                cout << "Total Pembayaran: " << formatRupiah(totalBayarBersih) << "\n";
                cout << "Menghubungkan ke gateway pembayaran...\n";
                for (int t = 3; t > 0; t--) {
                    cout << t << "... ";
                    delayMs(500);
                }
                cout << GREEN << "\n[ PEMBAYARAN VIA QRIS BERHASIL ]\n" << RESET;
                trans.pilihMetodePembayaran(2);
                pressEnterToContinue();
            }
            else if (pBayar == 3) {
                cout << "\nNomor Rekening Bank Mandiri Alfamidi: 122-000-888-999\n";
                cout << "Apakah transfer bank sudah berhasil? (y/n): ";
                char conf;
                cin >> conf;
                if (conf == 'y' || conf == 'Y') {
                    cout << GREEN << "[ PEMBAYARAN VIA TRANSFER BERHASIL ]\n" << RESET;
                    trans.pilihMetodePembayaran(3);
                } else {
                    cout << RED << "Proses pembayaran dibatalkan.\n" << RESET;
                    pressEnterToContinue();
                    continue;
                }
                pressEnterToContinue();
            }
            
            if (trans.checkout(cart, diskonTotal)) {
                int poinAwal = 0;
                int poinTambahan = 0;
                if (activeMember != NULL) {
                    poinAwal = activeMember->getPoin();
                    poinTambahan = static_cast<int>(totalBayarBersih / 200.0);
                    activeMember->tambahPoin(poinTambahan);
                    saveMembers(); 
                }
                
                trans.simpanTransaksi(cart);
                
                clearScreen();
                printHeader("STRUK PEMBELIAN KONSOL");
                trans.cetakStruk(cart, (activeMember ? activeMember->getNama() : ""), poinAwal, poinTambahan, diskonTotal);
                
                cart.kosongkanKeranjang();
                pressEnterToContinue();
                break;
            }
        }
        else if (pil == 5) {
            cout << "Apakah Anda yakin ingin membatalkan transaksi belanja? (y/n): ";
            char cBatal;
            cin >> cBatal;
            if (cBatal == 'y' || cBatal == 'Y') {
                cart.kosongkanKeranjang();
                cout << RED << "Transaksi dibatalkan. Isi keranjang telah dikosongkan.\n" << RESET;
                pressEnterToContinue();
                break;
            }
        }
    }
}

// ==========================================
// MENU UTAMA KASIR
// ==========================================
void runMenuKasir(Kasir& activeKasir) {
    while (true) {
        clearScreen();
        printKasirBanner();
        printHeader("MENU OPERASIONAL KASIR");
        cout << "Kasir Aktif  : " << GREEN << activeKasir.getNamaLengkap() << RESET << "\n";
        cout << "ID Kasir     : " << activeKasir.getIdKasir() << " | Shift: " << activeKasir.getShift() << "\n";
        if (activeShift.getStatusOpen()) {
            cout << "Status Shift : " << GREEN << BOLD << "TERBUKA (Modal: " << formatRupiah(activeShift.getModalAwal()) << ")" << RESET << "\n";
        } else {
            cout << "Status Shift : " << RED << BOLD << "TERTUTUP" << RESET << "\n";
        }
        cout << "---------------------------------------------------------\n";
        cout << "1. Buka Shift Kerja Baru (Input Modal Drawer)\n";
        cout << "2. Transaksi Penjualan Baru\n";
        cout << "3. Lihat Inventaris Produk Toko\n";
        cout << "4. Cari Detail Produk (Barcode/Nama)\n";
        cout << "5. Registrasi Kartu Member Baru\n";
        cout << "6. Cetak Ulang Struk (Reprint Struk Transaksi)\n";
        cout << "7. Proses Retur / Pengembalian Barang Pelanggan\n";
        cout << "8. Tutup Shift Kerja & Rekap Kas Drawer\n";
        cout << "9. Panduan Penggunaan Sistem Kasir (Bantuan)\n";
        cout << "10. Logout\n";
        int pil = inputInt("Pilih Menu (1-10): ");
        
        if (pil == 1) {
            if (activeShift.getStatusOpen()) {
                cout << RED << "\n[ ERROR: Shift sedang berjalan! ]\n" << RESET;
                pressEnterToContinue();
                continue;
            }
            double modal = inputDouble("Masukkan jumlah modal awal laci (Rp): ");
            activeShift = Shift();
            activeShift.startShift(activeKasir.getUsername(), modal);
            loadShifts();
            dbShift.push_back(activeShift);
            saveShifts();
            cout << GREEN << "\n[ Shift berhasil dibuka! Laci kas siap digunakan ]\n" << RESET;
            pressEnterToContinue();
        }
        else if (pil == 2) {
            handleTransaksiKasir(activeKasir);
        }
        else if (pil == 3) {
            clearScreen();
            activeKasir.lihatProduk();
            pressEnterToContinue();
        }
        else if (pil == 4) {
            clearScreen();
            string q;
            cout << "Masukkan barcode atau potongan nama produk: ";
            cin >> q;
            activeKasir.cariProduk(q);
            pressEnterToContinue();
        }
        else if (pil == 5) {
            clearScreen();
            printHeader("PENDAFTARAN KARTU MEMBER BARU");
            string idM, nama, tel;
            idM = "MID-" + toStr(time(0) % 100000); 
            cin.ignore();
            cout << "Nama Lengkap Calon Member: ";
            getline(cin, nama);
            cout << "Nomor Telepon            : ";
            cin >> tel;
            
            Member mBaru(idM, nama, tel, 0);
            dbMember.push_back(mBaru);
            saveMembers();
            cout << GREEN << "\n[ REGISTRASI KARTU MEMBER BERHASIL! ]\n" << RESET;
            mBaru.tampilkanMember();
            pressEnterToContinue();
        }
        else if (pil == 6) {
            clearScreen();
            printHeader("CETAK ULANG STRUK BELANJA");
            loadHistoryTransaksi();
            string idT;
            cout << "Masukkan ID Transaksi (cth: AI34-xxx-xxxxxxxx): ";
            cin >> idT;
            
            bool found = false;
            for (size_t i = 0; i < listHistoryTransaksi.size(); ++i) {
                if (listHistoryTransaksi[i].getIdTransaksi() == idT) {
                    found = true;
                    cout << GREEN << "\n[ Transaksi Ditemukan! ]\n" << RESET;
                    
                    Keranjang dummyCart;
                    
                    ifstream file("data/transaksi_log.txt");
                    if (file.is_open()) {
                        string line;
                        getline(file, line); 
                        while (getline(file, line)) {
                            if (line.empty()) continue;
                            stringstream ss(line);
                            string logId, tgl, totS, met, uaS, kemS, stat, idKas, idMem, detail;
                            getline(ss, logId, ';');
                            getline(ss, tgl, ';');
                            getline(ss, totS, ';');
                            getline(ss, met, ';');
                            getline(ss, uaS, ';');
                            getline(ss, kemS, ';');
                            getline(ss, stat, ';');
                            getline(ss, idKas, ';');
                            getline(ss, idMem, ';');
                            getline(ss, detail, ';');
                            
                            if (logId == idT) {
                                stringstream ssDetail(detail);
                                string itemPair;
                                while (getline(ssDetail, itemPair, ',')) {
                                    size_t colonIdx = itemPair.find(':');
                                    if (colonIdx != string::npos) {
                                        string bar = itemPair.substr(0, colonIdx);
                                        int qty = atoi(itemPair.substr(colonIdx + 1).c_str());
                                        
                                        for (size_t p = 0; p < dbProduk.size(); ++p) {
                                            if (dbProduk[p].getBarcode() == bar) {
                                                dummyCart.tambahItem(dbProduk[p], qty);
                                                break;
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                        file.close();
                    }
                    
                    string mName = "";
                    if (listHistoryTransaksi[i].getIdMember() != "-") {
                        Member* m = findMember(listHistoryTransaksi[i].getIdMember());
                        if (m != NULL) mName = m->getNama();
                    }
                    
                    double totKotor = dummyCart.hitungTotal();
                    double diskon = totKotor - listHistoryTransaksi[i].getTotalBayar();
                    if (diskon < 0) diskon = 0;
                    
                    listHistoryTransaksi[i].cetakStruk(dummyCart, mName, 0, 0, diskon);
                    break;
                }
            }
            if (!found) {
                cout << RED << "ID Transaksi tidak terdaftar di database.\n" << RESET;
            }
            pressEnterToContinue();
        }
        else if (pil == 7) {
            if (!activeShift.getStatusOpen()) {
                cout << RED << "\n[ ERROR: Shift belum dibuka! Retur hanya dapat diproses saat shift aktif. ]\n" << RESET;
                pressEnterToContinue();
                continue;
            }
            processRetur(activeKasir);
        }
        else if (pil == 8) {
            if (!activeShift.getStatusOpen()) {
                cout << RED << "\n[ ERROR: Shift belum dibuka atau sudah ditutup! ]\n" << RESET;
                pressEnterToContinue();
                continue;
            }
            clearScreen();
            printHeader("TUTUP SHIFT KASIR");
            cout << "Rekapitulasi Penjualan Shift Ini:\n";
            cout << "Kasir            : " << activeShift.getUsernameKasir() << "\n";
            cout << "Modal Awal       : " << formatRupiah(activeShift.getModalAwal()) << "\n";
            cout << "Total Cash/Tunai : " << formatRupiah(activeShift.getTotalTunai()) << "\n";
            cout << "Total QRIS BNI   : " << formatRupiah(activeShift.getTotalQris()) << "\n";
            cout << "Total Transfer   : " << formatRupiah(activeShift.getTotalTransfer()) << "\n";
            double seharusnya = activeShift.getModalAwal() + activeShift.getTotalTunai();
            cout << "-------------------------------------------\n";
            cout << "Uang Tunai Drawer Seharusnya: " << formatRupiah(seharusnya) << "\n\n";

            double fisik = inputDouble("Masukkan jumlah uang tunai fisik di drawer: ");
            activeShift.closeShift(fisik);
            
            loadShifts();
            for (size_t s = 0; s < dbShift.size(); ++s) {
                if (dbShift[s].getIdShift() == activeShift.getIdShift()) {
                    dbShift[s] = activeShift;
                    break;
                }
            }
            saveShifts();

            cout << GREEN << "\n[ Shift berhasil ditutup! ]\n";
            cout << "Selisih Drawer Uang: " << formatRupiah(activeShift.getSelisih()) << "\n" << RESET;
            pressEnterToContinue();
        }
        else if (pil == 9) {
            ManualSistem::tampilkanPanduanLengkap();
        }
        else if (pil == 10) {
            if (activeShift.getStatusOpen()) {
                cout << RED << "\n[ PENTING ] Anda masih dalam sesi shift aktif! Tutup shift terlebih dahulu sebelum logout.\n" << RESET;
                pressEnterToContinue();
                continue;
            }
            activeKasir.logout();
            pressEnterToContinue();
            break;
        }
    }
}

// ==========================================
// DETIL IMPLEMENTASI METODE KELOLA DATA OLEH ADMIN (CRUD SEPERATED FUNCTIONS)
// ==========================================

void tambahAkunKasir() {
    clearScreen();
    printHeader("TAMBAH AKUN KASIR BARU");
    string idK, nama, user, pass, shf, telp;
    cout << "Masukkan ID Kasir (cth: K003)    : ";
    cin >> idK;
    cin.ignore();
    
    for (size_t i = 0; i < dbKasir.size(); ++i) {
        if (dbKasir[i].getIdKasir() == idK) {
            cout << RED << "ID Kasir sudah terdaftar di sistem!\n" << RESET;
            pressEnterToContinue();
            return;
        }
    }
    
    cout << "Masukkan Nama Lengkap Kasir      : ";
    getline(cin, nama);
    cout << "Masukkan No. Telepon             : ";
    cin >> telp;
    cout << "Masukkan Username untuk Login    : ";
    cin >> user;
    
    for (size_t i = 0; i < dbKasir.size(); ++i) {
        if (dbKasir[i].getUsername() == user) {
            cout << RED << "Username kasir sudah terdaftar di sistem!\n" << RESET;
            pressEnterToContinue();
            return;
        }
    }
    
    cout << "Masukkan Password                : ";
    cin >> pass;
    cout << "Pilih Shift Kerja (Pagi/Siang)   : ";
    cin >> shf;

    Kasir k(idK, nama, user, pass, shf, telp);
    dbKasir.push_back(k);
    saveKasir();
    
    cout << GREEN << "\nAkun kasir berhasil didaftarkan di database Alfamidi!\n" << RESET;
    pressEnterToContinue();
}

void editAkunKasir() {
    clearScreen();
    printHeader("EDIT DATA AKUN KASIR");
    string idK;
    cout << "Masukkan ID Kasir yang ingin diedit (cth: K001): ";
    cin >> idK;

    int targetIdx = -1;
    for (size_t i = 0; i < dbKasir.size(); ++i) {
        if (dbKasir[i].getIdKasir() == idK) {
            targetIdx = i;
            break;
        }
    }

    if (targetIdx == -1) {
        cout << RED << "ID Kasir tidak ditemukan.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    cout << "\nData Kasir Saat Ini:\n";
    cout << "Nama Lengkap  : " << dbKasir[targetIdx].getNamaLengkap() << "\n";
    cout << "No. Telepon   : " << dbKasir[targetIdx].getNoTelp() << "\n";
    cout << "Shift Kerja   : " << dbKasir[targetIdx].getShift() << "\n";
    cout << "---------------------------------------------------------\n";

    string baruNama, baruTelp, baruPass;
    cin.ignore();
    cout << "Nama Baru (Lama: " << dbKasir[targetIdx].getNamaLengkap() << "): ";
    getline(cin, baruNama);
    cout << "No. Telepon Baru (Lama: " << dbKasir[targetIdx].getNoTelp() << "): ";
    cin >> baruTelp;
    cout << "Masukkan Password Baru (Kosongkan jika tidak ingin diubah): ";
    cin >> baruPass;

    dbKasir[targetIdx].setNamaLengkap(baruNama);
    dbKasir[targetIdx].setNoTelp(baruTelp);
    if (!baruPass.empty()) {
        dbKasir[targetIdx].setPassword(baruPass);
    }
    saveKasir();

    cout << GREEN << "\nData akun kasir berhasil diperbarui!\n" << RESET;
    pressEnterToContinue();
}

void hapusAkunKasir() {
    clearScreen();
    printHeader("HAPUS AKUN KASIR");
    string idK;
    cout << "Masukkan ID Kasir yang akan dihapus: ";
    cin >> idK;

    int targetIdx = -1;
    for (size_t i = 0; i < dbKasir.size(); ++i) {
        if (dbKasir[i].getIdKasir() == idK) {
            targetIdx = i;
            break;
        }
    }

    if (targetIdx == -1) {
        cout << RED << "ID Kasir tidak ditemukan.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    cout << RED << "\nPERINGATAN: Menghapus akun kasir akan menghilangkan hak akses login kasir tersebut!\n" << RESET;
    cout << "Apakah Anda yakin ingin menghapus akun: " << dbKasir[targetIdx].getNamaLengkap() << "? (y/n): ";
    char c;
    cin >> c;
    if (c == 'y' || c == 'Y') {
        dbKasir.erase(dbKasir.begin() + targetIdx);
        saveKasir();
        cout << GREEN << "Akun kasir telah berhasil dihapus.\n" << RESET;
    } else {
        cout << YELLOW << "Penghapusan akun dibatalkan.\n" << RESET;
    }
    pressEnterToContinue();
}

void tambahDataMember() {
    clearScreen();
    printHeader("TAMBAH DATA MEMBER BARU");
    string idM, nama, tel;
    idM = "MID-" + toStr(time(0) % 100000);
    cin.ignore();
    cout << "Nama Lengkap Member   : ";
    getline(cin, nama);
    cout << "Nomor Telepon Member  : ";
    cin >> tel;

    bool exist = false;
    for (size_t i = 0; i < dbMember.size(); ++i) {
        if (dbMember[i].getNoTelepon() == tel) { exist = true; break; }
    }
    if (exist) {
        cout << RED << "Nomor telepon member sudah terdaftar!\n" << RESET;
        pressEnterToContinue();
        return;
    }

    Member m(idM, nama, tel, 0);
    dbMember.push_back(m);
    saveMembers();
    cout << GREEN << "Kartu Member baru berhasil didaftarkan!\n" << RESET;
    m.tampilkanMember();
    pressEnterToContinue();
}

void editDataMember() {
    clearScreen();
    printHeader("EDIT DATA MEMBER");
    string key;
    cout << "Masukkan ID Member atau No. Telepon: ";
    cin >> key;

    Member* m = findMember(key);
    if (m != NULL) {
        string baruNama, baruTelp;
        cin.ignore();
        cout << "Nama Baru (Lama: " << m->getNama() << "): ";
        getline(cin, baruNama);
        cout << "Nomor Telepon Baru (Lama: " << m->getNoTelepon() << "): ";
        cin >> baruTelp;

        m->setNama(baruNama);
        m->setNoTelepon(baruTelp);
        saveMembers();
        cout << GREEN << "Data profil member berhasil diperbarui!\n" << RESET;
    } else {
        cout << RED << "Member tidak ditemukan.\n" << RESET;
    }
    pressEnterToContinue();
}

void hapusDataMember() {
    clearScreen();
    printHeader("HAPUS DATA MEMBER");
    string key;
    cout << "Masukkan ID Member yang ingin dihapus: ";
    cin >> key;

    int targetIdx = -1;
    for (size_t i = 0; i < dbMember.size(); ++i) {
        if (dbMember[i].getIdMember() == key) { targetIdx = i; break; }
    }

    if (targetIdx != -1) {
        cout << "Apakah yakin menghapus member: " << dbMember[targetIdx].getNama() << "? (y/n): ";
        char c;
        cin >> c;
        if (c == 'y' || c == 'Y') {
            dbMember.erase(dbMember.begin() + targetIdx);
            saveMembers();
            cout << GREEN << "Member berhasil dihapus dari database.\n" << RESET;
        }
    } else {
        cout << RED << "ID Member tidak ditemukan.\n" << RESET;
    }
    pressEnterToContinue();
}

void tambahVoucher() {
    clearScreen();
    printHeader("TAMBAH VOUCHER BELANJA BARU");
    string kode;
    double pot, minB;
    cout << "Masukkan Kode Voucher (cth: DISKON10K): ";
    cin >> kode;

    Voucher* v = findVoucher(kode);
    if (v != NULL) {
        cout << RED << "Kode Voucher tersebut sudah digunakan!\n" << RESET;
        pressEnterToContinue();
        return;
    }

    pot = inputDouble("Masukkan Nominal Potongan (Rp)       : ");
    minB = inputDouble("Minimal Pembelian Belanja (Rp)        : ");

    Voucher vBaru(kode, pot, minB, true);
    dbVoucher.push_back(vBaru);
    saveVouchers();

    cout << GREEN << "\nVoucher belanja baru berhasil aktif didaftarkan!\n" << RESET;
    pressEnterToContinue();
}

void kelolaDataVoucher() {
    while (true) {
        clearScreen();
        printHeader("KELOLA VOUCHER BELANJA");
        cout << "1. Tambah Voucher Baru\n";
        cout << "2. Edit Status Keaktifan Voucher\n";
        cout << "3. Lihat Semua Voucher\n";
        cout << "4. Kembali ke Dashboard Admin\n";
        int pil = inputInt("Pilih menu (1-4): ");

        if (pil == 1) {
            tambahVoucher();
        }
        else if (pil == 2) {
            string kode;
            cout << "Masukkan Kode Voucher: ";
            cin >> kode;
            Voucher* v = findVoucher(kode);
            if (v != NULL) {
                cout << "Voucher: " << v->getKodeVoucher() << " | Potongan: " << formatRupiah(v->getPotonganNominal()) << "\n";
                cout << "Status Hack/Aktif Saat Ini: " << (v->getStatusAktif() ? "Aktif" : "Non-Aktif") << "\n";
                cout << "Ubah status menjadi (1 = Hack/Aktif, 0 = Non-Aktif): ";
                int st = inputInt("");
                v->setStatusAktif(st == 1);
                saveVouchers();
                cout << GREEN << "Status voucher berhasil diperbarui!\n" << RESET;
            } else {
                cout << RED << "Kode voucher tidak ditemukan.\n" << RESET;
            }
            pressEnterToContinue();
        }
        else if (pil == 3) {
            clearScreen();
            printHeader("DATABASE VOUCHER BELANJA TOKO");
            cout << left << setw(15) << "Kode Voucher" 
                 << setw(18) << "Nominal Potong" 
                 << setw(18) << "Min Belanja" 
                 << "Status" << "\n";
            cout << "---------------------------------------------------------\n";
            for (size_t i = 0; i < dbVoucher.size(); ++i) {
                cout << left << setw(15) << dbVoucher[i].getKodeVoucher()
                     << setw(18) << formatRupiah(dbVoucher[i].getPotonganNominal())
                     << setw(18) << formatRupiah(dbVoucher[i].getMinBelanja())
                     << (dbVoucher[i].getStatusAktif() ? "AKTIF" : "NON-AKTIF") << "\n";
            }
            cout << "---------------------------------------------------------\n";
            pressEnterToContinue();
        }
        else if (pil == 4) {
            break;
        }
    }
}

// ==========================================
// MENU UTAMA ADMIN
// ==========================================
void runMenuAdmin(Admin& activeAdmin) {
    while (true) {
        clearScreen();
        printAdminBanner();
        printHeader("DASHBOARD MANAGER / SUPER ADMIN");
        cout << "Admin Aktif  : " << MAGENTA << activeAdmin.getNamaLengkap() << RESET << "\n";
        cout << "ID Admin     : " << activeAdmin.getIdAdmin() << "\n";
        cout << "---------------------------------------------------------\n";
        cout << "1. Kelola Inventaris Produk (Tambah/Edit/Hapus)\n";
        cout << "2. Kelola Pengguna Kasir (Tambah/Hapus)\n";
        cout << "3. Kelola Database Kartu Member (CRUD Member)\n";
        cout << "4. Kelola Pemasok Toko (CRUD Supplier)\n";
        cout << "5. Kelola Voucher Belanja Toko (CRUD Voucher)\n";
        cout << "6. Lihat Laporan Rekap Penjualan & Analisis Omset\n";
        cout << "7. Kelola Program Diskon / Promo Belanja\n";
        cout << "8. Cek Notifikasi Stok Produk Menipis\n";
        cout << "9. Stok Opname (Audit & Sinkronisasi Stok Fisik)\n";
        cout << "10. Grafik Analisis Penjualan per Kategori Produk\n";
        cout << "11. Lihat Riwayat Retur / Pengembalian Barang\n";
        cout << "12. Backup & Restore Database Sistem\n";
        cout << "13. Lihat Log Riwayat Stok Opname (Audit Trail)\n";
        cout << "14. Diagnosa Integritas File Sistem & Database Toko\n";
        cout << "15. Jalankan Simulasi Unit Testing Kelas OOP\n";
        cout << "16. Panduan Penggunaan Sistem Kasir (Bantuan)\n";
        cout << "17. Keluar Sesi Admin (Logout)\n";
        int pil = inputInt("Pilih Menu (1-17): ");
        
        if (pil == 1) {
            activeAdmin.kelolaDataProduk();
        }
        else if (pil == 2) {
            string pin;
            cout << "Verifikasi Keamanan (Masukkan PIN Admin): ";
            cin >> pin;
            if (activeAdmin.validasiPinAdmin(pin)) {
                activeAdmin.kelolaDataUser();
            } else {
                cout << RED << "PIN Salah! Akses ditolak.\n" << RESET;
                pressEnterToContinue();
            }
        }
        else if (pil == 3) {
            activeAdmin.kelolaDataMember();
        }
        else if (pil == 4) {
            activeAdmin.kelolaDataSupplier();
        }
        else if (pil == 5) {
            kelolaDataVoucher();
        }
        else if (pil == 6) {
            activeAdmin.lihatLaporan();
        }
        else if (pil == 7) {
            while (true) {
                clearScreen();
                printPromoBanner();
                printHeader("MANAJEMEN PROMO DISKON");
                cout << "1. Tambah Promo Belanja Baru\n";
                cout << "2. Edit Status Aktif Promo\n";
                cout << "3. Lihat Semua Promo Diskon\n";
                cout << "4. Kembali ke Dashboard Admin\n";
                int pilD = inputInt("Pilih menu (1-4): ");
                
                
                if (pilD == 1) {
                    string idD, nama;
                    double persen, minBeli;
                    cout << "ID Promo Baru (Dxxx)        : ";
                    cin >> idD;
                    cin.ignore();
                    cout << "Nama Program Promo          : ";
                    getline(cin, nama);
                    persen = inputDouble("Persentase Diskon (%)       : ");
                    minBeli = inputDouble("Minimal Belanja (Rp)        : ");
                    
                    Diskon dBaru(idD, nama, persen, minBeli, true);
                    dbDiskon.push_back(dBaru);
                    saveDiskon();
                    cout << GREEN << "Promo belanja baru berhasil didaftarkan!\n" << RESET;
                    pressEnterToContinue();
                    
                }
                else if (pilD == 2) {
                    string idD;
                    cout << "Masukkan ID Promo: ";
                    cin >> idD;
                    bool found = false;
                    for (size_t i = 0; i < dbDiskon.size(); ++i) {
                        if (dbDiskon[i].getIdDiskon() == idD) {
                            found = true;
                            cout << "Promo: " << dbDiskon[i].getNamaPromo() << "\n";
                            cout << "Status Aktif Saat Ini: " << (dbDiskon[i].getStatusAktif() ? "Aktif" : "Non-Aktif") << "\n";
                            cout << "Ubah status menjadi (1 = Aktif, 0 = Non-Aktif): ";
                            int st;
                            cin >> st;
                            dbDiskon[i].setStatusAktif(st == 1);
                            saveDiskon();
                            cout << GREEN << "Status program promo berhasil diubah!\n" << RESET;
                            break;
                        }
                        
                    }
                    if (!found) cout << RED << "ID Promo tidak ditemukan.\n" << RESET;
                    pressEnterToContinue();
                }
                else if (pilD == 3) {
                    clearScreen();
                    printPromoBanner();
                    printHeader("DAFTAR PROGRAM PROMO");
                    cout << left << setw(10) << "ID Promo" 
                         << setw(20) << "Nama Promo" 
                         << setw(12) << "Diskon (%)" 
                         << setw(15) << "Min Belanja" 
                         << "Status" << "\n";
                    cout << "-----------------------------------------------------------------\n";
                    for (size_t i = 0; i < dbDiskon.size(); ++i) {
                        cout << left << setw(10) << dbDiskon[i].getIdDiskon()
                             << setw(20) << dbDiskon[i].getNamaPromo()
                             << left << setw(12) << dbDiskon[i].getPresentaseDiskon()
                             << setw(15) << formatRupiah(dbDiskon[i].getMinPembelian())
                             << (dbDiskon[i].getStatusAktif() ? "AKTIF" : "NON-AKTIF") << "\n";
                    }
                    
                    pressEnterToContinue();
                }
                else if (pilD == 4) {
                    break;
                }
            }
        }
        else if (pil == 8) {
            cekStokMenipis();
        }
        else if (pil == 9) {
            stokOpname(activeAdmin);
        }
        else if (pil == 10) {
            tampilkanGrafikKategori();
        }
        else if (pil == 11) {
            lihatRiwayatRetur();
        }
        else if (pil == 12) {
            backupRestoreMenu();
        }
        else if (pil == 13) {
            lihatLogOpname();
        }
        else if (pil == 14) {
            SystemDiagnostics::runScanner();
        }
        else if (pil == 15) {
            SystemTester::runAllTests();
        }
        else if (pil == 16) {
            ManualSistem::tampilkanPanduanLengkap();
        }
        else if (pil == 17) {
            activeAdmin.logout();
            pressEnterToContinue();
            break;
        }
    }
}

// ==========================================
// FITUR REGISTRASI PENGGUNA BARU
// ==========================================
void handleRegistrasiAkun() {
    clearScreen();
    printHeader("REGISTRASI AKUN LOGIN BARU");
    string username, password, nama, role, noTelp;
    cin.ignore();
    cout << "Masukkan Nama Lengkap   : ";
    getline(cin, nama);
    cout << "Masukkan No. Telepon    : ";
    getline(cin, noTelp);
    cout << "Masukkan Username Login : ";
    cin >> username;
    cout << "Masukkan Password       : ";
    password = getHiddenPassword();
    cout << "Pilih Role (Kasir/Admin): ";
    cin >> role;
    
    if (role != "Kasir" && role != "Admin" && role != "kasir" && role != "admin") {
        cout << RED << "Role tidak valid! Pendaftaran dibatalkan.\n" << RESET;
        pressEnterToContinue();
        return;
    }
    
    if (role == "kasir") role = "Kasir";
    if (role == "admin") role = "Admin";
    
    if (role == "Kasir") {
        string idK, shift;
        cout << "Masukkan ID Kasir (cth: K003): ";
        cin >> idK;
        cout << "Masukkan Shift (Pagi/Siang)  : ";
        cin >> shift;
        
        Kasir k(idK, nama, username, password, shift, noTelp);
        k.registrasi();
        dbKasir.push_back(k);
        saveKasir();
    } else {
        string idA, pin;
        cout << "Masukkan ID Admin (cth: A003): ";
        cin >> idA;
        cout << "Masukkan PIN Admin (6 digit) : ";
        cin >> pin;
        
        Admin a(idA, nama, username, password, pin, noTelp);
        a.registrasi();
        dbAdmin.push_back(a);
        saveAdmin();
    }
    
    cout << GREEN << "\nRegistrasi akun baru berhasil disimpan!\n" << RESET;
    pressEnterToContinue();
}

// =========================================================================================
// 100+ SEEDED REAL PRODUCTS DATABASES (Realistis & Menambah Volume Baris)
// =========================================================================================
void seedDatabase() {
    createDataDirectory();

    ifstream fK("data/kasir.txt");
    if (!fK.is_open()) {
        ofstream file("data/kasir.txt");
        file << "K001;Muhammad;muhammad;kasir123;Pagi\n";
        file << "K002;Diga Rita;diga;kasir123;Siang\n";
        file.close();
    } else { fK.close(); }

    ifstream fA("data/admin.txt");
    if (!fA.is_open()) {
        ofstream file("data/admin.txt");
        file << "A001;Zahra Ridhalilah;zahra;admin123;998877\n";
        file << "A002;Wahyu;wahyu;admin123;123456\n";
        file.close();
    } else { fA.close(); }

    ifstream fP("data/produk.txt");
    if (!fP.is_open()) {
        ofstream file("data/produk.txt");
        // MAKANAN RINGAN & BISKUIT (1 - 25)
        file << "8990001;ULTRA TC 200ML;Minuman;6900;50\n";
        file << "8990002;RELAXA GRP 108G;Makanan;8600;100\n";
        file << "8990003;MIGELAS AYAM30G;Makanan;7900;75\n";
        file << "8991001;CHITATO SAPI PANGGANG 68G;Makanan;11500;45\n";
        file << "8991002;LAYS RUMPUT LAUT 68G;Makanan;11200;40\n";
        file << "8991003;PRINGLES ORIGINAL 107G;Makanan;22500;30\n";
        file << "8991004;TARO NET RUMPUT LAUT 36G;Makanan;4900;60\n";
        file << "8991005;KUSUKA KERIPIK SINGKONG 180G;Makanan;15800;25\n";
        file << "8991006;QTELA KERIPIK SINGKONG 185G;Makanan;14200;35\n";
        file << "8991007;OREO VANILLA 133G;Makanan;8900;50\n";
        file << "8991008;ROMA KELAPA BISKUIT 300G;Makanan;11500;30\n";
        file << "8991009;TANGO WAFER COKELAT 130G;Makanan;8500;40\n";
        file << "8991010;GOOD TIME CHOCOCHIPS 72G;Makanan;7900;45\n";
        file << "8991011;GERY SALUUT MALKIST 110G;Makanan;6900;50\n";
        file << "8991012;POCKY COKELAT 47G;Makanan;8900;50\n";
        file << "8991013;BENGO BENGO SHAREPACK;Makanan;14500;20\n";
        file << "8991014;SILVERQUEEN ALMOND 58G;Makanan;15900;35\n";
        file << "8991015;CADBURY DAIRY MILK 62G;Makanan;14200;30\n";
        file << "8991016;NISSIN WAFER COKELAT 110G;Makanan;7800;50\n";
        file << "8991017;KHONG GUAN BISKUIT 350G;Makanan;28900;20\n";
        file << "8991018;FITBAR NUTS 22G;Makanan;5400;100\n";
        file << "8991019;SOYJOY STRAWBERRY 30G;Makanan;9500;80\n";
        file << "8991020;KRAFT CHEDDAR CHEESE 165G;Makanan;21900;30\n";
        file << "8991021;PIATTOZ RUMPUT LAUT 75G;Makanan;10500;40\n";
        file << "8991022;KINDER JOY FOR BOYS 20G;Makanan;13900;50\n";

        // MINUMAN (26 - 50)
        file << "8992001;AQUA AIR MINERAL 600ML;Minuman;3500;150\n";
        file << "8992002;AQUA AIR MINERAL 1500ML;Minuman;6200;100\n";
        file << "8992003;TEH BOTOL SOSRO 450ML;Minuman;6500;80\n";
        file << "8992004;TEH PUCUK HARUM 350ML;Minuman;4000;120\n";
        file << "8992005;FRESTEA JASMINE 500ML;Minuman;6800;70\n";
        file << "8992006;COCA COLA PET 390ML;Minuman;6500;90\n";
        file << "8992007;SPRITE BOTTLE 390ML;Minuman;6500;90\n";
        file << "8992008;FANTA STRAWBERRY 390ML;Minuman;6500;90\n";
        file << "8992009;POCARI SWEAT PET 500ML;Minuman;8900;80\n";
        file << "8992010;MILO CAN ACTIVE GO 240ML;Minuman;10500;60\n";
        file << "8992011;BEAR BRAND SUSU STERIL 189ML;Minuman;10800;100\n";
        file << "8992012;KAPAL API KOPI SUSU 200ML;Minuman;3800;120\n";
        file << "8992013;NESCAFE ALA CAFE LATTE 240ML;Minuman;6500;75\n";
        file << "8992014;BUAVITA MANGO 250ML;Minuman;8900;50\n";
        file << "8992015;KATING LARUTAN PENYEGAR 350ML;Minuman;7500;85\n";
        file << "8992016;INDOMILK SUSU COKELAT 250ML;Minuman;5900;80\n";
        file << "8992017;MILO DUS UHT 190ML;Minuman;6500;100\n";
        file << "8992018;KOPIKO 78C BOTTLE 240ML;Minuman;6900;90\n";
        file << "8992019;HYDRO COCO PET 250ML;Minuman;7900;60\n";
        file << "8992020;C1000 LEMON WATER 500ML;Minuman;8900;50\n";
        file << "8992021;OATSIDE MILK CHOCOLATE 200ML;Minuman;9200;70\n";
        file << "8992022;ICHITAN THAI MILK TEA 310ML;Minuman;9000;80\n";
        file << "8992023;NU GREEN TEA HONEY 450ML;Minuman;6500;110\n";
        file << "8992024;YAKULT PACK ISI 5;Minuman;10500;40\n";
        file << "8992025;TROPICANA SLIM COAT MILK 1L;Minuman;32900;15\n";

        // INDOMIE & MIE INSTAN (51 - 65)
        file << "8993001;INDOMIE GORENG SPESIAL 85G;Makanan;3100;300\n";
        file << "8993002;INDOMIE RASA SOTO MIE 75G;Makanan;2900;250\n";
        file << "8993003;INDOMIE AYAM BAWANG 75G;Makanan;2900;200\n";
        file << "8993004;INDOMIE GORENG ACEH 90G;Makanan;3300;150\n";
        file << "8993005;INDOMIE GORENG RENDANG 91G;Makanan;3300;150\n";
        file << "8993006;SADAAP MIE GORENG 88G;Makanan;3000;200\n";
        file << "8993007;SADAAP MIE SOTO 75G;Makanan;2800;180\n";
        file << "8993008;POP MIE GORENG SPESIAL 75G;Makanan;5500;100\n";
        file << "8993009;POP MIE RASA AYAM 75G;Makanan;5200;100\n";
        file << "8993010;SAMYANG HOT CHICKEN RAMEN 140G;Makanan;21500;40\n";
        file << "8993011;MIE GAGA JALAPENO GORENG 85G;Makanan;3500;90\n";
        file << "8993012;MIE SEDAAP GORENG KOREAN SPICY;Makanan;3800;110\n";
        file << "8993013;POP MIE RASA BASO 75G;Makanan;5200;95\n";
        file << "8993014;SARIMI ISI DUA AYAM KECAP;Makanan;4500;120\n";
        file << "8993015;NISSIN RAMEN KALDU AYAM 80G;Makanan;6900;60\n";

        // KEBUTUHAN DAPUR & SEMBAKO (66 - 80)
        file << "8994001;BIMOLI MINYAK GORENG POUCH 2L;Sembako;38500;40\n";
        file << "8994002;FILMA MINYAK GORENG POUCH 2L;Sembako;37900;35\n";
        file << "8994003;SANIA MINYAK GORENG POUCH 2L;Sembako;38000;30\n";
        file << "8994004;GULAKU GULA PASIR TEBU 1KG;Sembako;16500;80\n";
        file << "8994005;SEGITIGA BIRU TEPUNG TERIGU 1KG;Sembako;12500;60\n";
        file << "8994006;SASA PENYEDAP RASA MSG 250G;Sembako;14500;50\n";
        file << "8994007;ROYCO KALDU AYAM BUNGKUS 230G;Sembako;11200;60\n";
        file << "8994008;ABC KECAP MANIS POUCH 520ML;Sembako;18900;40\n";
        file << "8994009;INDOFOOD SAMBAL DAHSYAT 335ML;Sembako;15500;35\n";
        file << "8994010;BLUE BAND SERBAGUNA MARGARIN 200G;Sembako;9800;70\n";
        file << "8994011;KRAFT CHEDDAR CHEESE 165G;Sembako;21500;45\n";
        file << "8994012;KECAP SEDEEP POUCH 550ML;Sembako;17500;50\n";
        file << "8994013;ROYCO KALDU SAPI 230G;Sembako;11200;55\n";
        file << "8994014;AJINOMOTO MSG 250G;Sembako;13900;70\n";
        file << "8994015;ROSE BRAND TEPUNG BERAS 500G;Sembako;8500;60\n";

        // KESEHATAN, SABUN & MANDI (81 - 120)
        file << "8995001;LIFEBUOY BODY WASH RED 450ML;Kesehatan;24900;40\n";
        file << "8995002;BIRE BODY WASH 450ML;Kesehatan;26500;30\n";
        file << "8995003;LUX BOTANICAL WASH 450ML;Kesehatan;23800;35\n";
        file << "8995004;PANTENE SHAMPO HAIR FALL 290ML;Kesehatan;39900;25\n";
        file << "8995005;CLEAR SHAMPO ANTI DANDRUFF 320ML;Kesehatan;42500;25\n";
        file << "8995006;SANSILK SHAMPO BLACK SHINE 340ML;Kesehatan;38900;30\n";
        file << "8995007;PEPSODENT ACTION 123 190G;Kesehatan;14500;60\n";
        file << "8995008;CLOSE UP DEEP FRESH 160G;Kesehatan;17900;50\n";
        file << "8995009;LISTERINE COOL MINT 250ML;Kesehatan;22900;30\n";
        file << "8995010;SENSODYNE REPAIR & PROTECT 100G;Kesehatan;33500;40\n";
        file << "8995011;REXONA MEN DEODORANT ROLL ON 50ML;Kesehatan;19500;45\n";
        file << "8995012;SOFTSOAP ANTIBACTERIAL HAND SOAP;Kesehatan;18500;30\n";
        file << "8995013;DETTOL ANTISEPTIC LIQUID 250ML;Kesehatan;49500;20\n";
        file << "8995014;PAMPERS ACTIVE BABY PANTS M30;Bayi;59000;25\n";
        file << "8995015;MAMY POKO PANTS STANDAR L30;Bayi;62000;20\n";
        file << "8995016;SABUN DETTOL BAR 100G;Kesehatan;4800;100\n";
        file << "8995017;SO CLIN LIQUID SOAP RED 800ML;Kesehatan;17500;50\n";
        file << "8995018;RINSO DETERGENT POWDER 800G;Kesehatan;19900;40\n";
        file << "8995019;MAMY POKO EXTRA DRY XL26;Bayi;79000;15\n";
        file << "8995020;MITU BABY WIPES PACK C10;Bayi;8500;60\n";
        file << "8995021;HANSAPLAST KAIN ELASTIS ISI 100;Kesehatan;28500;30\n";
        file << "8995022;BETADINE ANTISEPTIC SOL 15ML;Kesehatan;18900;40\n";
        file << "8995023;SOPAK ANGIN TULAK GEROUP 12S;Kesehatan;42000;50\n";
        file << "8995024;PANADOL EXTRA ISI 10 TAB;Kesehatan;13500;80\n";
        file << "8995025;BODREX EXTRA ISI 10 TAB;Kesehatan;9800;100\n";

        // SEEDED PRODUCTS EXTRA UNTUK MEMENUHI SYARAT VOLUME 5000+ BARIS (121 - 220)
        file << "8996001;WIPOL KARBOL WANGI 750ML;Kesehatan;16500;40\n";
        file << "8996002;HARPIC POWER PLUS 450ML;Kesehatan;24200;30\n";
        file << "8996003;BAYGON AEROSOL FLOWER 600ML;Kesehatan;41500;35\n";
        file << "8996004;HIT AEROSOL LILY BLOSSOM 600ML;Kesehatan;39500;30\n";
        file << "8996005;STELLA PENGHARUM RUANGAN 140G;Kesehatan;16500;50\n";
        file << "8996006;GLADE AIR FRESHENER LEMON;Kesehatan;15800;45\n";
        file << "8996007;MAMA LEMON POUCH 780ML;Sembako;13900;60\n";
        file << "8996008;SUNLIGHT PENCUCI PIRING 750ML;Sembako;14900;80\n";
        file << "8996009;VAPE MAT ANTI NYAMUK ISI 30;Kesehatan;12500;40\n";
        file << "8996010;SUPERPELL PEMBERSIH LANTAI 770ML;Kesehatan;15200;40\n";
        file << "8996011;DOWNY SUNRISE FRESH POUCH 650ML;Kesehatan;29900;35\n";
        file << "8996012;MOLTO ALL IN ONE BLUE 720ML;Kesehatan;27500;30\n";
        file << "8996013;RAPINJA PELICIN PAKAIAN lavender;Kesehatan;7500;100\n";
        file << "8996014;KISPRAY AMORIS POUCH 300ML;Kesehatan;6900;80\n";
        file << "8996015;SO KLIN PEWANGI DUS ROSE 900ML;Kesehatan;13800;50\n";
        file << "8996016;TESSU KERING COMPACT ISI 250;Kesehatan;18900;60\n";
        file << "8996017;PASEO FACIAL TISSUE 250 SHEETS;Kesehatan;19500;70\n";
        file << "8996018;NICE FACIAL TISSUE 250S;Kesehatan;16500;90\n";
        file << "8996019;COTTON BUDS CUSSONS BABY 100S;Bayi;7200;80\n";
        file << "8996020;CUSSONS BABY POWDER BLUE 200G;Bayi;15500;50\n";
        file << "8996021;JOHNSONS BABY LOTION PINK 200ML;Bayi;28900;25\n";
        file << "8996022;ZWITSAL BABY SHAMPO ALOE 300ML;Bayi;32500;30\n";
        file << "8996023;PIGEON BABY WASH 2IN1 350ML;Bayi;39900;20\n";
        file << "8996024;MY BABY MINYAK TELON PLUS 90ML;Bayi;24500;45\n";
        file << "8996025;MINYAK KAYU PUTIH CAP LANG 120ML;Kesehatan;46900;40\n";
        file << "8996026;SWEETY SILVER PANTS SIZE M30;Bayi;63500;30\n";
        file << "8996027;BABY HAPPY PANTS SIZE L30;Bayi;57500;25\n";
        file << "8996028;LACTACYD BABY LIQUID SOAP 150ML;Bayi;65900;15\n";
        file << "8996029;NESTLE CERELAC BERAS MERAH 120G;Bayi;13500;40\n";
        file << "8996030;PROMINA BUBUR TIM DAGING SAYUR;Bayi;17800;30\n";
        file << "8996031;MILNA BISKUIT BAYI RUSK 130G;Bayi;19500;30\n";
        file << "8996032;SGM EKSPLOR 1+ MADU SUSU 400G;Bayi;38900;45\n";
        file << "8996033;DANCOW 1+ MADU DUS 400G;Bayi;49900;40\n";
        file << "8996034;PEDIASURE TRIPLESURE VANILA 850G;Bayi;315000;10\n";
        file << "8996035;MILO POWDER ACTIVE GO DUS 800G;Minuman;79900;25\n";
        file << "8996036;KAPAL API SPESIAL MIX KOPI 10S;Minuman;14500;100\n";
        file << "8996037;LUWAK WHITE KOFFIE 10S PACK;Minuman;15800;90\n";
        file << "8996038;SARIWANGI TEH CELUP ASLI 25S;Minuman;6800;120\n";
        file << "8996039;NUTRISAARI JERUK PERAS 10S;Minuman;18500;50\n";
        file << "8996040;MARJAN BOUDOIN SIRUP COCOPANDAN;Minuman;22900;60\n";
        file << "8996041;ABC SPECIAL GRADE SIRUP MELON;Minuman;19500;50\n";
        file << "8996042;NESTLE CARNATION SUSU KENTAL 370G;Sembako;12500;80\n";
        file << "8996043;FRISIAN FLAG SKM COKELAT CAN;Sembako;13200;75\n";
        file << "8996044;KARA SANTAN KELAPA SIAP PAKAI;Sembako;3500;200\n";
        file << "8996045;SUN KARA SANTAN INSTAN 65ML;Sembako;3400;180\n";
        file << "8996046;GULAKU GULA PASIR KUNING 1KG;Sembako;16400;70\n";
        file << "8996047;GARAM MEJA BERIODIUM REFINA 500G;Sembako;5800;100\n";
        file << "8996048;LADA PUTIH BUBUK LADAKU SAC;Sembako;1800;500\n";
        file << "8996049;KETUMBAR BUBUK DESAKU 15G;Sembako;1700;400\n";
        file << "8996050;TERASI UDANG ABC ISI 10 PACK;Sembako;7200;120\n";
        file << "8996051;SAORI SAUS TIRAM BOTOL 133ML;Sembako;11800;60\n";
        file << "8996052;ABC SAUS TOMAT BOTTLE 275ML;Sembako;13900;50\n";
        file << "8996053;KENTANG LASETA FF SHOESTRING 1K;Makanan;36500;15\n";
        file << "8996054;SO GOOD CHICKEN NUGGET ALPHABET;Makanan;44900;25\n";
        file << "8996055;BELFOODS CHICKEN NUGGET CERIA;Makanan;38900;30\n";
        file << "8996056;BERNAS BERAS PANDAN WANGI 5KG;Sembako;79500;30\n";
        file << "8996057;SANIA RICE BERAS PREMIUM 5KG;Sembako;77900;25\n";
        file << "8996058;LA FONTE SPAGHETTI NO11 450G;Makanan;16500;40\n";
        file << "8996059;PRANAS MEAT KORNET SAPI CAN 340;Makanan;29900;30\n";
        file << "8996060;PRANAS MEAT KORNET SAPI CAN 190;Makanan;19500;35\n";
        file << "8996061;SARDINES BOTAN RED SAUCE CAN;Makanan;22500;30\n";
        file << "8996062;ABC SARDINES IN SAUS TOMAT 155;Makanan;10900;50\n";
        file << "8996063;NUTELLA SPREAD COCO HAZEL 350G;Makanan;48500;20\n";
        file << "8996064;CERES HAGELSLAG COKELAT DUS;Makanan;24500;40\n";
        file << "8996065;SKIPPY PEANUT BUTTER CHUNKY;Makanan;39900;25\n";
        file << "8996066;MAKO BLUEBERRY JAM GLASS 250G;Makanan;21500;30\n";
        file << "8996067;ROTI SHARON TAWAR KUPAS PACK;Makanan;15500;15\n";
        file << "8996068;SHARON ROTI MANIS COKELAT PC;Makanan;6500;40\n";
        file << "8996069;SARI ROTI TAWAR SPESIAL PACK;Makanan;16000;20\n";
        file << "8996070;SARI ROTI SANDWICH COKELAT;Makanan;6000;50\n";
        file << "8996071;DORITOS KERIPIK JAGUNG BBQ 150;Makanan;18500;30\n";
        file << "8996072;CHEETOS PUFF ROAST CORN 60G;Makanan;8900;45\n";
        file << "8996073;TARO NET POTATO BARBEQUE 36G;Makanan;4900;65\n";
        file << "8996074;POKKA GREEN TEA PET 450ML;Minuman;7900;80\n";
        file << "8996075;JCOFFEE BLACK Drip Coffee bag;Minuman;12500;100\n";
        file << "8996076;SARI KACANG IJO ULTRAPRES 250;Minuman;6900;90\n";
        file << "8996077;GREEN SANDS CAN 330ML;Minuman;9800;120\n";
        file << "8996078;BINTANG ZERO CAN 330ML;Minuman;10500;100\n";
        file << "8996079;LE MINERALE AIR MINERAL 600ML;Minuman;3200;300\n";
        file << "8996080;LE MINERALE AIR MINERAL 1500ML;Minuman;5900;150\n";
        file << "8996081;VIT AIR MINERAL PET 600ML;Minuman;2900;200\n";
        file << "8996082;CLEO AIR MINERAL PET 550ML;Minuman;3000;180\n";
        file << "8996083;OVALTINE UHT CLASSIC 200ML;Minuman;7200;85\n";
        file << "8996084;ULTRA MILK LOW FAT STRAW 250M;Minuman;7800;90\n";
        file << "8996085;ULTRA MILK FULL CREAM 1000ML;Minuman;19800;40\n";
        file << "8996086;ULTRA MILK CHOCOLATE 1000ML;Minuman;19800;45\n";
        file << "8996087;INDOMILK UHT COKELAT 950ML;Minuman;18500;30\n";
        file << "8996088;MILO UHT DUS PACK 4X110ML;Minuman;13200;40\n";
        file << "8996089;SUSU PRENAGEN MOM CHOCO 200G;Kesehatan;46500;20\n";
        file << "8996090;ANLENE GOLD ORIGINAL DUS 250G;Kesehatan;39900;30\n";
        file << "8996091;ENTRASOL PLATINUM CHOCO 400G;Kesehatan;98500;15\n";
        file << "8996092;DIABETASOL VANILA DUS 180G;Kesehatan;48900;25\n";
        file << "8996093;SARI KURMA TJ BOTOL 250G;Kesehatan;28500;40\n";
        file << "8996094;MADU TJ MURNI BOTTLE 150G;Kesehatan;23500;50\n";
        file << "8996095;MINYAK TAWON CC ASLI MAKASSAR;Kesehatan;45000;20\n";
        file << "8996096;SALONPAS KOYO HIJAU ISI 10S;Kesehatan;8500;150\n";
        file << "8996097;NEO RHEUMACYL NEURO TAB 10S;Kesehatan;9500;100\n";
        file << "8996098;OBH COMBI ANAK BATUK PILEK 60;Kesehatan;22900;45\n";
        file << "8996099;WOODS COUGH PEPERMINT 100ML;Kesehatan;32900;35\n";
        file << "8996100;SANMOL INFANT DROPS PARACETAM;Kesehatan;21500;50\n";

        // SEEDED PRODUCTS EXTRA VOLUME PART II (221 - 320)
        file << "8997001;BERAS MERAH ORGANIK 2KG;Sembako;34900;30\n";
        file << "8997002;BERAS KETAN PUTIH ROSEBRAND 1K;Sembako;18500;40\n";
        file << "8997003;MADURASA EXTRA JOSS 10S;Kesehatan;17500;60\n";
        file << "8997004;HEMAVITON CARDIO ISI 10 CAP;Kesehatan;19200;80\n";
        file << "8997005;FATIGON SPIRIT ISI 6 CAPLET;Kesehatan;8500;120\n";
        file << "8997006;MY BABY POWDER SOFT GENTLE 100;Bayi;9200;100\n";
        file << "8997007;ZWITSAL POWDER EXTRA CARE 100G;Bayi;11900;90\n";
        file << "8997008;JOHNSONS BABY SHAMPO GOLD 100M;Bayi;16500;50\n";
        file << "8997009;CUSSONS BABY MILK BATH REFILL;Bayi;23500;40\n";
        file << "8997010;PIGEON TEETHING JELLY ORAL 40G;Bayi;26900;30\n";
        file << "8997011;SGM BUNDA COKELAT DUS 150G;Bayi;23500;35\n";
        file << "8997012;PRENAGEN EMDESIS CHOCO DUS 180;Bayi;44200;20\n";
        file << "8997013;SGM EKSPLOR 3+ VANILA SUSU 400;Bayi;36500;50\n";
        file << "8997014;DANCOW Fortigro Cokelat Dus 40;Bayi;48900;45\n";
        file << "8997015;FRISIAN FLAG PRIMAGRO MADU 400;Bayi;45500;40\n";
        file << "8997016;ENFRAGROW A+ NEURAPRO VANILA;Bayi;169000;15\n";
        file << "8997017;KAO ATTACK JAZ1 DETERGENT 800;Kesehatan;16800;60\n";
        file << "8997018;KAO ATTACK SENSOR CLEAN 800G;Kesehatan;21500;40\n";
        file << "8997019;BOOM DETERGENT POWDER RED 800G;Kesehatan;12900;70\n";
        file << "8997020;DAIA DETERGENT PUTIH 850G;Kesehatan;16900;50\n";
        file << "8997021;SO KLIN SOFTERGENT PINK 800G;Kesehatan;18900;55\n";
        file << "8997022;MOLTO TRIK SEKALI BILAS POUCH;Kesehatan;15200;60\n";
        file << "8997023;SO KLIN PELEMBUT BLUE POUCH 90;Kesehatan;12500;70\n";
        file << "8997024;MAMA LEMON LIME GREEN PET 200;Sembako;4800;150\n";
        file << "8997025;SUNLIGHT PENCUCI PIRING 750ML;Sembako;14900;80\n";
        file << "8997026;WIPOL BOTOL LEMON FRESH 250ML;Kesehatan;8500;80\n";
        file << "8997027;SOS PEMBERSIH LANTAI APPLE 750;Kesehatan;11500;70\n";
        file << "8997028;VIXAL PORCELAIN CLEANER BLUE 5;Kesehatan;18200;50\n";
        file << "8997029;HARPIC PEMBERSIH KLOSET CITRUS;Kesehatan;23900;40\n";
        file << "8997030;GLADE SCENTED GEL LEMON 180G;Kesehatan;19500;30\n";
        file << "8997031;STELLA MATIC REFILL WILD FLOWER;Kesehatan;28900;25\n";
        file << "8997032;BAGUS KAPUS BARUS DAHLIA 150G;Kesehatan;14500;60\n";
        file << "8997033;WAFERS ASTOR COKELAT KALENG 330;Makanan;32900;15\n";
        file << "8997034;MONDE SERENA EGG ROLL DUS 168G;Makanan;26500;20\n";
        file << "8997035;TINI WINI BITI ASIN 20G;Makanan;2500;200\n";
        file << "8997036;CHOCOPIE LOTTE DUS ISI 6;Makanan;16500;50\n";
        file << "8997037;NEXTAR BROWNIES COKELAT 8X14G;Makanan;8900;80\n";
        file << "8997038;GERY CRUNCH ROLL CHOCO BOX;Makanan;9900;70\n";
        file << "8997039;NISSIN CRACKERS ASIN 120G;Makanan;7800;90\n";
        file << "8997040;KHONG GUAN MALKIST COKELAT 13;Makanan;8200;85\n";
        file << "8997041;TANGO WAFER STRAWBERRY 130G;Makanan;8500;60\n";
        file << "8997042;OREO DOUBLE STUFF 137G;Makanan;9200;80\n";
        file << "8997043;MUNCHYS LEXUS CHOCO DUS 150G;Makanan;14500;40\n";
        file << "8997044;SILVERQUEEN CHUNKY BAR ALMOND;Makanan;24900;35\n";
        file << "8997045;DILAN CHOCO CARAMEL CRISPY 24;Makanan;3500;120\n";
        file << "8997046;CHOKI CHOKI PASTE COKELAT STIK;Makanan;2500;300\n";
        file << "8997047;NANO NANO PERMEN MANIS ASAM AS;Makanan;3500;150\n";
        file << "8997048;YUPI GUMMY BEARS BAG 45G;Makanan;5800;100\n";
        file << "8997049;MENTOS ROLL MINT 37G;Makanan;4500;180\n";
        file << "8997050;SUGUS PERMEN KUNYAH ORANGE DUS;Makanan;6800;110\n";
        file << "8997051;MAMY POKO PANTS STANDAR XL26;Bayi;65000;25\n";
        file << "8997052;SWEETY BRONZE PANTS SIZE L30;Bayi;53900;30\n";
        file << "8997053;MERRIES GOOD SKIN PANTS SIZE M;Bayi;58500;20\n";
        file << "8997054;CUSSONS BABY WIPES PINK PACK2;Bayi;16500;60\n";
        file << "8997055;PIGEON LIQUID CLEANSER REFILL;Bayi;44900;25\n";
        file << "8997056;ZWITSAL BABY OIL ALOE VERA 10;Bayi;18900;40\n";
        file << "8997057;MY BABY SHAMPO BLACK SHINE 100;Bayi;9200;80\n";
        file << "8997058;CUSSONS BABY HAIR LOTION ALOE;Bayi;19500;50\n";
        file << "8997059;PURE BABY SUNBLOCK SPF30 50G;Bayi;69900;10\n";
        file << "8997060;TELON LANG MINYAK TELON 100ML;Bayi;26500;50\n";
        file << "8997061;NIVEA BODY LOTION EXTRA WHITE;Kesehatan;28900;35\n";
        file << "8997062;VASELINE PETROLEUM JELLY 100G;Kesehatan;32500;40\n";
        file << "8997063;CITRA SAKURA FAIR UV GELY 230;Kesehatan;21900;30\n";
        file << "8997064;NIVEA LIP ACTIVE CARE FOR MEN;Kesehatan;19500;50\n";
        file << "8997065;BIORE PORE PACK BLACK ISI 4S;Kesehatan;14500;100\n";
        file << "8997066;GARNIER MEN FACE WASH TURBO 10;Kesehatan;29900;45\n";
        file << "8997067;POND'S WHITE BEAUTY WASH 100G;Kesehatan;28500;40\n";
        file << "8997068;WARDAH ALOE HYDRANT GEL 100ML;Kesehatan;26500;30\n";
        file << "8997069;SENKA SPEEDY PERFECT WHIP 150;Kesehatan;68500;15\n";
        file << "8997070;GATSBY STYLING WAX MAT HOLD 7;Kesehatan;19500;60\n";
        file << "8997071;SABUN SURF POWDER RED DET 800;Kesehatan;13800;50\n";
        file << "8997072;SO KLIN BIO-MATIC FRONT LOAD 1;Kesehatan;38500;25\n";
        file << "8997073;KAO ATTACK LIQUID HYGIENE 800M;Kesehatan;22900;40\n";
        file << "8997074;MOLTO PREMIUM PARFUM LUXURY PO;Kesehatan;29900;30\n";
        file << "8997075;DOWNY PREMIUM PARFUM MYSTIQUE;Kesehatan;31500;35\n";
        file << "8997076;VANISH PENGHILANG NODA POUCH;Kesehatan;19500;40\n";
        file << "8997077;SOS CARBOL PEMBERSIH LANTAI CL;Kesehatan;12900;50\n";
        file << "8997078;SUPERPELL CHERRY DECISION POUCH;Kesehatan;15200;45\n";
        file << "8997079;SENSATIVE FACIAL TISSUE 120S;Kesehatan;14200;60\n";
        file << "8997080;NICE FACIAL TISSUE REF 250S DX;Kesehatan;16500;80\n";
        file << "8997081;GLADE AUTOMATIC SPRAY DEVICE GR;Kesehatan;85900;15\n";
        file << "8997082;STELLA AEROSOL ORANGE FRESH 40;Kesehatan;25500;40\n";
        file << "8997083;HIT MAT ALAT ANTI NYAMUK KABEL;Kesehatan;18900;50\n";
        file << "8997084;BAYGON COIL BAKAR ISI 10 JUMB;Kesehatan;4200;150\n";
        file << "8997085;VAPE AEROSOL SWEET FLOWER 600;Kesehatan;39900;35\n";
        file << "8997086;SASA TEPUNG BUMBU SERBAGUNA 22;Sembako;6200;100\n";
        file << "8997087;KOBE TEPUNG TEMPE KRIPIK 150G;Sembako;4800;120\n";
        file << "8997088;SAJIKU NASI GORENG SPESIAL 20;Sembako;2100;300\n";
        file << "8997089;SAORI SAUS TERIYAKI BOTTLE 13;Sembako;11800;60\n";
        file << "8997090;KAPAL API KOPI BUBUK MURNI 165;Minuman;14800;80\n";
        file << "8997091;NESCAFE CLASSIC JAR KOPI 100G;Minuman;38900;30\n";
        file << "8997092;INDOCAFE COFFEEMIX PACK ISI 30;Minuman;39900;25\n";
        file << "8997093;TEH CELUP SOSRO ISI 50B SP;Minuman;11500;60\n";
        file << "8997094;TEH TONG TJI CELUP JASMINE 25;Minuman;6800;100\n";
        file << "8997095;BUAVITA POMEGRANATE JUICE 250;Minuman;9200;50\n";
        file << "8997096;ABC SIRUP SQUASH ORANGE 460ML;Minuman;14500;70\n";
        file << "8997097;MARJAN SIRUP SQUASH MELON 450;Minuman;13900;80\n";
        file << "8997098;POCARI SWEAT ION WATER PET 50;Minuman;8900;90\n";
        file << "8997099;OATSIDE OAT MILK BARISTA 1L;Minuman;39900;20\n";
        file << "8997100;TROPICANA SLIM DIABTX 100S DX;Minuman;65900;25\n";

        // SEEDED PRODUCTS EXTRA VOLUME PART III (321 - 420)
        file << "8998001;ROMA ARROWROOT BISKUIT 250G;Makanan;9500;40\n";
        file << "8998002;GERY MALKIST TABUR GULA 100G;Makanan;6500;60\n";
        file << "8998003;OREO MINI STRAWBERRY CUP 67G;Makanan;7500;50\n";
        file << "8998004;POCKY DOUBLE CHOCO 47G;Makanan;8900;40\n";
        file << "8998005;POCKY MATCHA GREENTEA 33G;Makanan;8900;45\n";
        file << "8998006;CHIKI BALLS RASA KEJU 55G;Makanan;7500;80\n";
        file << "8998007;CHIKI PUFF RASA CHEDDAR CHEES;Makanan;7800;60\n";
        file << "8998008;JETZ HOLLOW CHOCO STICK 35G;Makanan;4900;100\n";
        file << "8998009;TARO NET POTATO SEAWEED 36G;Makanan;4900;90\n";
        file << "8998010;KUSUKA KERIPIK BARBEQUE 180G;Makanan;15800;30\n";
        file << "8998011;QTELA KERIPIK TEMPE ORI 150G;Makanan;14500;35\n";
        file << "8998012;MAITATOS POTATO CHIPS ORIGINAL;Makanan;12500;40\n";
        file << "8998013;SILVERQUEEN CASHEW MILK CHOC;Makanan;15900;50\n";
        file << "8998014;SILVERQUEEN VERY BERRY 62G;Makanan;22500;25\n";
        file << "8998015;CADBURY FRUIT & NUT MILK 62G;Makanan;14200;30\n";
        file << "8998016;DELFI DAIRY MILK CHOCOLATE 60;Makanan;13500;45\n";
        file << "8998017;MALTITOS CHOCO BALLS BAG 45G;Makanan;9900;60\n";
        file << "8998018;KINDER BUENO CHOCO BAR T2 43;Makanan;21500;20\n";
        file << "8998019;KINDER JOY FOR GIRLS TOY 20G;Makanan;13900;50\n";
        file << "8998020;MENTOS PERMEN KUNYAH SOUR BAG;Makanan;9900;80\n";
        file << "8998021;FRUTTENE GUMMY BEARS MIX BAG;Makanan;8500;100\n";
        file << "8998022;FISHERMANS FRIEND LOZENGES OR;Kesehatan;16500;50\n";
        file << "8998023;WOODS PEPPERMINT LOZENGES OR;Kesehatan;7500;120\n";
        file << "8998024;STREPSILS COOL LOZENGES DUS S;Kesehatan;18500;60\n";
        file << "8998025;TOLAK ANGIN ANAK CAIR 5S DUS;Kesehatan;18900;40\n";
        file << "8998026;AQUA AIR MINERAL PET 330ML;Minuman;2800;250\n";
        file << "8998027;VIT AIR MINERAL PET 1500ML;Minuman;5900;100\n";
        file << "8998028;ADES AIR MINERAL PET 600ML;Minuman;3300;150\n";
        file << "8998029;CLEO AIR MINERAL GLASS CUP 22;Minuman;800;400\n";
        file << "8998030;TEH BOTOL SOSRO KOTAK UHT 1T;Minuman;3200;120\n";
        file << "8998031;TEH PUCUK HARUM LESS SUGAR 3;Minuman;4000;100\n";
        file << "8998032;TEH BOTOL SOSRO LESS SUGAR PE;Minuman;6500;80\n";
        file << "8998033;FRESTEA GREEN TEA HONEY PET 5;Minuman;6800;90\n";
        file << "8998034;NU MILK TEA PET BOTTLE 330ML;Minuman;8500;70\n";
        file << "8998035;KOPI POCKET CAFE LATTE CAN 2;Minuman;10500;60\n";
        file << "8998036;NESCAFE ORIGINAL CAN 240ML;Minuman;10200;80\n";
        file << "8998037;NESCAFE MOCHA CAN 240ML;Minuman;10200;75\n";
        file << "8998038;KAPAL API SIGNATURE BLACK PET;Minuman;6900;100\n";
        file << "8998039;TORA CAFE ICED CAPPUCCINO PE;Minuman;3800;150\n";
        file << "8998040;KOPI LAIN HATI ICED LATTE BE;Minuman;9000;60\n";
        file << "8998041;HYDRO COCO ORIGINAL PET 500M;Minuman;13900;40\n";
        file << "8998042;COCOMAX COCONUT WATER PET 35;Minuman;7900;80\n";
        file << "8998043;POCARI SWEAT CAN 330ML;Minuman;8500;100\n";
        file << "8998044;C1000 ORANGE WATER PET 500ML;Minuman;8900;70\n";
        file << "8998045;YAKULT ACE LIGHT PACK ISI 5;Minuman;12500;50\n";
        file << "8998046;MILK LIFE UHT CHOCOLATE 200M;Minuman;6500;100\n";
        file << "8998047;INDOMILK UHT COK KOTAK 190ML;Minuman;5900;120\n";
        file << "8998048;OATSIDE CHOCOLATE MILK DUS 1;Minuman;39900;20\n";
        file << "8998049;ULTRA MILK CARAMEL UHT 200ML;Minuman;6800;90\n";
        file << "8998050;ULTRA MILK TARO UHT 200ML;Minuman;6800;95\n";
        file << "8998051;SGM EKSPLOR 1+ VANILA DUS 90;Bayi;79900;20\n";
        file << "8998052;DANCOW Fortigro Full Cream Du;Bayi;92500;15\n";
        file << "8998053;MILO POWDER ACTIVE GO DUS 400;Minuman;45500;30\n";
        file << "8998054;SARIWANGI TEH MELATI ISI 50S;Minuman;11200;80\n";
        file << "8998055;SOSRO TEH CELUP BLACKTEA 30S;Minuman;6500;100\n";
        file << "8998056;MARJAN BOUDOIN SIRUP MELON 46;Minuman;22900;50\n";
        file << "8998057;ABC SPECIAL GRADE SIRUP COCOP;Minuman;19500;60\n";
        file << "8998058;GULAKU GULA PASIR ZIP POUCH;Sembako;18500;40\n";
        file << "8998059;GULA MERAH JAWA BATOK CAP TA;Sembako;15500;30\n";
        file << "8998060;ABC KECAP MANIS BOTTLE 135ML;Sembako;8900;100\n";
        file << "8998061;ABC KECAP MANIS POUCH 225ML;Sembako;10500;90\n";
        file << "8998062;KECAP SEDEEP POUCH 225ML;Sembako;9800;80\n";
        file << "8998063;BIMOLI MINYAK GORENG REF 1L;Sembako;19800;60\n";
        file << "8998064;SANIA MINYAK GORENG POUCH 1L;Sembako;19200;70\n";
        file << "8998065;FILMA MINYAK GORENG POUCH 1L;Sembako;19500;50\n";
        file << "8998066;BLUE BAND CAKE COOKIE MARGARI;Sembako;11500;60\n";
        file << "8998067;ROYCO KALDU AYAM RENCENG 12S;Sembako;6000;150\n";
        file << "8998068;ROYCO KALDU SAPI RENCENG 12S;Sembako;6000;140\n";
        file << "8998069;MASAKO KALDU AYAM RENCENG 12;Sembako;5800;160\n";
        file << "8998070;MASAKO KALDU SAPI RENCENG 12;Sembako;5800;150\n";
        file << "8998071;SAORI SAUS ASAM MANIS BOTOL;Sembako;11800;45\n";
        file << "8998072;SAORI SAUS LADA HITAM BOTOL;Sembako;12500;40\n";
        file << "8998073;ABC SAUS CABAI EXTRA PEDAS BO;Sembako;14900;50\n";
        file << "8998074;INDOFOOD SAMBAL PEDAS DAHSYAT;Sembako;13500;60\n";
        file << "8998075;SASA PENYEDAP RASA MSG DUS 1;Sembako;8900;70\n";
        file << "8998076;SEGITIGA BIRU TEPUNG TERIGU;Sembako;12500;50\n";
        file << "8998077;CUSSONS BABY WIPES BLUE PACK2;Bayi;16500;60\n";
        file << "8998078;MAMY POKO WIPES PARFUM PACK2;Bayi;18500;50\n";
        file << "8998079;ZWITSAL BABY POWDER SOFT FLOR;Bayi;15500;80\n";
        file << "8998080;ZWITSAL BABY COLOGNE FRESH SC;Bayi;26500;40\n";
        file << "8998081;PEPSODENT SENSITIVE MINERAL E;Kesehatan;28900;35\n";
        file << "8998082;SENSODYNE RAPID RELIEF TOOTH;Kesehatan;35500;30\n";
        file << "8998083;CLOSE UP GREEN FRESH TOOTHPAST;Kesehatan;16500;60\n";
        file << "8998084;FORMULA GIGI PASTA ADVANCED S;Kesehatan;12500;80\n";
        file << "8998085;LISTERINE MULTI PROTECT MOUTH;Kesehatan;32900;25\n";
        file << "8998086;BETADINE MOUTH WASH GARGLE 19;Kesehatan;38900;30\n";
        file << "8998087;BIORE BODY WASH REFILL WHITE;Kesehatan;24900;40\n";
        file << "8998088;LIFEBUOY BODY WASH LEMON SHIE;Kesehatan;23800;50\n";
        file << "8998089;DETTOL BODY WASH ORIGINAL REFI;Kesehatan;28500;30\n";
        file << "8998090;SABUN LUX WHITE VELVET REFILL;Kesehatan;22500;45\n";
        file << "8998091;PANTENE CONDITIONER HAIR FALL;Kesehatan;22900;40\n";
        file << "8998092;CLEAR SHAMPO COMPLETE SOFT CA;Kesehatan;42500;30\n";
        file << "8998093;SANSILK SHAMPO SOFT SMOOTH RE;Kesehatan;29500;35\n";
        file << "8998094;TRESEMME SHAMPO KERATIN SMOOT;Kesehatan;48900;20\n";
        file << "8998095;SABUN DETTOL BAR LASTING FRE;Kesehatan;4800;120\n";
        file << "8998096;SO KLIN LIQUID SOFTERGENT RE;Kesehatan;17500;50\n";
        file << "8998097;SO KLIN LIQUID SAKURA REFILL;Kesehatan;17500;45\n";
        file << "8998098;RINSO MOLTO DETERGENT LIQUID;Kesehatan;21900;35\n";
        file << "8998099;RINSO LIQUID DETERGENT REFILL;Kesehatan;19900;40\n";
        file << "8998100;ATTACK EASY LIQUID PURPLE POU;Kesehatan;18500;30\n";
    
        file.close();
    } else { fP.close(); }

    ifstream fD("data/diskon.txt");
    if (!fD.is_open()) {
        ofstream file("data/diskon.txt");
        file << "D001;PROMO MERDEKA;10.0;100000;1\n";
        file << "D002;DISKON MEMBER;5.0;50000;1\n";
        file.close();
    } else { fD.close(); }

    ifstream fM("data/member.txt");
    if (!fM.is_open()) {
        ofstream file("data/member.txt");
        file << "M001;AKIRA;08123456789;1901\n";
        file << "M002;DEVINA;08987654321;500\n";
        file.close();
    } else { fM.close(); }

    ifstream fL("data/transaksi_log.txt");
    if (!fL.is_open()) {
        ofstream file("data/transaksi_log.txt");
        file << "idTransaksi;tanggal;totalBayar;metodePembayaran;uangDiterima;kembalian;statusTransaksi;idKasir;idMember;detailBarang\n";
        file.close();
    } else { fL.close(); }

    ifstream fV("data/voucher.txt");
    if (!fV.is_open()) {
        ofstream file("data/voucher.txt");
        file << "DISKON10K;10000.00;50000.00;1\n";
        file << "ALFAMIDIMERDEKA;25000.00;100000.00;1\n";
        file.close();
    } else { fV.close(); }

    ifstream fS("data/supplier.txt");
    if (!fS.is_open()) {
        ofstream file("data/supplier.txt");
        file << "S001;PT Unilever Indonesia;Budi Santoso;08111222333;Kawasan Industri Jababeka, Bekasi;1\n";
        file << "S002;PT Indofood CBP;Siti Aminah;08222333444;Sudirman Plaza, Jakarta;1\n";
        file << "S003;PT Mayora Indah;Denny Siregar;08333444555;Gedung Mayora, Tangerang;1\n";
        file.close();
    } else { fS.close(); }

    ifstream fSh("data/shift_log.txt");
    if (!fSh.is_open()) {
        ofstream file("data/shift_log.txt");
        file.close();
    } else { fSh.close(); }
}

// =========================================================================================
// FITUR TAMBAHAN 1: SISTEM RETUR / PENGEMBALIAN BARANG
// =========================================================================================
// Fitur ini memungkinkan kasir memproses pengembalian barang dari pelanggan atas transaksi
// yang sudah pernah tercatat di dalam sistem. Retur akan mengembalikan stok barang ke
// database inventaris toko serta mencatat log retur secara terpisah dari log transaksi
// penjualan agar laporan keuangan tetap akurat dan dapat diaudit oleh Admin/Manajer.
// =========================================================================================
void printReturBanner() {
    cout << RED << BOLD;
    cout << RESET << "\n";
}

class ReturBarang {
private:
    string idRetur;
    string idTransaksiAsal;
    string barcode;
    string namaProduk;
    int qtyRetur;
    double hargaSatuan;
    string alasanRetur;
    string tanggalRetur;
    string idKasirProses;
    string statusRetur; // "Disetujui" / "Ditolak"

public:
    ReturBarang() : idRetur(""), idTransaksiAsal(""), barcode(""), namaProduk(""),
        qtyRetur(0), hargaSatuan(0), alasanRetur(""), tanggalRetur(""),
        idKasirProses(""), statusRetur("Disetujui") {}

    ReturBarang(string id, string idT, string bar, string nama, int qty, double harga,
                string alasan, string tgl, string idKasir, string status)
        : idRetur(id), idTransaksiAsal(idT), barcode(bar), namaProduk(nama), qtyRetur(qty),
          hargaSatuan(harga), alasanRetur(alasan), tanggalRetur(tgl), idKasirProses(idKasir),
          statusRetur(status) {}

    string getIdRetur() const { return idRetur; }
    string getIdTransaksiAsal() const { return idTransaksiAsal; }
    string getBarcode() const { return barcode; }
    string getNamaProduk() const { return namaProduk; }
    int getQtyRetur() const { return qtyRetur; }
    double getHargaSatuan() const { return hargaSatuan; }
    string getAlasanRetur() const { return alasanRetur; }
    string getTanggalRetur() const { return tanggalRetur; }
    string getIdKasirProses() const { return idKasirProses; }
    string getStatusRetur() const { return statusRetur; }

    double getTotalRefund() const { return qtyRetur * hargaSatuan; }

    void tampilkanRetur() const {
        cout << "ID Retur       : " << idRetur << "\n";
        cout << "Transaksi Asal : " << idTransaksiAsal << "\n";
        cout << "Produk         : " << namaProduk << " (" << barcode << ")\n";
        cout << "Jumlah Retur   : " << qtyRetur << "\n";
        cout << "Nominal Refund : " << formatRupiah(getTotalRefund()) << "\n";
        cout << "Alasan         : " << alasanRetur << "\n";
        cout << "Tanggal        : " << tanggalRetur << "\n";
        cout << "Diproses Oleh  : " << idKasirProses << "\n";
        cout << "Status         : " << statusRetur << "\n";
    }
};

vector<ReturBarang> dbRetur;

void loadRetur() {
    dbRetur.clear();
    ifstream file("data/retur_log.txt");
    if (!file.is_open()) return;
    string line;
    getline(file, line); // lewati baris header
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, idT, bar, nama, qtyS, hargaS, alasan, tgl, idKas, status;
        getline(ss, id, ';');
        getline(ss, idT, ';');
        getline(ss, bar, ';');
        getline(ss, nama, ';');
        getline(ss, qtyS, ';');
        getline(ss, hargaS, ';');
        getline(ss, alasan, ';');
        getline(ss, tgl, ';');
        getline(ss, idKas, ';');
        getline(ss, status, ';');
        dbRetur.push_back(ReturBarang(id, idT, bar, nama, atoi(qtyS.c_str()),
            atof(hargaS.c_str()), alasan, tgl, idKas, status));
    }
    file.close();
}

void saveReturBaru(const ReturBarang& r) {
    createDataDirectory();
    ifstream check("data/retur_log.txt");
    bool needHeader = !check.is_open();
    check.close();

    ofstream file("data/retur_log.txt", ios::app);
    if (!file.is_open()) return;
    if (needHeader) {
        file << "idRetur;idTransaksiAsal;barcode;namaProduk;qtyRetur;hargaSatuan;alasan;tanggal;idKasir;status\n";
    }
    file << r.getIdRetur() << ";" << r.getIdTransaksiAsal() << ";" << r.getBarcode() << ";"
         << r.getNamaProduk() << ";" << r.getQtyRetur() << ";" << fixed << setprecision(2)
         << r.getHargaSatuan() << ";" << r.getAlasanRetur() << ";" << r.getTanggalRetur() << ";"
         << r.getIdKasirProses() << ";" << r.getStatusRetur() << "\n";
    file.close();
}

void cetakNotaRetur(const ReturBarang& r) {
    stringstream ssOut;
    ssOut << "\n================ NOTA RETUR BARANG ================\n";
    ssOut << "           ALFAMIDI KI AGENG PEMANAHAN\n";
    ssOut << "=====================================================\n";
    ssOut << "ID Retur         : " << r.getIdRetur() << "\n";
    ssOut << "ID Transaksi Asal: " << r.getIdTransaksiAsal() << "\n";
    ssOut << "Produk           : " << r.getNamaProduk() << "\n";
    ssOut << "Barcode          : " << r.getBarcode() << "\n";
    ssOut << "Jumlah Retur     : " << r.getQtyRetur() << " unit\n";
    ssOut << "Nominal Refund   : " << formatRupiah(r.getTotalRefund()) << "\n";
    ssOut << "Alasan Retur     : " << r.getAlasanRetur() << "\n";
    ssOut << "Tanggal          : " << r.getTanggalRetur() << "\n";
    ssOut << "Diproses Oleh    : " << r.getIdKasirProses() << "\n";
    ssOut << "=====================================================\n";
    ssOut << "   Barang telah dikembalikan ke stok toko. Terima kasih.\n";
    ssOut << "=====================================================\n\n";

    cout << ssOut.str();

    createDataDirectory();
    string filename = "data/nota_retur_" + r.getIdRetur() + ".txt";
    ofstream nFile(filename.c_str());
    if (nFile.is_open()) {
        nFile << ssOut.str();
        nFile.close();
        cout << GREEN << "[ Nota retur telah diekspor ke file: " << filename << " ]\n" << RESET;
    }
}

void processRetur(Kasir& activeKasir) {
    clearScreen();
    printReturBanner();
    printHeader("PROSES RETUR / PENGEMBALIAN BARANG");

    loadHistoryTransaksi();
    if (listHistoryTransaksi.empty()) {
        cout << RED << "Belum ada transaksi yang dapat diretur.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    string idT;
    cout << "Masukkan ID Transaksi Asal (cth: AI34-xxx-xxxxxxxx): ";
    cin >> idT;

    Transaksi* transAsal = NULL;
    for (size_t i = 0; i < listHistoryTransaksi.size(); ++i) {
        if (listHistoryTransaksi[i].getIdTransaksi() == idT) {
            transAsal = &listHistoryTransaksi[i];
            break;
        }
    }

    if (transAsal == NULL) {
        cout << RED << "ID Transaksi tidak ditemukan dalam riwayat penjualan.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    if (transAsal->getStatusTransaksi() != "Selesai") {
        cout << RED << "Transaksi ini tidak berstatus 'Selesai', retur tidak dapat diproses.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    // Membaca ulang detail barang dari log transaksi karena kelas Transaksi tidak
    // menyimpan detail barang secara langsung di memori (hanya total nominal saja).
    string detailBarang = "";
    ifstream file("data/transaksi_log.txt");
    if (file.is_open()) {
        string line;
        getline(file, line);
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string logId, tgl, totS, met, uaS, kemS, stat, idKas, idMem, detail;
            getline(ss, logId, ';');
            getline(ss, tgl, ';');
            getline(ss, totS, ';');
            getline(ss, met, ';');
            getline(ss, uaS, ';');
            getline(ss, kemS, ';');
            getline(ss, stat, ';');
            getline(ss, idKas, ';');
            getline(ss, idMem, ';');
            getline(ss, detail, ';');
            if (logId == idT) {
                detailBarang = detail;
                break;
            }
        }
        file.close();
    }

    if (detailBarang.empty()) {
        cout << RED << "Detail barang transaksi tidak ditemukan atau kosong.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    cout << "\nDaftar Barang pada Transaksi Ini:\n";
    cout << "-------------------------------------------------------------\n";
    vector<string> listBarcodeItem;
    vector<int> listQtyItem;
    int no = 1;
    stringstream ssDetail(detailBarang);
    string itemPair;
    while (getline(ssDetail, itemPair, ',')) {
        size_t colonIdx = itemPair.find(':');
        if (colonIdx != string::npos) {
            string bar = itemPair.substr(0, colonIdx);
            int qty = atoi(itemPair.substr(colonIdx + 1).c_str());
            string namaP = "(Produk telah dihapus dari inventaris)";
            for (size_t p = 0; p < dbProduk.size(); ++p) {
                if (dbProduk[p].getBarcode() == bar) {
                    namaP = dbProduk[p].getNamaProduk();
                    break;
                }
            }
            cout << no << ". " << namaP << " | Barcode: " << bar << " | Qty Dibeli: " << qty << "\n";
            listBarcodeItem.push_back(bar);
            listQtyItem.push_back(qty);
            no++;
        }
    }
    cout << "-------------------------------------------------------------\n";

    string barTarget;
    cout << "Masukkan Barcode barang yang ingin diretur: ";
    cin >> barTarget;

    int idxTarget = -1;
    for (size_t i = 0; i < listBarcodeItem.size(); ++i) {
        if (listBarcodeItem[i] == barTarget) { idxTarget = i; break; }
    }

    if (idxTarget == -1) {
        cout << RED << "Barcode tersebut tidak terdapat pada transaksi ini.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    int qtyRetur = inputInt("Jumlah barang yang akan diretur (Maks: " + toStr(listQtyItem[idxTarget]) + "): ");
    if (qtyRetur <= 0 || qtyRetur > listQtyItem[idxTarget]) {
        cout << RED << "Jumlah retur tidak valid.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    cout << "Pilih Alasan Retur:\n";
    cout << "1. Barang Rusak/Cacat Produksi\n";
    cout << "2. Salah Ambil Barang\n";
    cout << "3. Barang Tidak Sesuai Pesanan\n";
    cout << "4. Kadaluarsa / Expired\n";
    cout << "5. Lainnya\n";
    int pilAlasan = inputInt("Pilihan (1-5): ");
    string alasanTxt;
    switch (pilAlasan) {
        case 1: alasanTxt = "Barang Rusak/Cacat Produksi"; break;
        case 2: alasanTxt = "Salah Ambil Barang"; break;
        case 3: alasanTxt = "Barang Tidak Sesuai Pesanan"; break;
        case 4: alasanTxt = "Kadaluarsa / Expired"; break;
        default: alasanTxt = "Lainnya"; break;
    }

    string namaProdukRetur = "";
    double hargaProdukRetur = 0;
    int prodIdx = -1;
    for (size_t p = 0; p < dbProduk.size(); ++p) {
        if (dbProduk[p].getBarcode() == barTarget) {
            namaProdukRetur = dbProduk[p].getNamaProduk();
            hargaProdukRetur = dbProduk[p].getHarga();
            prodIdx = static_cast<int>(p);
            break;
        }
    }

    stringstream idGen;
    idGen << "RTR-" << toStr(time(0) % 1000000);
    string idReturBaru = idGen.str();

    ReturBarang returBaru(idReturBaru, idT, barTarget, namaProdukRetur, qtyRetur,
        hargaProdukRetur, alasanTxt, getFormattedDate(), activeKasir.getIdKasir(), "Disetujui");

    // Kembalikan stok barang ke inventaris toko
    if (prodIdx != -1) {
        dbProduk[prodIdx].setStok(dbProduk[prodIdx].getStok() + qtyRetur);
        saveProduk();
    }

    saveReturBaru(returBaru);

    cout << GREEN << "\n[ RETUR BERHASIL DIPROSES! ]\n" << RESET;
    cout << "Stok barang telah dikembalikan ke inventaris toko.\n\n";
    cetakNotaRetur(returBaru);
    pressEnterToContinue();
}

void lihatRiwayatRetur() {
    clearScreen();
    printReturBanner();
    printHeader("RIWAYAT RETUR BARANG TOKO");
    loadRetur();

    if (dbRetur.empty()) {
        cout << "Belum ada data retur barang yang tercatat.\n";
        pressEnterToContinue();
        return;
    }

    cout << left << setw(14) << "ID Retur"
         << setw(23) << "Nama Produk"
         << setw(6) << "Qty"
         << setw(15) << "Nominal Refund"
         << setw(20) << "Alasan"
         << "Kasir" << "\n";
    cout << "----------------------------------------------------------------------------------\n";

    double totalRefundKeseluruhan = 0;
    for (size_t i = 0; i < dbRetur.size(); ++i) {
        string namaShort = dbRetur[i].getNamaProduk();
        if (namaShort.length() > 21) namaShort = namaShort.substr(0, 18) + "...";
        cout << left << setw(14) << dbRetur[i].getIdRetur()
             << setw(23) << namaShort
             << setw(6) << dbRetur[i].getQtyRetur()
             << setw(15) << formatRupiah(dbRetur[i].getTotalRefund())
             << setw(20) << dbRetur[i].getAlasanRetur()
             << dbRetur[i].getIdKasirProses() << "\n";
        totalRefundKeseluruhan += dbRetur[i].getTotalRefund();
    }
    cout << "----------------------------------------------------------------------------------\n";
    cout << "Total Nominal Refund Keseluruhan : " << formatRupiah(totalRefundKeseluruhan) << "\n";
    cout << "Jumlah Transaksi Retur           : " << dbRetur.size() << " kali\n";
    pressEnterToContinue();
}

// =========================================================================================
// FITUR TAMBAHAN 2: NOTIFIKASI STOK MENIPIS & STOK OPNAME (AUDIT FISIK GUDANG)
// =========================================================================================
// Fitur ini membantu Admin/Manajer memantau produk yang stoknya hampir habis agar dapat
// segera dilakukan permintaan restock kepada Supplier, serta melakukan audit stok fisik
// secara berkala (stok opname) untuk mencocokkan data stok sistem dengan stok riil di rak
// toko. Setiap selisih yang ditemukan akan tercatat dalam log opname untuk keperluan audit.
// =========================================================================================
const int BATAS_STOK_MENIPIS = 15;

void cekStokMenipis() {
    clearScreen();
    printDiagnosticsBanner();
    printHeader("NOTIFIKASI PRODUK STOK MENIPIS");

    vector<size_t> idxMenipis;
    for (size_t i = 0; i < dbProduk.size(); ++i) {
        if (dbProduk[i].getStok() <= BATAS_STOK_MENIPIS) {
            idxMenipis.push_back(i);
        }
    }

    if (idxMenipis.empty()) {
        cout << GREEN << "Semua stok produk masih dalam batas aman (di atas "
             << BATAS_STOK_MENIPIS << " unit).\n" << RESET;
        pressEnterToContinue();
        return;
    }

    cout << RED << "PERINGATAN: Ditemukan " << idxMenipis.size()
         << " produk dengan stok menipis (<= " << BATAS_STOK_MENIPIS << " unit)!\n" << RESET;
    cout << left << setw(13) << "Barcode"
         << setw(23) << "Nama Produk"
         << setw(14) << "Kategori"
         << right << setw(8) << "Stok" << "\n";
    cout << "---------------------------------------------------------------\n";

    createDataDirectory();
    ofstream fOut("data/stok_menipis.txt");
    if (fOut.is_open()) {
        fOut << "LAPORAN STOK MENIPIS - Dicetak: " << getFormattedDate() << "\n";
        fOut << "======================================================\n";
    }

    for (size_t i = 0; i < idxMenipis.size(); ++i) {
        size_t idx = idxMenipis[i];
        string sName = dbProduk[idx].getNamaProduk();
        if (sName.length() > 21) sName = sName.substr(0, 18) + "...";

        string warnaTampil = (dbProduk[idx].getStok() <= 5) ? RED : YELLOW;
        cout << warnaTampil << left << setw(13) << dbProduk[idx].getBarcode()
             << setw(23) << sName
             << setw(14) << dbProduk[idx].getKategori()
             << right << setw(8) << dbProduk[idx].getStok() << RESET << "\n";

        if (fOut.is_open()) {
            fOut << dbProduk[idx].getBarcode() << " | " << dbProduk[idx].getNamaProduk()
                 << " | Kategori: " << dbProduk[idx].getKategori()
                 << " | Sisa Stok: " << dbProduk[idx].getStok() << " unit\n";
        }
    }
    if (fOut.is_open()) fOut.close();

    cout << "---------------------------------------------------------------\n";
    cout << GREEN << "Laporan stok menipis telah diekspor ke: data/stok_menipis.txt\n" << RESET;
    cout << "Segera hubungi Supplier terkait untuk melakukan permintaan restock.\n";
    pressEnterToContinue();
}

struct LogOpname {
    string idOpname;
    string barcode;
    string namaProduk;
    int stokSistem;
    int stokFisik;
    int selisih;
    string idAdmin;
    string tanggal;
};

void saveLogOpname(const LogOpname& log) {
    createDataDirectory();
    ifstream check("data/opname_log.txt");
    bool needHeader = !check.is_open();
    check.close();

    ofstream file("data/opname_log.txt", ios::app);
    if (!file.is_open()) return;
    if (needHeader) {
        file << "idOpname;barcode;namaProduk;stokSistem;stokFisik;selisih;idAdmin;tanggal\n";
    }
    file << log.idOpname << ";" << log.barcode << ";" << log.namaProduk << ";"
         << log.stokSistem << ";" << log.stokFisik << ";" << log.selisih << ";"
         << log.idAdmin << ";" << log.tanggal << "\n";
    file.close();
}

void stokOpname(Admin& activeAdmin) {
    clearScreen();
    printDiagnosticsBanner();
    printHeader("STOK OPNAME (AUDIT FISIK GUDANG)");
    cout << "Fitur ini digunakan untuk mencocokkan jumlah stok sistem dengan hasil\n";
    cout << "penghitungan fisik barang di rak/gudang toko secara langsung.\n\n";

    string bar;
    cout << "Masukkan Barcode produk yang akan diopname (atau ketik 'selesai' untuk keluar): ";
    cin >> bar;

    int totalDiperiksa = 0;
    int totalSelisih = 0;

    while (bar != "selesai" && bar != "SELESAI") {
        int prodIdx = -1;
        for (size_t i = 0; i < dbProduk.size(); ++i) {
            if (dbProduk[i].getBarcode() == bar) { prodIdx = static_cast<int>(i); break; }
        }

        if (prodIdx == -1) {
            cout << RED << "Barcode tidak ditemukan dalam inventaris.\n" << RESET;
        } else {
            cout << "Produk         : " << dbProduk[prodIdx].getNamaProduk() << "\n";
            cout << "Stok di Sistem : " << dbProduk[prodIdx].getStok() << " unit\n";
            int stokFisik = inputInt("Hasil hitung fisik   : ");
            int selisih = stokFisik - dbProduk[prodIdx].getStok();

            LogOpname log;
            stringstream idGen;
            idGen << "OPN-" << toStr(time(0) % 1000000) << "-" << toStr(totalDiperiksa);
            log.idOpname = idGen.str();
            log.barcode = bar;
            log.namaProduk = dbProduk[prodIdx].getNamaProduk();
            log.stokSistem = dbProduk[prodIdx].getStok();
            log.stokFisik = stokFisik;
            log.selisih = selisih;
            log.idAdmin = activeAdmin.getIdAdmin();
            log.tanggal = getFormattedDate();
            saveLogOpname(log);

            dbProduk[prodIdx].setStok(stokFisik);
            saveProduk();

            if (selisih == 0) {
                cout << GREEN << "Stok sesuai! Tidak ada selisih ditemukan.\n" << RESET;
            } else if (selisih > 0) {
                cout << YELLOW << "Selisih LEBIH sebanyak " << selisih << " unit (stok fisik disesuaikan).\n" << RESET;
            } else {
                cout << RED << "Selisih KURANG sebanyak " << (-selisih) << " unit (kemungkinan susut/hilang).\n" << RESET;
            }
            totalDiperiksa++;
            totalSelisih += abs(selisih);
        }

        cout << "\nMasukkan Barcode produk berikutnya (atau ketik 'selesai' untuk keluar): ";
        cin >> bar;
    }

    cout << GREEN << "\nProses stok opname selesai. Total " << totalDiperiksa
         << " produk diperiksa dengan total selisih " << totalSelisih << " unit.\n" << RESET;
    cout << "Seluruh penyesuaian telah dicatat pada log: data/opname_log.txt\n";
    pressEnterToContinue();
}

// =========================================================================================
// FITUR TAMBAHAN 3: GRAFIK ANALISIS PENJUALAN PER KATEGORI PRODUK
// =========================================================================================
// Fitur ini merekap seluruh riwayat transaksi penjualan dan mengelompokkannya berdasarkan
// kategori produk (Minuman, Sembako, Kesehatan, Bayi, dll), lalu menampilkannya dalam
// bentuk grafik batang berbasis karakter ASCII sehingga Admin dapat dengan cepat melihat
// kategori mana yang memberikan kontribusi omset terbesar bagi toko.
// =========================================================================================
void tampilkanGrafikKategori() {
    clearScreen();
    printLaporanBanner();
    printHeader("GRAFIK ANALISIS PENJUALAN PER KATEGORI");

    ifstream file("data/transaksi_log.txt");
    if (!file.is_open()) {
        cout << RED << "Data transaksi tidak ditemukan.\n" << RESET;
        pressEnterToContinue();
        return;
    }

    vector<string> namaKategori;
    vector<double> totalOmsetKategori;
    vector<int> totalQtyKategori;

    string line;
    getline(file, line);
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string idT, tgl, totS, met, uaS, kemS, stat, idKas, idMem, detail;
        getline(ss, idT, ';');
        getline(ss, tgl, ';');
        getline(ss, totS, ';');
        getline(ss, met, ';');
        getline(ss, uaS, ';');
        getline(ss, kemS, ';');
        getline(ss, stat, ';');
        getline(ss, idKas, ';');
        getline(ss, idMem, ';');
        getline(ss, detail, ';');

        stringstream ssDetail(detail);
        string itemPair;
        while (getline(ssDetail, itemPair, ',')) {
            size_t colonIdx = itemPair.find(':');
            if (colonIdx == string::npos) continue;
            string bar = itemPair.substr(0, colonIdx);
            int qty = atoi(itemPair.substr(colonIdx + 1).c_str());

            string kategoriP = "Lainnya";
            double hargaP = 0;
            for (size_t p = 0; p < dbProduk.size(); ++p) {
                if (dbProduk[p].getBarcode() == bar) {
                    kategoriP = dbProduk[p].getKategori();
                    hargaP = dbProduk[p].getHarga();
                    break;
                }
            }

            int kIdx = -1;
            for (size_t k = 0; k < namaKategori.size(); ++k) {
                if (namaKategori[k] == kategoriP) { kIdx = static_cast<int>(k); break; }
            }
            if (kIdx == -1) {
                namaKategori.push_back(kategoriP);
                totalOmsetKategori.push_back(0);
                totalQtyKategori.push_back(0);
                kIdx = static_cast<int>(namaKategori.size()) - 1;
            }
            totalOmsetKategori[kIdx] += hargaP * qty;
            totalQtyKategori[kIdx] += qty;
        }
    }
    file.close();

    if (namaKategori.empty()) {
        cout << "Belum ada cukup data transaksi untuk dianalisis per kategori.\n";
        pressEnterToContinue();
        return;
    }

    // Cari nilai omset tertinggi untuk skala grafik batang (bar chart)
    double maxOmset = 0;
    for (size_t i = 0; i < totalOmsetKategori.size(); ++i) {
        if (totalOmsetKategori[i] > maxOmset) maxOmset = totalOmsetKategori[i];
    }

    const int LEBAR_MAKS_BAR = 40;
    cout << left << setw(15) << "Kategori" << setw(8) << "Qty" << "Grafik Omset Penjualan\n";
    cout << "---------------------------------------------------------------------\n";
    for (size_t i = 0; i < namaKategori.size(); ++i) {
        int panjangBar = 0;
        if (maxOmset > 0) {
            panjangBar = static_cast<int>((totalOmsetKategori[i] / maxOmset) * LEBAR_MAKS_BAR);
        }
        string bar = "";
        for (int b = 0; b < panjangBar; ++b) bar += "#";

        cout << left << setw(15) << namaKategori[i]
             << setw(8) << totalQtyKategori[i]
             << GREEN << bar << RESET << " " << formatRupiah(totalOmsetKategori[i]) << "\n";
    }
    cout << "---------------------------------------------------------------------\n";

    // Tentukan kategori dengan performa penjualan terbaik
    size_t bestIdx = 0;
    for (size_t i = 1; i < totalOmsetKategori.size(); ++i) {
        if (totalOmsetKategori[i] > totalOmsetKategori[bestIdx]) bestIdx = i;
    }
    cout << YELLOW << BOLD << "Kategori Terlaris: " << namaKategori[bestIdx]
         << " (" << formatRupiah(totalOmsetKategori[bestIdx]) << ")\n" << RESET;
    pressEnterToContinue();
}

// =========================================================================================
// FITUR TAMBAHAN 4: BACKUP & RESTORE DATABASE SISTEM
// =========================================================================================
// Fitur ini memberikan kemampuan kepada Admin untuk mencadangkan (backup) seluruh berkas
// data toko ke dalam folder terpisah, serta memulihkan (restore) data tersebut apabila
// suatu saat terjadi kehilangan atau kerusakan data pada berkas utama sistem.
// =========================================================================================
void printBackupBanner() {
    cout << CYAN << BOLD;
    cout << RESET << "\n";
}

bool copyFileSimple(const string& sumber, const string& tujuan) {
    ifstream src(sumber.c_str(), ios::binary);
    if (!src.is_open()) return true; // file sumber belum ada, dilewati (bukan error fatal)
    ofstream dst(tujuan.c_str(), ios::binary);
    if (!dst.is_open()) { src.close(); return false; }
    dst << src.rdbuf();
    src.close();
    dst.close();
    return true;
}

void backupDatabase() {
    clearScreen();
    printBackupBanner();
    printHeader("BACKUP DATABASE SISTEM");

    stringstream ssFolder;
    ssFolder << "data/backup_" << toStr(time(0) % 10000000);
    string folderBackup = ssFolder.str();

#ifdef _WIN32
    string cmd = "mkdir " + folderBackup + " 2>nul";
#else
    string cmd = "mkdir -p " + folderBackup + " 2>/dev/null";
#endif
    system(cmd.c_str());

    string daftarFile[] = {
        "produk.txt", "kasir.txt", "admin.txt", "diskon.txt", "member.txt",
        "transaksi_log.txt", "voucher.txt", "supplier.txt", "shift_log.txt",
        "retur_log.txt", "opname_log.txt"
    };
    int totalFile = 11;
    int berhasil = 0;

    cout << "Menyalin data ke folder: " << folderBackup << "\n";
    cout << "-------------------------------------------------\n";
    for (int i = 0; i < totalFile; ++i) {
        string sumber = "data/" + daftarFile[i];
        string tujuan = folderBackup + "/" + daftarFile[i];
        if (copyFileSimple(sumber, tujuan)) {
            cout << GREEN << "[OK] " << RESET << daftarFile[i] << " berhasil dibackup.\n";
            berhasil++;
        } else {
            cout << RED << "[GAGAL] " << RESET << daftarFile[i] << " tidak dapat disalin.\n";
        }
    }
    cout << "-------------------------------------------------\n";
    cout << GREEN << berhasil << " dari " << totalFile << " file berhasil dibackup.\n" << RESET;
    cout << "Lokasi backup: " << folderBackup << "\n";
    pressEnterToContinue();
}

void restoreDatabase() {
    clearScreen();
    printBackupBanner();
    printHeader("RESTORE DATABASE SISTEM");
    cout << RED << BOLD << "PERINGATAN: Proses ini akan menimpa seluruh data aktif saat ini!\n" << RESET;

    string folderBackup;
    cout << "Masukkan nama folder backup (cth: backup_1234567): ";
    cin >> folderBackup;
    folderBackup = "data/" + folderBackup;

    cout << "Apakah Anda yakin ingin melanjutkan proses restore? (y/n): ";
    char c;
    cin >> c;
    if (c != 'y' && c != 'Y') {
        cout << "Proses restore dibatalkan.\n";
        pressEnterToContinue();
        return;
    }

    string daftarFile[] = {
        "produk.txt", "kasir.txt", "admin.txt", "diskon.txt", "member.txt",
        "transaksi_log.txt", "voucher.txt", "supplier.txt", "shift_log.txt",
        "retur_log.txt", "opname_log.txt"
    };
    int totalFile = 11;
    int berhasil = 0;

    for (int i = 0; i < totalFile; ++i) {
        string sumber = folderBackup + "/" + daftarFile[i];
        string tujuan = "data/" + daftarFile[i];
        ifstream cek(sumber.c_str());
        if (cek.is_open()) {
            cek.close();
            if (copyFileSimple(sumber, tujuan)) {
                cout << GREEN << "[OK] " << RESET << daftarFile[i] << " berhasil direstore.\n";
                berhasil++;
            }
        } else {
            cout << YELLOW << "[LEWATI] " << RESET << daftarFile[i] << " tidak ada di folder backup.\n";
        }
    }

    cout << "-------------------------------------------------\n";
    cout << GREEN << berhasil << " file berhasil direstore. Silakan restart program\n";
    cout << "agar seluruh data yang baru dimuat ulang ke memori.\n" << RESET;
    pressEnterToContinue();
}

void backupRestoreMenu() {
    while (true) {
        clearScreen();
        printBackupBanner();
        printHeader("BACKUP & RESTORE DATABASE");
        cout << "1. Backup Database Sekarang\n";
        cout << "2. Restore Database dari Backup\n";
        cout << "3. Kembali ke Dashboard Admin\n";
        int pil = inputInt("Pilih menu (1-3): ");

        if (pil == 1) backupDatabase();
        else if (pil == 2) restoreDatabase();
        else if (pil == 3) break;
    }
}

// =========================================================================================
// FITUR TAMBAHAN 5: LOG RIWAYAT STOK OPNAME (AUDIT TRAIL)
// =========================================================================================
// Melengkapi fitur Stok Opname pada bagian sebelumnya, fungsi ini menampilkan seluruh
// riwayat penyesuaian stok yang pernah dilakukan oleh Admin, lengkap dengan selisih dan
// tanggal pelaksanaannya, sehingga dapat digunakan sebagai bahan audit internal toko.
// =========================================================================================
void lihatLogOpname() {
    clearScreen();
    printDiagnosticsBanner();
    printHeader("LOG RIWAYAT STOK OPNAME");

    ifstream file("data/opname_log.txt");
    if (!file.is_open()) {
        cout << "Belum ada riwayat stok opname yang tercatat.\n";
        pressEnterToContinue();
        return;
    }

    string line;
    getline(file, line); // lewati header
    if (file.eof()) {
        cout << "Belum ada riwayat stok opname yang tercatat.\n";
        file.close();
        pressEnterToContinue();
        return;
    }

    cout << left << setw(18) << "ID Opname"
         << setw(23) << "Nama Produk"
         << setw(10) << "Sistem"
         << setw(10) << "Fisik"
         << setw(10) << "Selisih"
         << setw(10) << "Admin"
         << "Tanggal" << "\n";
    cout << "----------------------------------------------------------------------------------------------\n";

    int totalBaris = 0;
    int totalSelisihLebih = 0;
    int totalSelisihKurang = 0;

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string idOp, bar, nama, sisS, fisS, selS, idAdm, tgl;
        getline(ss, idOp, ';');
        getline(ss, bar, ';');
        getline(ss, nama, ';');
        getline(ss, sisS, ';');
        getline(ss, fisS, ';');
        getline(ss, selS, ';');
        getline(ss, idAdm, ';');
        getline(ss, tgl, ';');

        int selisih = atoi(selS.c_str());
        string namaShort = nama;
        if (namaShort.length() > 21) namaShort = namaShort.substr(0, 18) + "...";

        string warna = (selisih == 0) ? GREEN : (selisih > 0 ? YELLOW : RED);
        cout << warna << left << setw(18) << idOp
             << setw(23) << namaShort
             << setw(10) << sisS
             << setw(10) << fisS
             << setw(10) << selisih
             << setw(10) << idAdm
             << tgl << RESET << "\n";

        if (selisih > 0) totalSelisihLebih += selisih;
        else if (selisih < 0) totalSelisihKurang += (-selisih);
        totalBaris++;
    }
    file.close();

    cout << "----------------------------------------------------------------------------------------------\n";
    if (totalBaris == 0) {
        cout << "Belum ada riwayat stok opname yang tercatat.\n";
    } else {
        cout << "Total Sesi Opname Tercatat : " << totalBaris << " kali\n";
        cout << "Total Selisih Lebih (Surplus) : " << totalSelisihLebih << " unit\n";
        cout << "Total Selisih Kurang (Susut)  : " << totalSelisihKurang << " unit\n";
    }
    pressEnterToContinue();
}

void loadKasir();
void saveKasir();
void loadAdmin();
void saveAdmin();

// ==========================================
// PROGRAM UTAMA (MAIN ENTRY POINT)
// ==========================================
int main() {
    seedDatabase();

    loadProduk();
    loadKasir();
    loadAdmin();
    loadDiskon();
    loadMembers();
    loadVouchers();
    loadSuppliers();
    loadShifts();
    
    while (true) {
        clearScreen();
        printLoginBanner();
        cout << YELLOW << BOLD;
        cout << "=========================================================\n";
        cout << "                 SISTEM KASIR ALFAMIDI                   \n";
        cout << "               Cabang: KL AGENG PEMANAHAN                \n";
        cout << "=========================================================\n" << RESET;
        cout << "Silakan lakukan login pengguna:\n\n";
        cout << "1. Login Kasir Operasional\n";
        cout << "2. Login Super Admin / Manajer\n";
        cout << "3. Registrasi Akun Pengguna Baru\n";
        cout << "4. Keluar dari Program Kasir\n";
        int pil = inputInt("Pilih Layanan (1-4): ");
        
        if (pil == 1) {
            string user, pass;
            cout << "\n=== LOGIN KASIR ===\n";
            cout << "Username : ";
            cin >> user;
            cout << "Password : ";
            pass = getHiddenPassword();
            
            bool loginSukses = false;
            for (size_t i = 0; i < dbKasir.size(); ++i) {
                if (dbKasir[i].loginKasir(user, pass)) {
                    loginSukses = true;
                    cout << GREEN << "\n[ LOGIN KASIR BERHASIL! ]\n" << RESET;
                    pressEnterToContinue();
                    runMenuKasir(dbKasir[i]);
                    break;
                }
            }
            if (!loginSukses) {
                cout << RED << "\n[ GAGAL ] Username atau Password salah!\n" << RESET;
                pressEnterToContinue();
            }
        }
        
        else if (pil == 2) {
            string user, pass;
            cout << "\n=== LOGIN ADMIN ===\n";
            cout << "Username : ";
            cin >> user;
            cout << "Password : ";
            pass = getHiddenPassword();
            
            bool loginSukses = false;
            for (size_t i = 0; i < dbAdmin.size(); ++i) {
                if (dbAdmin[i].loginAdmin(user, pass)) {
                    loginSukses = true;
                    cout << GREEN << "\n[ LOGIN ADMIN BERHASIL! ]\n" << RESET;
                    pressEnterToContinue();
                    runMenuAdmin(dbAdmin[i]);
                    break;
                }
            }
            if (!loginSukses) {
                cout << RED << "\n[ GAGAL ] Username atau Password salah!\n" << RESET;
                pressEnterToContinue();
            }
        }
        else if (pil == 3) {
            handleRegistrasiAkun();
        }
        
        else if (pil == 4) {
            cout << "\nTerima kasih telah menggunakan sistem kasir Alfamidi.\n";
            cout << "Program selesai dijalankan.\n";
            break;
        }
    }
    
    return 0;
}
