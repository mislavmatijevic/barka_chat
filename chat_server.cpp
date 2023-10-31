#include "chat_header.h"

// Semafor koji kontrolira metode za novog klijenta, brisanje klijenta, iteriranje po listi itd.
pthread_mutex_t monitor_kriticni;
// Semafor koji kontrolira da se ove dvije funkcije dolje za ispis ne prepliću.
pthread_mutex_t monitor_ispis;

const char *boja_normal = "\033[0m";
const char *boja_normal_bold = "\033[1;37m";
const char *boja_crvena = "\033[1;31m";
const char *boja_zelena = "\033[1;92m";
const char *boja_zuta = "\033[1;93m";
const char *boja_plava = "\033[1;96m";

int posalji_klijentu (poruka_paket odlazni_paket)
{
    int poslano_bajtova = posalji_paket(odlazni_paket.za_koga, &odlazni_paket);
    if (poslano_bajtova == -1 || poslano_bajtova == 0) return -1;

    const char* boja_info;    
    pthread_mutex_lock(&monitor_ispis);
    cout << boja_plava << "\n>>>\tPOSLAO SAM " << poslano_bajtova << "B\n\tOd: "
         << boja_normal_bold << odlazni_paket.od_koga << " ";
         switch (odlazni_paket.od_koga)
         {
             case SERVER_spojen_si:
             {
                boja_info = boja_zelena;
                cout << boja_info << "Javljam klijentu da je spojen.";
                break;
             }
             case SERVER_gasim_se:
             {
                boja_info = boja_zuta;
                cout << boja_info << "Javljam klijentu da je odspojen.";
                break;
             }
             case SERVER_sugovornici:
             {
                boja_info = boja_normal_bold;
                cout << boja_info << "Šaljem klijentu popis aktivnih sudionika.";
                break;
             }
             case SERVER_sugovornik_nedostupan:
             {
                boja_info = boja_crvena;
                cout << boja_info << "Javljam klijentu da šalje poruku nedostupnom sugovorniku.";
                break;
             }
             case SERVER_oznaka:
             {
                 
                boja_info = boja_zuta;
                cout << boja_info << "Ovo je serverska poruka klijentu.";
                break;
             }
             default:
             {
                boja_info = boja_zelena;
                cout << boja_info << "Obična poruka klijenta drugome klijentu.";
             }
         }
    cout << boja_plava << "\n\tPrema: " 
         << boja_info << odlazni_paket.za_koga
         << boja_plava << "\n\tPoruka: "
         << boja_info << odlazni_paket.poruka << endl;

    pthread_mutex_unlock(&monitor_ispis);
    return poslano_bajtova;
}


int server_primi_paket(int arg_opisnik, poruka_paket *paket_za_pohranu)
{
	poruka_paket_slanje_info paket_s_info;	
	int primljeno_bajtova_1 = recvfrom(arg_opisnik, &paket_s_info, sizeof(poruka_paket_slanje_info), 0, (sockaddr *)&adresa_klijent, &adresa_klijent_velicina);
    if (!primljeno_bajtova_1 || primljeno_bajtova_1 == -1) return -1;

	char *primljena_poruka = new char [paket_s_info.velicina_poruke];
	memset(primljena_poruka, 0, paket_s_info.velicina_poruke); // Da se izbjegne razno smeće.
    
	int primljeno_bajtova_2 = recvfrom(arg_opisnik, primljena_poruka, paket_s_info.velicina_poruke, 0, (sockaddr *)&adresa_klijent, &adresa_klijent_velicina);
	if (!primljeno_bajtova_2 || primljeno_bajtova_2 == -1) return -1;

	paket_za_pohranu->od_koga = paket_s_info.od_koga;
	paket_za_pohranu->za_koga = paket_s_info.za_koga;
	paket_za_pohranu->poruka = primljena_poruka;
	
	return primljeno_bajtova_1 + primljeno_bajtova_2; // Točno vidimo koliko se bajta poslalo ako je sve prošlo u redu.
}

