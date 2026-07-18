#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <curl/curl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace std;

// Sabitler
const string DOSYA_ADI = "hesap.json";
const string OLLAMA_URL = "http://localhost:11434/api/chat";
const string MODEL = "llama3.2:3b";

// ─── Veri yapısı ─────────────────────────────────────────────────────────────
struct Veri {
  int gelir;
  int gider;
  string ay;
};

vector<Veri> depo;

// Ay takvim sırası
const map<string, int> AY_SIRA = {
    {"ocak", 1},      {"january", 1},   {"jan", 1},
    {"subat", 2},     {"şubat", 2},     {"february", 2},  {"feb", 2},
    {"mart", 3},      {"march", 3},     {"mar", 3},
    {"nisan", 4},     {"april", 4},     {"apr", 4},
    {"mayis", 5},     {"mayıs", 5},     {"may", 5},
    {"haziran", 6},   {"june", 6},      {"jun", 6},
    {"temmuz", 7},    {"july", 7},      {"jul", 7},
    {"agustos", 8},   {"ağustos", 8},   {"august", 8},    {"aug", 8},
    {"eylul", 9},     {"eylül", 9},     {"september", 9}, {"sep", 9},
    {"ekim", 10},     {"october", 10},  {"oct", 10},
    {"kasim", 11},    {"kasım", 11},    {"november", 11}, {"nov", 11},
    {"aralik", 12},   {"aralık", 12},   {"december", 12}, {"dec", 12}};

// ─── Yardımcı string işlemleri ───────────────────────────────────────────────
string kucult(string s) {
  struct Replacement { string target; string replacement; };
  static const vector<Replacement> reps = {
    {"İ", "i"}, {"I", "ı"}, {"Ş", "ş"}, {"Ç", "ç"}, {"Ğ", "ğ"}, {"Ü", "ü"}, {"Ö", "ö"}
  };
  for (const auto& r : reps) {
    size_t pos = 0;
    while ((pos = s.find(r.target, pos)) != string::npos) {
      s.replace(pos, r.target.length(), r.replacement);
      pos += r.replacement.length();
    }
  }
  for (char &c : s) {
    c = tolower((unsigned char)c);
  }
  return s;
}

string trim(string s) {
  s.erase(0, s.find_first_not_of(" \t\r\n"));
  if (!s.empty())
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
  return s;
}

string ilk_buyuk(string s) {
  s = trim(s);
  if (!s.empty())
    s[0] = toupper((unsigned char)s[0]);
  return s;
}

bool icerir(const string &metin, const string &aranan) {
  return metin.find(aranan) != string::npos;
}

int ay_sirasi(const string &ay_tam) {
  stringstream ss(ay_tam);
  string ilk;
  ss >> ilk;
  auto it = AY_SIRA.find(kucult(ilk));
  return (it != AY_SIRA.end()) ? it->second : 99;
}

string yil_cikart(const string &ay_tam) {
  regex r(R"(\d{4})");
  smatch m;
  if (regex_search(ay_tam, m, r))
    return m[0];
  return "Bilinmiyor";
}

// ─── JSON dosya işlemleri ────────────────────────────────────────────────────
void depo_yukle() {
  ifstream f(DOSYA_ADI);
  if (!f.is_open())
    return;
  try {
    json j;
    f >> j;
    for (auto &item : j) {
      string ay = item.contains("month") ? item.value("month", "") : item.value("ay", "");
      if (ay.empty())
        continue;
      Veri v;
      v.gelir = item.contains("income") ? item.value("income", 0) : item.value("gelir", 0);
      v.gider = item.contains("expense") ? item.value("expense", 0) : (item.contains("expenses") ? item.value("expenses", 0) : item.value("gider", 0));
      v.ay = ay;
      depo.push_back(v);
    }
  } catch (...) {
  }
}

void depo_kaydet() {
  json j = json::array();
  for (auto &v : depo)
    j.push_back({{"income", v.gelir}, {"expense", v.gider}, {"month", v.ay}});
  ofstream f(DOSYA_ADI);
  f << j.dump(4);
}

