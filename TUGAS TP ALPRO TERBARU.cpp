#include <iostream>
#include <vector>
#include <string>


using namespace std;

// =====================
// CLASS AKUN
// =====================
class Akun {
private:
	
    string username;
    string password;
    string namaLengkap;
    string role;
    string noTelp;
    
public:
	
    Akun() {
        username = "";
        password = "";
        namaLengkap = "";
        role = "";
        noTelp = "";
        
    }

    Akun(string u, string p, string n, string r, string t) {
        username = u;
        password = p;
        namaLengkap = n;
        role = r;
        noTelp = t;
        
    }

    string getUsername() {
        return username;
    }

    string getPassword() {
        return password;
    }

    string getRole() {
        return role;
    }

    void tampilkanAkun() {
    	
        cout << "\nNama : " << namaLengkap;
        cout << "\nUsername : " << username;
        cout << "\nRole : " << role;
        cout << "\nNo Telp : " << noTelp << endl;
        
    }
};

// =====================
// CLASS PRODUK
// =====================
class Produk {
	
private:
	
    string kodeBarang;
    string namaBarang;
    string kategori;
    double harga;
    int stok;

public:
	
	
    Produk() {
        kodeBarang = "";
        namaBarang = "";
        kategori = "";
        harga = 0;
        stok = 0;
    }

    Produk(string kode, string nama, string kat,
           double hrg, int stk) {
        kodeBarang = kode;
        namaBarang = nama;
        kategori = kat;
        harga = hrg;
        stok = stk;
    }

    string getKodeBarang() {
        return kodeBarang;
    }

    string getNamaBarang() {
        return namaBarang;
    }

    double getHarga() {
        return harga;
    }

    int getStok() {
        return stok;
    }

    void setStok(int s) {
        stok = s;
    }

    void tampilkanProduk() {
        cout << "\nKode      : " << kodeBarang;
        cout << "\nNama      : " << namaBarang;
        cout << "\nKategori  : " << kategori;
        cout << "\nHarga     : Rp" << harga;
        cout << "\nStok      : " << stok;
        cout << "\n--------------------------";
    }
};

// =====================
// CLASS KERANJANG
// =====================
class Keranjang {
private:
    string namaProduk;
    int jumlah;
    double subtotal;

public:
    Keranjang() {
        namaProduk = "";
        jumlah = 0;
        subtotal = 0;
    }

    Keranjang(string nama, int jml, double sub) {
        namaProduk = nama;
        jumlah = jml;
        subtotal = sub;
    }

    double getSubtotal() {
        return subtotal;
    }

    void tampilkanItem() {
        cout << "\nProduk : " << namaProduk;
        cout << "\nJumlah : " << jumlah;
        cout << "\nSubtotal : Rp" << subtotal;
        cout << "\n--------------------------";
    }
};

// =====================
// CLASS TRANSAKSI
// =====================
class Transaksi {
private:
    string idTransaksi;
    double totalBayar;

public:
    Transaksi() {
        idTransaksi = "";
        totalBayar = 0;
    }

    void setIdTransaksi(string id) {
        idTransaksi = id;
    }

    void setTotalBayar(double total) {
        totalBayar = total;
    }

    double getTotalBayar() {
        return totalBayar;
    }

    void cetakStruk() {
        cout << "\n==========================";
        cout << "\n      STRUK BELANJA";
        cout << "\n==========================";
        cout << "\nID Transaksi : " << idTransaksi;
        cout << "\nTotal Bayar  : Rp" << totalBayar;
        cout << "\n==========================";
    }
};

// =====================
// CLASS DISKON
// =====================
class Diskon {
private:
    string namaPromo;
    double persentase;

public:
    Diskon() {
        namaPromo = "Promo Awal";
        persentase = 10;
    }

    double hitungDiskon(double total) {
        return total * persentase / 100;
    }
};

// =====================
// CLASS LAPORAN
// =====================
class Laporan {
private:
    int totalTransaksi;
    double totalPendapatan;

public:
    Laporan() {
        totalTransaksi = 0;
        totalPendapatan = 0;
    }

    void tambahData(double total) {
        totalTransaksi++;
        totalPendapatan += total;
    }

    void tampilkanLaporan() {
        cout << "\n========== LAPORAN ==========";
        cout << "\nJumlah Transaksi : "
             << totalTransaksi;
        cout << "\nTotal Pendapatan : Rp"
             << totalPendapatan;
        cout << "\n=============================";
    }
};

// =====================
// GLOBAL
// =====================
vector<Produk> daftarProduk;
Laporan laporan;

// =====================
// CLASS ADMIN
// =====================
class Admin {
private:
    string idAdmin;
    string nama;
    string username;
    string password;
    string pinAdmin;

public:
    Admin() {
        idAdmin = "ADM001";
        nama = "Admin";
        username = "admin";
        password = "123";
        pinAdmin = "9999";
    }

    bool loginAdmin(string user, string pass) {
        return (user == username && pass == password);
    }

    void tampilMenuAdmin() {
        cout << "\n============================";
        cout << "\n       MENU ADMIN";
        cout << "\n============================";
        cout << "\n1. Tambah Barang";
        cout << "\n2. Lihat Barang";
        cout << "\n3. Edit Barang";
        cout << "\n4. Hapus Barang";
        cout << "\n5. Laporan";
        cout << "\n6. Logout";
    }
};

// =====================
// CLASS KASIR
// =====================


class Kasir {
	
private:
    string idKasir;
    string nama;
    string username;
    string password;
    string shift;

public:
	