bool primi_od(int arg_opisnik, poruka_paket *procitani_paket)
{
    int rezultat = server_primi_paket(arg_opisnik, procitani_paket);
    if (rezultat == -1) return false;

    const char* boja_info;

    pthread_mutex_lock(&monitor_ispis);
    cout << boja_zelena << "\n<<<\tPRIMAM " << rezultat << "B\n\tOd: "
         << boja_normal_bold << procitani_paket->od_koga << " (" << arg_opisnik << ") ";
         switch (procitani_paket->od_koga)
         {
             case KLIJENT_spajam_se:
             {
                boja_info = boja_plava;
                cout << boja_info << "Ja sam novi klijent, moje ime je u poruci.";
                break;
             }
             case KLIJENT_odlazim:
             {
                boja_info = boja_plava;
                cout << boja_info << "Odo ja, so long.";
                break;
             }
             default: boja_info = boja_zuta;
         }
    cout << boja_zelena << "\n\tPrema: " 
         << boja_info << procitani_paket->za_koga
         << boja_zelena << "\n\tPoruka: "
         << boja_info << procitani_paket->poruka << endl;
         

    pthread_mutex_unlock(&monitor_ispis);
    return true;
}

class klijent {

    private:
    klijent* bivsi;
    klijent* sljedeci;

    int moj_opisnik;
    string ime;
    pthread_t moja_dretva;

    static int broj_klijenata;