// ─── Kullanıcı girişi ────────────────────────────────────────────────────────
int sayi_al(const string &mesaj) {
  while (true) {
    cout << mesaj;
    string girdi;
    getline(cin, girdi);
    girdi = trim(girdi);
    if (girdi.empty()) {
      cout << "Bu alan bos birakilamaz!\n";
      continue;
    }
    try {
      int s = stoi(girdi);
      if (s < 0)
        cout << "Negatif sayi girilemez!\n";
      else
        return s;
    } catch (...) {
      cout << "Lutfen gecerli bir tam sayi girin!\n";
    }
  }
}

string yazi_al(const string &mesaj) {
  while (true) {
    cout << mesaj;
    string girdi;
    getline(cin, girdi);
    girdi = trim(girdi);
    if (girdi.empty())
      cout << "Bu alan bos birakilamaz!\n";
    else
      return girdi;
  }
}

// ─── Veri ekleme ─────────────────────────────────────────────────────────────
void veri_ekle() {
  cout << "\nYeni veri girisi:\n\n";

  // Yil dogrulamasi
  string yil_adi;
  while (true) {
    yil_adi = yazi_al("Hangi yil? (orn: 2026): ");
    if (regex_match(yil_adi, regex(R"(20\d{2})")))
      break;
    cout << "Lutfen gecerli bir yil girin (orn: 2026)!\n";
  }

  string ay_adi = yazi_al("Hangi ay? (orn: Mayis): ");
  string ay_tam = ilk_buyuk(ay_adi) + " " + yil_adi;

  // Ayni ay kayitli mi?
  for (int i = 0; i < (int)depo.size(); i++) {
    if (kucult(depo[i].ay) == kucult(ay_tam)) {
      cout << "'" << ay_tam
           << "' ayi zaten kayitli! Uzerine yazmak ister misiniz? "
              "(evet/hayir): ";
      string cevap;
      getline(cin, cevap);
      cevap = kucult(trim(cevap));
      if (cevap != "evet" && cevap != "e") {
        cout << "Iptal edildi.\n\n";
        return;
      }
      depo.erase(depo.begin() + i);
      break;
    }
  }

  Veri yeni;
  yeni.gelir = sayi_al("Toplam gelir (TL): ");
  yeni.gider = sayi_al("Toplam gider (TL): ");
  yeni.ay = ay_tam;

  depo.push_back(yeni);
  depo_kaydet();

  int net = yeni.gelir - yeni.gider;
  cout << "\n'" << ay_tam << "' verisi kaydedildi!\n";
  cout << "   Gelir: " << yeni.gelir << " TL | Gider: " << yeni.gider
       << " TL | Net Kar: " << net << " TL\n\n";
}

// ─── Geçmiş göster ───────────────────────────────────────────────────────────
void gecmis_goster(const string &ay) {
  for (auto &v : depo) {
    if (kucult(v.ay) == kucult(ay)) {
      cout << "\n" << v.ay << " verileri:\n";
      cout << "  Gelir  : " << v.gelir << " TL\n";
      cout << "  Gider  : " << v.gider << " TL\n";
      cout << "  Net kar: " << (v.gelir - v.gider) << " TL\n\n";
      return;
    }
  }
  cout << "'" << ay << "' ayina ait veri bulunamadi!\n";
}

// Sayiyi Turkce bicimde binlik ayiraciyla yazar (20000 -> "20.000").
string bin_ayir(long long n) {
  bool eksi = n < 0;
  string s = to_string(eksi ? -n : n);
  string sonuc;
  int sayac = 0;
  for (int i = (int)s.size() - 1; i >= 0; i--) {
    sonuc = s[i] + sonuc;
    if (++sayac % 3 == 0 && i != 0)
      sonuc = "." + sonuc;
  }
  return (eksi ? "-" : "") + sonuc;
}

