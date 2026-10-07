#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>   // std::find
#include <chrono>      // mesure du temps
#ifdef _WIN32
  #include <windows.h>
  #include <psapi.h>   // GetProcessMemoryInfo (ajouter -lpsapi a l'edition de liens)
#else
  #include <sys/resource.h>
  #include <unistd.h>
#endif
using namespace std;

// ---------- Mesure de la memoire (en Mo) ----------
double memoireActuelleMo() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize / (1024.0 * 1024.0);
#else
    long pages = 0, residentes = 0;
    ifstream f("/proc/self/statm");
    f >> pages >> residentes;
    return residentes * static_cast<double>(sysconf(_SC_PAGESIZE)) / (1024.0 * 1024.0);
#endif
}

double memoirePicMo() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
#else
    struct rusage u;
    getrusage(RUSAGE_SELF, &u);
    return u.ru_maxrss / 1024.0;
#endif
}


bool contientDansLOrdre(const string& mot1, const string& mot2) {
    auto it = mot1.begin();
    for (char c : mot2) {
        it = find(it, mot1.end(), c);
        if (it == mot1.end()) return false;
        ++it;
    }
    return true;
}

int main() {
    string recherche;
    int mode;
    cout << "Lettres a chercher (dans l'ordre) : ";
    cin >> recherche;
    cout << "Mode 1 = tout charger dans un vector puis chercher\n"
         << "Mode 2 = chercher pendant la lecture (sans stocker tous les mots)\n"
         << "Votre choix : ";
    cin >> mode;

    ifstream fichier("mots.txt");
    if (!fichier.is_open()) {
        cerr << "Erreur : impossible d'ouvrir mots.txt" << endl;
        return 1;
    }

    vector<string> mots;
    vector<string> resultats;
    string mot;
    size_t nbLus = 0;
    double tLecture = 0, tRecherche = 0, tTotal = 0;

    double memAvant = memoireActuelleMo();
    double memApresLecture = 0;

    if (mode == 1) {
        // ---- Lecture seule ----
        auto t0 = chrono::steady_clock::now();
        while (fichier >> mot) {
            mots.push_back(mot);
        }
        auto t1 = chrono::steady_clock::now();
        nbLus = mots.size();
        memApresLecture = memoireActuelleMo();

        // ---- Recherche seule ----
        auto t2 = chrono::steady_clock::now();
        for (const string& m : mots) {
            if (contientDansLOrdre(m, recherche)) resultats.push_back(m);
        }
        auto t3 = chrono::steady_clock::now();

        tLecture   = chrono::duration<double, milli>(t1 - t0).count();
        tRecherche = chrono::duration<double, milli>(t3 - t2).count();
        tTotal     = tLecture + tRecherche;
    } else {
        // ---- Lecture + recherche en meme temps ----
        auto t0 = chrono::steady_clock::now();
        while (fichier >> mot) {
            nbLus++;
            if (contientDansLOrdre(mot, recherche)) resultats.push_back(mot);
        }
        auto t1 = chrono::steady_clock::now();
        tTotal = chrono::duration<double, milli>(t1 - t0).count();
        memApresLecture = memoireActuelleMo();
    }
    fichier.close();

    double memFin = memoireActuelleMo();
    double memPic = memoirePicMo();

    // ---- Affichage ----
    cout << "\n===== RESULTATS =====\n";
    cout << "Mots lus          : " << nbLus << endl;
    cout << "Mots trouves      : " << resultats.size() << endl;
    size_t limite = min<size_t>(resultats.size(), 10);
    for (size_t i = 0; i < limite; i++) cout << " - " << resultats[i] << endl;
    if (resultats.size() > limite) cout << " ... (" << resultats.size() - limite << " autres)" << endl;

    cout << "\n===== TEMPS =====\n";
    if (mode == 1) {
        cout << "Lecture du fichier : " << tLecture   << " ms" << endl;
        cout << "Recherche (find)   : " << tRecherche << " ms" << endl;
    }
    cout << "Temps total        : " << tTotal << " ms" << endl;

    cout << "\n===== MEMOIRE =====\n";
    cout << "sizeof(string)     : " << sizeof(string) << " octets" << endl;
    cout << "Avant lecture      : " << memAvant << " Mo" << endl;
    cout << "Apres lecture      : " << memApresLecture << " Mo" << endl;
    cout << "Fin du programme   : " << memFin << " Mo" << endl;
    cout << "Pic de memoire     : " << memPic << " Mo" << endl;
    if (mode == 1) {
        cout << "vector mots        : " << mots.capacity() * sizeof(string) / (1024.0 * 1024.0)
             << " Mo (capacite " << mots.capacity() << " elements)" << endl;
    }
    return 0;
}