    public:
    klijent(int opisnik_arg, pthread_t pozivna_dretva, string primljeno)
    {
        moj_opisnik = opisnik_arg;
        moja_dretva = pozivna_dretva;
        ime = primljeno;
        cout << boja_zelena << "Konstruktor klijenta: " << moj_opisnik << "-" << ime << endl;
        sljedeci = NULL;
        broj_klijenata++;
    }
    ~klijent()
    {
        // Ništa ne radi pri dealokaciji statično alociranog zaglavlja liste klasa.
        if (moj_opisnik != -100)
        {
            cout << boja_normal_bold << "Dekonstruktor klijenta: "
                 << ime << ", id = " << moj_opisnik << endl;
            cout << boja_normal_bold << "Dretva od " << ime
                << " --> kill kod: " << pthread_kill(moja_dretva, 0) << endl;
            close(moj_opisnik);
            cout << boja_zuta << ime << boja_zelena << " - opisnik zatvoren.\n";
            broj_klijenata--;
        }
    }
    void dodaj_klijenta_obavijesti_druge_i_njega(klijent *novi)
    {
        pthread_mutex_lock(&monitor_kriticni);
        klijent* pom = this->sljedeci;
        klijent* zadnji_u_listi = this;
        poruka_paket paket_s_info;
        paket_s_info.od_koga = SERVER_oznaka;
        paket_s_info.poruka =
        "Server: Novi klijent: " + novi->ime + ", id = " + to_string(novi->moj_opisnik);
        
        // Ovaj string je čisto serverska stvar za debugganje ove proklete liste.
        string ispis = "Tražim mjesto za novoga: zaglavlje -> ";
        while (pom) // Nađi zadnji element i usput sve obavijesti o novome klijentu!
        {
            paket_s_info.za_koga = pom->moj_opisnik;
            posalji_klijentu(paket_s_info);
            ispis += pom->ime + " (" + to_string(pom->moj_opisnik) + ") -> ";
            if (pom->sljedeci == NULL)
            {
                zadnji_u_listi = pom;
            }
            pom = pom->sljedeci;
        }
        ispis += "OVDJE! :)\n"; // Odnosi se na mjesto u listi, da se vidi.
        cout << boja_zelena << ispis;

        // Dodavanje novoga klijenta na kraj vezane liste.
        zadnji_u_listi->sljedeci = novi;
        novi->bivsi = zadnji_u_listi;

        // Lijepo pakiranje prvog povratnog paketa:
        paket_s_info.od_koga = SERVER_spojen_si;
        paket_s_info.za_koga = novi->moj_opisnik;
        paket_s_info.poruka = "Trenutno aktivnih na serveru: " + to_string(broj_klijenata);
        posalji_klijentu(paket_s_info);
        pthread_mutex_unlock(&monitor_kriticni);
    }
    void posalji_popis_aktivnih(klijent *podnositelj_zahtjeva, poruka_paket paket_s_info)
    {
        pthread_mutex_lock(&monitor_kriticni);

        klijent* pom = this->sljedeci;
        paket_s_info.poruka = "---ime---|--id--\n";
        while (pom)
        {
            paket_s_info.poruka += pom->ime + "  |  " + to_string(pom->moj_opisnik) + "\n";
            pom = pom->sljedeci;
        }

        paket_s_info.od_koga = SERVER_sugovornici;
        paket_s_info.za_koga = podnositelj_zahtjeva->moj_opisnik;
        pthread_mutex_unlock(&monitor_kriticni);

        int poslano = posalji_klijentu(paket_s_info);
        if (poslano == -1)
        {
            makni_klijenta_i_obavijesti_druge(podnositelj_zahtjeva);
        }
    }
    bool pripremi_poruku(klijent *posiljatelj, poruka_paket *paket)
    {
        pthread_mutex_lock(&monitor_kriticni);
        // Pronađi i pošiljatelja i primatelja u listi.
        // Pošiljatelj nam treba za appendanje njegova imena u poruku.
        // Primatelj da provjerimo je li itko prijavljen s odredišnim opisnikom.
        klijent* primatelj = this->sljedeci;

        // Vrti sve dok nije pronađen primatelj ili jedan od njih nije postao NULL.
        while (primatelj)
        {
            if (primatelj->moj_opisnik != paket->za_koga) primatelj = primatelj->sljedeci;
            else break;
        }
        pthread_mutex_unlock(&monitor_kriticni);

        if (!primatelj) return false;       
        return true;
    }
    void makni_klijenta_i_obavijesti_druge(klijent *za_brisanje)
    {
        pthread_mutex_lock(&monitor_kriticni);
        cout << boja_zuta << "Krećem u brisanje " << za_brisanje->moj_opisnik
             << " (" << za_brisanje->ime << ")" << endl;
        
        poruka_paket obavijest;
        obavijest.od_koga = SERVER_oznaka;
        obavijest.poruka =
        "Server: Gud ridans, " + za_brisanje->ime + " (" + to_string(za_brisanje->moj_opisnik) + ")!";

        klijent *pom = this->sljedeci;
        while (pom)
        {   // Obavijesti sve da je ovaj klijent otišao.
            if (pom != za_brisanje)
            {
                obavijest.za_koga = pom->moj_opisnik;
                int poslano = posalji_klijentu(obavijest);
                if (poslano == -1)
                {
                    makni_klijenta_i_obavijesti_druge(pom);
                    // Ako ovaj nije primio poruku, i njega izbriši.
                }
            }
            pom = pom->sljedeci;
        }
        cout << boja_crvena << za_brisanje->bivsi->sljedeci << boja_normal_bold
             << " postaje " << za_brisanje->sljedeci;
        za_brisanje->bivsi->sljedeci = za_brisanje->sljedeci;
        cout << boja_zelena << " = " << za_brisanje->bivsi->sljedeci << endl;
    
        delete za_brisanje;
        cout << boja_zelena << "Obrisan " << za_brisanje->moj_opisnik
             << " (" << za_brisanje->ime << ")" << endl;
        pthread_mutex_unlock(&monitor_kriticni);
    }
    void izbrisi_sve_prijavljene_klijente()
    {
        if (!broj_klijenata) return;
        pthread_mutex_lock(&monitor_kriticni);
        cout << "\n_______________________________\n\n";
        cout << boja_normal_bold << "Krećem u brisanje klijenata. Prvo im javiti da se gasim.\n";
        klijent* pom = this->sljedeci;
        while (pom) // Prvo svima javi da se server ugasio.
        {
            poruka_paket zadnji_paket;
            zadnji_paket.od_koga = SERVER_gasim_se;
            zadnji_paket.za_koga = pom->moj_opisnik;
            zadnji_paket.poruka = "Server: Welp ugasio sam se.";
            posalji_klijentu(zadnji_paket); // Ne provjerava se je li -1 jer je ionako sve gotovo.
            pom = pom->sljedeci;
        }
        cout << boja_normal_bold << "Svima sam javio da se gasim.\n";
        sleep(1); // Malo pričekaj za ACK.
        pom = this->sljedeci;
        while (pom)
        {
            klijent* pom_brisanje = pom;
            pom = pom->sljedeci;
            delete pom_brisanje;
        }
        cout << boja_normal_bold << "Sve sam obrisao.\n";
        //pthread_mutex_unlock(&monitor_kriticni);
    }
} klijent_lista(SERVER_oznaka, SERVER_oznaka, "Server (zaglavlje liste)"); // SERVER_oznaka jest -100 jer to zvuči kul.