// ─── Grafik (gnuplot) ────────────────────────────────────────────────────────
// Yil ve ay kullaniciya sorulur; sadece o tek kayit icin Gelir/Gider/Net Kar
// degerleri karsilastirilir. Uc deger her zaman ayni "yuksek-dusuk-yuksek"
// iliskisine sahip oldugundan (Net Kar = Gelir - Gider) cizgi grafiginin
// genel sekli her ay icin neredeyse ayni goruntuyu veriyordu; bu yuzden
// cubuklarin uzerine gercek TL degerini de yazarak aylari sekilden degil
// dogrudan rakamdan ayirt edilebilir hale getiriyoruz.
void grafik_ciz_ay() {
  if (depo.empty()) {
    cout << "Henuz hic veri yok!\n";
    return;
  }

  // gnuplot yuklu mu?
  if (system("which gnuplot > /dev/null 2>&1") != 0) {
    cout << "gnuplot bulunamadi! Yuklemek icin: brew install gnuplot\n";
    return;
  }

  string yil_adi;
  while (true) {
    yil_adi = yazi_al("Hangi yil? (orn: 2026): ");
    if (regex_match(yil_adi, regex(R"(20\d{2})")))
      break;
    cout << "Lutfen gecerli bir yil girin (orn: 2026)!\n";
  }
  string ay_adi = yazi_al("Hangi ay? (orn: Mayis): ");
  string ay_tam = ilk_buyuk(ay_adi) + " " + yil_adi;

  const Veri *bulunan = nullptr;
  for (auto &v : depo)
    if (kucult(v.ay) == kucult(ay_tam)) {
      bulunan = &v;
      break;
    }
  if (!bulunan) {
    cout << "'" << ay_tam << "' ayina ait veri bulunamadi!\n";
    return;
  }

  long long net = (long long)bulunan->gelir - bulunan->gider;

  // ── Veri dosyasi yaz ─────────────────────────────────────────────────────
  // 4. sutun noktanin uzerine yazilacak bicimlendirilmis TL degeri.
  string dat = "/tmp/muhasebe_ay.dat";
  ofstream df(dat);
  df << "0\t" << bulunan->gelir << "\t\"Gelir\"\t\"" << bin_ayir(bulunan->gelir)
     << " TL\"\n";
  df << "1\t" << bulunan->gider << "\t\"Gider\"\t\"" << bin_ayir(bulunan->gider)
     << " TL\"\n";
  df << "2\t" << net << "\t\"Net Kar\"\t\"" << bin_ayir(net) << " TL\"\n";
  df.close();

  long long ymax = max({(long long)bulunan->gelir, (long long)bulunan->gider, net, 0LL});
  long long ymin = min(0LL, net);
  long long pad = max(1LL, (ymax - ymin) / 8);

  // ── Gnuplot script yaz ──────────────────────────────────────────────────
  // Canli (persist) bir gnuplot penceresini arka planda tutmak macOS'ta
  // pencere aktivasyonu/SIGTTOU sorunlarina yol aciyordu: program calisirken
  // pencere gorunmuyor, program kapanana kadar bekliyordu. Bunun yerine
  // gnuplot'u dogrudan (arka planda degil, senkron) bir PNG dosyasina
  // render edip, dosyayi macOS'un kendi 'open' komutuyla aciyoruz. 'open'
  // Launch Services uzerinden calistigi icin pencere her zaman dogru
  // odaklanarak on plana geliyor.
  string png = "/tmp/muhasebe_ay.png";
  string script = "/tmp/muhasebe.gp";
  ofstream gp(script);

  gp << "set encoding utf8\n";
  gp << "set terminal pngcairo size 800,500 enhanced font 'Arial,11'\n";
  gp << "set output '" << png << "'\n";
  gp << "set title '" << ay_tam << " Ozeti' font 'Arial,14'\n";
  gp << "set style data linespoints\n";
  gp << "set ylabel 'TL'\n";
  gp << "set xrange [-0.5:2.5]\n";
  gp << "set yrange [" << (ymin - pad) << ":" << (ymax + pad) << "]\n";
  gp << "set grid lc rgb '#aaaaaa' lt 0\n";
  gp << "set xtics font 'Arial,11'\n";
  gp << "unset key\n";
  gp << "plot '" << dat
     << "' using 1:2:xtic(3) with linespoints lw 2 lc rgb '#3498db' pt 7 ps 2, \\\n";
  gp << "     '' using 1:2:4 with labels offset 0,1 font 'Arial,11' notitle\n";
  gp.close();

  cout << "Grafik olusturuluyor...\n";
  if (system(("gnuplot " + script).c_str()) != 0) {
    cout << "Grafik olusturulamadi!\n";
    return;
  }
  system(("open '" + png + "'").c_str());
  cout << "Grafik acildi.\n";
}

