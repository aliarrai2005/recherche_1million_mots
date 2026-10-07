#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>   // std::find
#include <chrono>      // mesure du temps
#ifdef _WIN32
  #include <windows.h>
  #include <psapi.h>   // GetProcessMemoryInfo (ajouter -lpsapi a la compilation)
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

// ---------- Test de sous-sequence avec find ----------
bool contientDansLOrdre(const string& mot1, const string& mot2) {
    auto it = mot1.begin();
    for (char c : mot2) {
        it = find(it, mot1.end(), c);
        if (it == mot1.end()) return false;
        ++it;
    }
    return true;
}

// ---------- Affichage des resultats ----------
void afficherResultats(const vector<string>& resultats) {
    cout << resultats.size() << " mot(s) trouve(s)." << endl;
    size_t limite = min<size_t>(resultats.size(), 10);
    for (size_t i = 0; i < limite; i++) cout << " - " << resultats[i] << endl;
    if (resultats.size() > limite)
        cout << " ... (" << resultats.size() - limite << " autres)" << endl;
}

int main() {
    int mode;
    cout << "Mode 1 = charger UNE fois dans un vector, puis chercher (sans relire le fichier)\n"
         << "Mode 2 = relire le fichier a chaque recherche (sans stocker les mots)\n"
         << "Votre choix : ";
    cin >> mode;

    vector<string> mots;
    double memAvant = memoireActuelleMo();
    double memApresLecture = memAvant;

    // ---------- MODE 1 : chargement unique ----------
    if (mode == 1) {
        ifstream fichier("mots.txt");
        if (!fichier.is_open()) {
            cerr << "Erreur : impossible d'ouvrir mots.txt" << endl;
            return 1;
        }
        mots.reserve(1000000);
        string mot;

        auto t0 = chrono::steady_clock::now();
        while (fichier >> mot) mots.push_back(mot);
        auto t1 = chrono::steady_clock::now();
        fichier.close();

        double tLecture = chrono::duration<double, milli>(t1 - t0).count();
        memApresLecture = memoireActuelleMo();

        cout << mots.size() << " mots charges en " << tLecture << " ms (une seule fois)." << endl;
        cout << "Memoire : " << memAvant << " Mo -> " << memApresLecture << " Mo" << endl;
    }

    // ---------- Boucle de recherches ----------
    string recherche;
    while (true) {
        cout << "\nLettres a chercher (0 pour quitter) : ";
        cin >> recherche;
        if (recherche == "0") break;

        vector<string> resultats;
        size_t nbLus = 0;

        if (mode == 1) {
            // Recherche en RAM, le fichier n'est PAS relu
            auto t2 = chrono::steady_clock::now();
            for (const string& m : mots) {
                if (contientDansLOrdre(m, recherche)) resultats.push_back(m);
            }
            auto t3 = chrono::steady_clock::now();
            nbLus = mots.size();

            afficherResultats(resultats);
            cout << "Mots parcourus     : " << nbLus << endl;
            cout << "Temps de recherche : "
                 << chrono::duration<double, milli>(t3 - t2).count() << " ms" << endl;
        } else {
            // Lecture + recherche en meme temps : le fichier est relu a chaque fois
            ifstream fichier("mots.txt");
            if (!fichier.is_open()) {
                cerr << "Erreur : impossible d'ouvrir mots.txt" << endl;
                return 1;
            }
            string mot;
            auto t2 = chrono::steady_clock::now();
            while (fichier >> mot) {
                nbLus++;
                if (contientDansLOrdre(mot, recherche)) resultats.push_back(mot);
            }
            auto t3 = chrono::steady_clock::now();
            fichier.close();

            afficherResultats(resultats);
            cout << "Mots lus           : " << nbLus << endl;
            cout << "Temps (lecture + recherche) : "
                 << chrono::duration<double, milli>(t3 - t2).count() << " ms" << endl;
        }
    }

    // ---------- Bilan memoire ----------
    cout << "\n===== MEMOIRE =====" << endl;
    cout << "sizeof(string)   : " << sizeof(string) << " octets" << endl;
    cout << "Avant lecture    : " << memAvant << " Mo" << endl;
    if (mode == 1) {
        cout << "Apres chargement : " << memApresLecture << " Mo" << endl;
        cout << "vector mots      : " << mots.capacity() * sizeof(string) / (1024.0 * 1024.0)
             << " Mo (capacite " << mots.capacity() << " elements)" << endl;
    }
    cout << "Fin du programme : " << memoireActuelleMo() << " Mo" << endl;
    cout << "Pic de memoire   : " << memoirePicMo() << " Mo" << endl;
    return 0;
}