int klijent::broj_klijenata = -1; // Budući da se klijent_lista gleda kao "aktivni", ovo ide na -1.

void kraj(int sig);

void povezivanje_socket()
{
	memset(&hints, 0, sizeof(addrinfo));

    hints.ai_family = AF_INET; /* koristi se IPv4 */
    hints.ai_socktype = SOCK_STREAM;

	int povratna = getaddrinfo(IPV4, PORT, &hints, &klijent_info);
	if(povratna != 0) {
		cout << boja_crvena << "Problem pri povezivanju na internet: getaddrinfo(): " << gai_strerror(povratna) << "(" << povratna << ")\n";
		exit(1); // Moguće je da nema internetske veze ako se pokreće lokalno!
	}

	/* kreiranje opisnika priključnice (socket) */
	opisnik = socket(klijent_info->ai_family, klijent_info->ai_socktype, klijent_info->ai_protocol);
	povratna = bind(opisnik, klijent_info->ai_addr, klijent_info->ai_addrlen);
	if(povratna == -1) {
		int brojgreske = errno;
		cout << boja_crvena << "Problem pri spajanju na priključnicu (bind): " << strerror(brojgreske) << " (" << brojgreske << ")";
		freeaddrinfo(klijent_info);
		kraj(-1); // Za sigurno gašenje (3s) sada već dodijeljenog socketa (nije prenužno al eto).
	}

	freeaddrinfo(klijent_info);
	povratna = listen(opisnik, VELICINA_REDA_CEKANJA);
	if(povratna == -1) {
		int brojgreske = errno;
		cout << boja_crvena << "Problem pri korištenju priključnice (listen): " << strerror(brojgreske) << " (" << brojgreske << ")\n";
		kraj(0);
	}
}

struct el_opisnik{
    int opisnik_klijent;
    el_opisnik* sljedeci;
};
typedef el_opisnik* vezana_opisnici;
vezana_opisnici lista_opisnika = NULL; // Lista svih opisnika ikada (očisti se tek na kraju).
el_opisnik *trenutni_opisnik; // Opisnik trenutno aktivne veze (u trenutnom ponavljanju petlje while u int mainu).

struct argument_za_dretvu // Izvrstan način za dati dretvi njen id i njenog opisnika.
{
    pthread_t id_dretve; // Pomoću ovoga id-a možemo "killati" dretvu u destruktoru njenoga klijenta.
    int opisnik_klijenta; // Treba funkciji "recvfrom", ali i konstruktoru klijenta.
} argument;
void stvori_thread_za_komunikaciju();
void *komunikacija_klijent(void *x) // Dretva određenoga primatelja.
{
    argument_za_dretvu dobiveni_arg = *((argument_za_dretvu *)x); // Proslijeđeni argument u dretvu.
    pthread_t id_ove_dretve = dobiveni_arg.id_dretve;
    klijent *moj_klijent = NULL; // Klijent koji komunicira preko ove dretve.
    int moj_opisnik = dobiveni_arg.opisnik_klijenta; // Opisnik klijenta ove dretve.
    cout << boja_normal_bold << "-> pthread_create - kom (" << dobiveni_arg.opisnik_klijenta << ")\n";
    while(true)
    {
        poruka_paket paket;
        primi_od(moj_opisnik, &paket);
        if (paket.od_koga == KLIJENT_prekinut)
        {
            klijent_lista.makni_klijenta_i_obavijesti_druge(moj_klijent);
            break;
        }

        if (paket.od_koga == KLIJENT_spajam_se)
        {
            // Tijekom testiranja trebalo je osigurati da server ne pokuša dvaput kreirati instancu klijenta.
            if (moj_klijent == NULL)
            {
                moj_klijent = new klijent(moj_opisnik, id_ove_dretve, paket.poruka);
                // TODO možda ove argumente dolje u metodu
                klijent_lista.dodaj_klijenta_obavijesti_druge_i_njega(moj_klijent);
                cout << boja_zelena << "Novi klijent dodan!"
                    << "\n------------------------------------\n";
            }
            else
            {
                klijent_lista.makni_klijenta_i_obavijesti_druge(moj_klijent);
                break;
            }
        }
        else if (paket.za_koga == KLIJENT_odlazim)
        {
            klijent_lista.makni_klijenta_i_obavijesti_druge(moj_klijent);
            break;
        }
        else if (paket.za_koga == KLIJENT_posalji_listu)
        {
            klijent_lista.posalji_popis_aktivnih(moj_klijent, paket);
        }
        else // Normalna komunikacija (niti se otvara niti se zatvara veza).
        {    // U uvjetu se pripremi i pošalje poruka ako je sve prošlo kako spada.          
            if (!klijent_lista.pripremi_poruku(moj_klijent, &paket) || posalji_klijentu(paket) == -1)
            {
                paket.za_koga = paket.od_koga; // UNO swap card.
                paket.od_koga = SERVER_sugovornik_nedostupan;
                paket.poruka = "Server: Sugovornik nije dostupan :(";

                int poslano = posalji_klijentu(paket);
                if (poslano == -1) // Pošalji povratnu informaciju natrag.
                {
                    klijent_lista.makni_klijenta_i_obavijesti_druge(moj_klijent);
                    break;
                }
            }
        }
    }
    cout << boja_plava << "-> Dretva (" << moj_opisnik << ") ugašena!\n";
    return x;
}