// ─── Ollama (streaming) ──────────────────────────────────────────────────────
size_t ollama_yaz(char *ptr, size_t size, size_t n, void *) {
  string chunk(ptr, size * n);
  istringstream ss(chunk);
  string satir;
  while (getline(ss, satir)) {
    satir = trim(satir);
    if (satir.empty())
      continue;
    try {
      json j = json::parse(satir);
      if (j.contains("message") && j["message"].contains("content")) {
        cout << j["message"]["content"].get<string>() << flush;
      }
    } catch (...) {
    }
  }
  return size * n;
}

string sistem_talimati() {
  Veri son = depo.empty() ? Veri{0, 0, "-"} : depo.back();
  json j = json::array();
  for (auto &v : depo)
    j.push_back({{"income", v.gelir}, {"expense", v.gider}, {"month", v.ay}});

  return "Sen bir Turk muhasebe asistanisin. En son girilen ay: " + son.ay +
         ".\n"
         "- Toplam gelir: " +
         to_string(son.gelir) +
         " TL\n"
         "- Toplam gider: " +
         to_string(son.gider) +
         " TL\n"
         "- Net kar: " +
         to_string(son.gelir - son.gider) +
         " TL\n"
         "- Tum aylara ait gecmis veriler: " +
         j.dump() +
         "\n\n"
         "Kurallar:\n"
         "1. Sadece bu verilere dayanarak cevap ver, uydurma.\n"
         "2. Turkce ve kisa konus.\n"
         "3. Hesaplama gerekiyorsa acikla.\n"
         "4. Veride olmayan bir sey sorulursa 'Bu bilgi elimde yok' de.\n"
         "5. Gecen aylardaki verilerle karsilastirma yapabilirsin.\n"
         "6. Kullanici gecen/giden ay derse depodaki verileri kullan.\n";
}

void ai_yanit_ver(const string &soru) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    cout << "CURL baslatilamadi!\n";
    return;
  }

  json body = {
      {"model", MODEL},
      {"stream", true},
      {"messages",
       json::array({{{"role", "system"}, {"content", sistem_talimati()}},
                    {{"role", "user"}, {"content", soru}}})}};
  string body_str = body.dump();

  curl_slist *hdrs = nullptr;
  hdrs = curl_slist_append(hdrs, "Content-Type: application/json");

  cout << "Asistan: " << flush;
  curl_easy_setopt(curl, CURLOPT_URL, OLLAMA_URL.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ollama_yaz);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);

  CURLcode res = curl_easy_perform(curl);
  cout << "\n";
  if (res != CURLE_OK)
    cout << "Ollama baglanti hatasi: " << curl_easy_strerror(res) << "\n";

  curl_slist_free_all(hdrs);
  curl_easy_cleanup(curl);
}

// ─── Ay/Yıl çıkarma ──────────────────────────────────────────────────────────
pair<string, string> sorudan_ay_yil(const string &metin) {
  static const vector<string> AYLAR = {
      "ocak",   "subat",   "şubat",   "mart",    "nisan",   "mayis",
      "mayıs",  "haziran", "temmuz",  "agustos", "ağustos", "eylul",
      "eylül",  "ekim",    "kasim",   "kasım",   "aralik",  "aralık",
      "january", "february", "march", "april", "may", "june",
      "july", "august", "september", "october", "november", "december"};
  string mk = kucult(metin);
  string bay, byil;

  for (auto &a : AYLAR) {
    if (icerir(mk, a)) {
      for (auto &v : depo) {
        if (icerir(kucult(v.ay), a)) {
          istringstream ss(v.ay);
          ss >> bay;
          break;
        }
      }
      if (bay.empty())
        bay = ilk_buyuk(a);
      break;
    }
  }

  smatch m;
  if (regex_search(metin, m, regex(R"(\b(20\d{2})\b)")))
    byil = m[1];

  return {bay, byil};
}