    Kasir() {
        idKasir = "KSR001";
        nama = "Kasir";
        username = "kasir";
        password = "123";
        shift = "Pagi";
    }

    bool loginKasir(string user, string pass) {
        return (user == username && pass == password);
    }

    void tampilMenuKasir() {
        cout << "\n============================";
        cout << "\n       MENU KASIR";
        cout << "\n============================";
        cout << "\n1. Transaksi";
        cout << "\n2. Lihat Barang";
        cout << "\n3. Laporan";
        cout << "\n4. Logout";
    }
};

Admin admin;
Kasir kasir;

// =====================
// TAMBAH BARANG
// =====================
void tambahBarang() {
    string kode, nama, kategori;
    double harga;
    int stok;

    cout << "\nKode Barang : ";
    cin >> kode;

    cin.ignore();

    cout << "Nama Barang : ";
    getline(cin, nama);

    cout << "Kategori : ";
    getline(cin, kategori);

    cout << "Harga : ";
    cin >> harga;

    cout << "Stok : ";
    cin >> stok;

    Produk p(kode, nama, kategori,
             harga, stok);

    daftarProduk.push_back(p);

    cout << "\nBarang berhasil ditambah!\n";
}

// =====================
// LIHAT BARANG
// =====================
void lihatBarang() {

    if(daftarProduk.empty()) {
        cout << "\nBelum ada barang!\n";
        return;
    }

    cout << "\n===== DATA BARANG =====\n";

    for(int i=0;i<daftarProduk.size();i++) {
        daftarProduk[i].tampilkanProduk();
    }
}


void editBarang() {

    if(daftarProduk.empty()) {
        cout << "\nBelum ada barang!\n";
        return;
    }

    lihatBarang();

    int index;

    cout << "\nPilih nomor barang yang ingin diedit : ";
    cin >> index;

    if(index < 0 || index >= daftarProduk.size()) {
        cout << "\nData tidak ditemukan!\n";
        return;
    }

    string nama;
    double harga;
    int stok;


    cin.ignore();

    cout << "Nama baru : ";
    getline(cin, nama);

    cout << "Harga baru : ";
    cin >> harga;

    cout << "Stok baru : ";
    cin >> stok;

    cout << "\nFitur edit akan kita lengkapi pada Tahap 3.";
}

void hapusBarang() {

    if(daftarProduk.empty()) {
        cout << "\nBelum ada barang!\n";
        return;
    }

    lihatBarang();

    int index;

    cout << "\nPilih nomor barang yang dihapus : ";
    cin >> index;

    if(index < 0 || index >= daftarProduk.size()) {
        cout << "\nData tidak ditemukan!\n";
        return;
    }

    daftarProduk.erase(daftarProduk.begin()+index);

    cout << "\nBarang berhasil dihapus!\n";
}
// =====================
// TRANSAKSI
// =====================
void transaksiBaru() {

    if(daftarProduk.empty()) {
        cout << "\nBelum ada produk!\n";
        return;
    }

    lihatBarang();

    int index;
    int jumlah;

    cout << "\n\nPilih nomor produk (0 dst): ";
    cin >> index;

    if(index < 0 || index >= daftarProduk.size()) {
        cout << "\nProduk tidak ditemukan!\n";
        return;
    }

    cout << "Jumlah beli : ";
    cin >> jumlah;

    double subtotal =
        daftarProduk[index].getHarga() * jumlah;

    Diskon diskon;

    double potongan =
        diskon.hitungDiskon(subtotal);

    double total =
        subtotal - potongan;

    Transaksi trx;

    trx.setIdTransaksi("TRX001");
    trx.setTotalBayar(total);

    laporan.tambahData(total);

    trx.cetakStruk();

    cout << "\nDiskon : Rp" << potongan;
    cout << "\nTotal Akhir : Rp" << total;
}

// =====================
// LOGIN
// =====================

bool login() {
	

    string user, pass;
    

    cout << "\n==============================";
    cout << "\n      LOGIN SISTEM";
    cout << "\n==============================";

    cout << "\nUsername : ";
    cin >> user;

    cout << "Password : ";
    cin >> pass;

    if(admin.loginAdmin(user, pass)) {
        cout << "\nLogin sebagai ADMIN berhasil!\n";
        return true;
    }

    if(kasir.loginKasir(user, pass)) {
        cout << "\nLogin sebagai KASIR berhasil!\n";
        return true;
    }

    cout << "\nUsername atau Password salah!\n";
    return false;
}


// =====================
// MENU
// =====================


void menuUtama() {

    int pilih;

    do {


        cout << "\n========== MENU UTAMA ==========\n";
        cout << "1. Tambah Barang\n";
        cout << "2. Lihat Barang\n";
        cout << "3. Edit Barang\n";
        cout << "4. Hapus Barang\n";
        cout << "5. Transaksi\n";
        cout << "6. Laporan\n";
        
        cout << "Pilih menu : ";
        cin >> pilih;
        
        switch(pilih) {

        case 1:
    tambahBarang();
    break;

        case 2:
    lihatBarang();
    break;

        case 3:
    editBarang();
    break;

        case 4:
    hapusBarang();
    break;

        case 5:
    transaksiBaru();
    break;

        case 6:
    laporan.tampilkanLaporan();
    break;

        case 7:
    cout << "\nLogout berhasil...\n";
    break;

        default:
            cout << "\nMenu tidak tersedia!";
        }

    } while(pilih != 7);
    
}

// =====================
// MAIN
// =====================
int main() {

    if(login()) {
    	
        menuUtama();
       
    }
    

    return 0;
    
}