void stvori_thread_za_komunikaciju(int arg_opisnik_klijenta)
{
    pthread_t nova_dretva;
    argument.id_dretve = nova_dretva;
    argument.opisnik_klijenta = arg_opisnik_klijenta;
    if (pthread_create(&argument.id_dretve, NULL, komunikacija_klijent, &argument))
    {
        cout << boja_crvena << "-> Pogreška pri stvaranju dretve za komunikaciju!\n";
        kraj(SIGINT);
    }
}

int main() {

    system("clear");
    cout << boja_normal_bold << "Server ugasiti sa Ctrl-C.\nNapravio: Mislav Matijević\n";
    povezivanje_socket();
    cout << boja_normal_bold << "Povezan na priključnicu!\n";

    sigset(SIGINT, kraj);
    pthread_mutex_init(&monitor_kriticni, 0);
    pthread_mutex_init(&monitor_ispis, 0);
    int opisnik_novi_klijent;

    cout << "Čekam klijente...\n";

    while (true) // Prihvaćanje novih veza.
    {
        adresa_klijent_velicina = sizeof(adresa_klijent);
        opisnik_novi_klijent = accept(opisnik, (sockaddr *)&adresa_klijent, &adresa_klijent_velicina);
        cout << boja_zelena << "\n------------------------------------\n"
             << "NOVI PRISTUP: " << opisnik_novi_klijent
             << "\n------------------------------------\n";
        stvori_thread_za_komunikaciju(opisnik_novi_klijent); // Uspostava komunikacije s novom vezom.
    }
}

void kraj(int sig)
{
    switch (sig)
    {
        case SIGINT: // Server je ugasio korisnik.
        {
            klijent_lista.izbrisi_sve_prijavljene_klijente();
        }
        case 0: // Server je ugasila pogreška nakon uspostavljanja veze s priključnicom.
        {
            pthread_mutex_destroy(&monitor_kriticni);
            pthread_mutex_destroy(&monitor_ispis);
            cout << boja_normal << "\n________________________________________________________\n" << flush;
            cout << boja_normal_bold << "Pričekajte molim, da se ne aktivira timeout za tcp vezu!\n";
            usleep(500000);
            cout << boja_crvena << "\\" << flush;
            // Iako najčešće ne pomaže, pričekajmo posljednji ACK ako je negdje zapeo.
            // Ovo je naravno u svrhu izbjegavanja "TIME_WAIT" stanja na TCP socketu.
            // A i napravio sam da čekanje prekul izgleda. B)
            sleep(1);
            cout << boja_zuta << "\r|" << flush;
            sleep(1);
            cout << boja_zelena << "\r/" << flush;
            usleep(500000);
            cout << boja_zelena << "\nHave a nice day.\n";
            break;
        }
        case -1: // Server je ugašen prije 
        {
            cout << boja_zuta
            << "\n________________________________________________________________\n"
            << "Već postoji aktivni server ili je prebrzo upaljen nakon gašenja!\n";
        }
    }

    close(opisnik);

	cout << boja_normal;
    exit(0);
}