// ─── Ana program ─────────────────────────────────────────────────────────────
int main() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  depo_yukle();

  if (!depo.empty())
    cout << "\nMuhasebe AI Asistani (" << depo.size() << " kayit yuklendi)\n";
  else
    cout << "\nMuhasebe AI Asistani hazir! (Henuz kayit yok -- 'veri ekle' "
            "yazarak baslayabilirsin)\n";
  cout << "Ipuclari: veri ekle - grafik - gecmis veriler - q cikis\n\n";

  const vector<string> EKLE_KW = {"veri ekle", "yeni ay",    "yeni veri",
                                  "ekle",      "kayit ekle", "veri gir"};
  const vector<string> GECMIS_KW = {"gecmis", "gecen", "giden", "onceki",
                                    "kayit"};

  while (true) {
    cout << "Sen: ";
    string soru;
    getline(cin, soru);
    soru = trim(soru);

    if (soru.empty()) {
      cout << "Bos mesaj gonderilemez!\n";
      continue;
    }
    if (kucult(soru) == "q") {
      cout << "Gorusmek uzere!\n";
      break;
    }

    string sk = kucult(soru);

    // ── Veri ekle ─────────────────────────────────────────────────────────
    bool ekle = false;
    for (auto &k : EKLE_KW)
      if (icerir(sk, k)) {
        ekle = true;
        break;
      }
    if (ekle) {
      veri_ekle();
      continue;
    }

    // ── Grafik ───────────────────────────────────────────────────────────
    if (icerir(sk, "grafik")) {
      grafik_ciz_ay();
      continue;
    }

    // ── Geçmiş veri ──────────────────────────────────────────────────────
    bool gecmis = false;
    for (auto &k : GECMIS_KW)
      if (icerir(sk, k)) {
        gecmis = true;
        break;
      }
    if (gecmis) {
      auto [bay, byil] = sorudan_ay_yil(soru);
      if (!bay.empty() && !byil.empty()) {
        gecmis_goster(bay + " " + byil);
      } else if (!byil.empty()) {
        vector<Veri> eslesmeler;
        for (auto &v : depo)
          if (icerir(v.ay, byil))
            eslesmeler.push_back(v);
        if (!eslesmeler.empty()) {
          cout << "\n" << byil << " yilina ait kayitlar:\n";
          for (auto &v : eslesmeler)
            cout << "  " << v.ay << " -- Gelir: " << v.gelir
                 << " TL | Gider: " << v.gider
                 << " TL | Net: " << (v.gelir - v.gider) << " TL\n";
          cout << "\n";
        } else {
          cout << byil << " yilina ait veri bulunamadi.\n";
        }
      } else if (!bay.empty()) {
        string eslesme;
        for (auto &v : depo)
          if (icerir(kucult(v.ay), kucult(bay))) {
            eslesme = v.ay;
            break;
          }
        if (!eslesme.empty())
          gecmis_goster(eslesme);
        else
          cout << "'" << bay << "' ayina ait veri bulunamadi.\n";
      } else {
        if (!depo.empty()) {
          cout << "\nTum kayitlar:\n";
          for (auto &v : depo)
            cout << v.ay << " -- Gelir: " << v.gelir
                 << " TL | Gider: " << v.gider
                 << " TL | Net: " << (v.gelir - v.gider) << " TL\n";
          cout << "\n";
        } else {
          cout << "Henuz hic kayit yok.\n";
        }
      }
      continue;
    }

    // ── AI ───────────────────────────────────────────────────────────────
    ai_yanit_ver(soru);
  }

  curl_global_cleanup();
  return 0;
}