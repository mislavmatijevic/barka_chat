#include "chat_header.h"
#include <ncurses.h>

string ime;

poruka_paket moj_paket;

sem_t sem_dolazak_prvog_paketa;
sem_t sem_odabir_sugovornika;
pthread_mutex_t monitor_ispis;
sem_t sem_kraj;

int broj_bajtova; // Zadnji primljeni broj bajtova.

void kraj(int sig);
void promjena_velicine_terminala(int sig);

pthread_t citaj_thr;
void *dretva_citaj(void *x);

// Da nista (uglavnom iz dretve) ne moze omesti vec zapoceto gasenje.
bool klijent_se_gasi = false;

// Signali za gasenje programa
#define SIGNAL_RAZINA_DRETVA 0 // program je (neocekivano) ugasen nakon kreiranja dretve
#define SIGNAL_RAZINA_VEZA -1 // program je ugasen nakon uspostave veze, prije dretve
#define SIGNAL_RAZINA_TERMINAL -2 // program je ugasen zbog neuspostave veze
#define SIGNAL_RAZINA_POCETAK -3 // program je ugasen zbog neispravnog terminala (premalen / nema boja)

class csucelje
{
    WINDOW *prozor_chat_rub,
			*prozor_chat_sadrzaj,
			*prozor_poruka_rub,
			*prozor_poruka,
			*prozor_info,
			*prozor_login = NULL;

    short prozor_chat_y;
    short prozor_chat_x;
    short y_koord_chat; 
	short najveca_poruka; // Najveca poruka određena sirinom prozora.

	#define BOJA_PROZOR_CHAT_INFO 1
	#define BOJA_PROZOR_PORUKA 2
	#define BOJA_CRVENA 3
	#define BOJA_SUGOVORNIK_PORUKE 4
	#define BOJA_BIJELA 5

    const char *poruka_unos = "Unesi poruku: ";
	const char *poruka_info = "Mislav Matijevic, 10/2020.\n(ali pravimo se da je 10/1980.)\n"
	"\n------------------\n"
	"Komande za server:\n\n"
	"?s - popis aktivnih\n"
	"!u - odjava\n"
	"cls - ocisti chat\n"
	"\n------------------\n\n";

	void obnovi_prozor_chat(bool zbog_promjene_velicine)
	{
		if (zbog_promjene_velicine)
		{
			wattron(prozor_chat_rub, COLOR_PAIR(BOJA_PROZOR_CHAT_INFO));
			box(prozor_chat_rub, 0 , 0);
			wattron(prozor_chat_rub, COLOR_PAIR(BOJA_SUGOVORNIK_PORUKE));
			mvwaddstr(prozor_chat_rub, 0, 4, "Pristigle poruke");
			wattron(prozor_chat_rub, COLOR_PAIR(BOJA_PROZOR_CHAT_INFO));
			mvwaddstr(prozor_chat_rub, 0, prozor_chat_x/2-5, "Obavijesti");
			wattron(prozor_chat_rub, COLOR_PAIR(BOJA_PROZOR_PORUKA));
			mvwaddstr(prozor_chat_rub, 0, prozor_chat_x-14, "Tvoje poruke");
			wrefresh(prozor_chat_rub);
		}
		wrefresh(prozor_chat_sadrzaj);
	}
	void obnovi_prozor_info(bool zbog_promjene_velicine)
	{
		wattron(prozor_info, COLOR_PAIR(BOJA_PROZOR_CHAT_INFO));
		mvwaddstr(prozor_info, prozor_chat_y/2, 1, "Tvoje ime: ");
		waddstr(prozor_info, ime.c_str());
		mvwaddstr(prozor_info, prozor_chat_y/2+1, 1, "Tvoj ID: ");
		waddstr(prozor_info, to_string(moj_paket.od_koga).c_str());
        if (moj_paket.za_koga > 0)
		{
			mvwaddstr(prozor_info, prozor_chat_y/2+2, 0, "ID sugovornika: ");
			waddstr(prozor_info, to_string(moj_paket.za_koga).c_str());
		}
		mvwaddstr(prozor_info, prozor_chat_y-5, 0, "Primljeno: ");
		waddstr(prozor_info, to_string(broj_bajtova).c_str());
		waddstr(prozor_info, "B");
        wrefresh(prozor_info);
	}
	void obnovi_prozor_poruke()
	{
		wclear(prozor_poruka_rub);
		wresize(prozor_poruka_rub, 3, prozor_chat_x);
		mvwin(prozor_poruka_rub, prozor_chat_y+3, 2);
		wattron(prozor_poruka_rub, COLOR_PAIR(BOJA_PROZOR_PORUKA));
		box(prozor_poruka_rub, 0, 0);
		mvwaddstr(prozor_poruka_rub, 0, 1, poruka_unos);
		//
		int lokacija_kursora_na_poruci = getcurx(prozor_poruka);
		wattron(prozor_poruka, COLOR_PAIR(BOJA_PROZOR_PORUKA));
		wattron(prozor_poruka, COLOR_PAIR(BOJA_PROZOR_PORUKA));
		wresize(prozor_poruka, 1, prozor_chat_x-3);
		mvwin(prozor_poruka, prozor_chat_y+4, 4);
		wmove(prozor_poruka, 0, lokacija_kursora_na_poruci);
		wrefresh(prozor_poruka_rub);
		wrefresh(prozor_poruka);
	}
    public:
    csucelje()
    {
        initscr();
        cbreak();
        if (!has_colors())
        {
            printw("Terminal ne podrzava boje! Pokusajte na drugome racunalu!");
			getch();
			kraj(SIGNAL_RAZINA_POCETAK);
			return;
        }

        start_color();
        init_pair(BOJA_PROZOR_CHAT_INFO, COLOR_YELLOW, COLOR_BLACK);
        init_pair(BOJA_PROZOR_PORUKA, COLOR_GREEN, COLOR_BLACK);
        init_pair(BOJA_CRVENA, COLOR_RED, COLOR_BLACK);
		init_pair(BOJA_SUGOVORNIK_PORUKE, COLOR_CYAN, COLOR_BLACK);
		init_pair(BOJA_BIJELA, COLOR_WHITE, COLOR_BLACK);

		refresh();

		pthread_mutex_init(&monitor_ispis, 0);
		sem_init(&sem_dolazak_prvog_paketa, 0, 0);
		sem_init(&sem_odabir_sugovornika, 0, 0);

		attron(COLOR_PAIR(BOJA_BIJELA));
        if (LINES < 30 || COLS < 95) 
		{
			printw("Trenutna velicina prozora jest %ix%i, a preporucena je 30x95! ENTER za nastavak... ", LINES, COLS);
			getch();
			clear();
			refresh();
		}

		sigset(SIGWINCH, promjena_velicine_terminala);		
		
		curs_set(0);
        prozor_login = newwin(0,0,0,0);
		nove_velicine_prozora();

        char c_ime [30];
		bool ispravno_ime;
		do {
			ispravno_ime = true;
			
			mvwgetnstr(prozor_login, 1, strlen("Unesi ime: ")+1, c_ime, 30); // Refresha prozor.

			if (!strlen(c_ime)) ispravno_ime = false;
			for (int i = 0; ispravno_ime && i < strlen(c_ime); i++)
			{
				if ((c_ime[i] < 'A' || c_ime[i] > 'Z') && (c_ime[i] < 'a' || c_ime[i] > 'z'))
					ispravno_ime = false;
			}

			if (!ispravno_ime)
				for (int i = strlen("Unesi ime: ")+1; i < COLS/2; i++)
					mvwaddch(prozor_login, 1, i, ' ');
		
		} while(!ispravno_ime);
		ime = c_ime;
		curs_set(1);

        // Brisanje login prozora.
        wborder(prozor_login, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '); // Izbrisi okvir oko login prozora.
        wmove(prozor_login, 0, 0);
        wclrtobot(prozor_login);
        wrefresh(prozor_login); // Osvjezi ga (pa nestane).
        delwin(prozor_login); // Izbrisi ga.
		prozor_login = NULL; // Zbog resize metode, da ne bude seg err.

		sigset(SIGINT, kraj);

		y_koord_chat = 0;

        prozor_chat_y = LINES/2+LINES/3-2;
        prozor_chat_x = COLS*65/100;

		prozor_chat_rub = newwin(0,0,0,0);
        prozor_chat_sadrzaj = newwin(0,0,0,0);
        prozor_poruka_rub = newwin(0,0,0,0);
        prozor_poruka = newwin(0,0,0,0);
        prozor_info = newwin(0,0,0,0);

		nove_velicine_prozora();
    }
	void nove_velicine_prozora()
	{
		pthread_mutex_lock(&monitor_ispis);
		if (prozor_login)
		{
			wborder(prozor_login, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '); // Izbrisi okvir oko login prozora.
			wresize(prozor_login, 3, COLS/2+1);
			mvwin(prozor_login, LINES/2-LINES/3, COLS/4);
			wattron(prozor_login, COLOR_PAIR(BOJA_PROZOR_CHAT_INFO));
			box(prozor_login, 0, 0);

			wattron(prozor_login, COLOR_PAIR(BOJA_PROZOR_PORUKA));
       		mvwprintw(prozor_login, 1, 1, "Unesi ime: ");
			wrefresh(prozor_login);
			pthread_mutex_unlock(&monitor_ispis);
		}
		else // Korisnik je već unio ime (zatvorila se forma za unos imena).
		{
			prozor_chat_y = LINES/2+LINES/3-2;
			prozor_chat_x = COLS*65/100;

			wresize(prozor_chat_rub, prozor_chat_y+2, prozor_chat_x+2);
			mvwin(prozor_chat_rub, 1, 1);
			//
			wresize(prozor_chat_sadrzaj, prozor_chat_y, prozor_chat_x);
			mvwin(prozor_chat_sadrzaj, 2, 2);

			obnovi_prozor_poruke();

			wborder(prozor_info, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
			wresize(prozor_info, prozor_chat_y, COLS-prozor_chat_x-4);
			mvwin(prozor_info, 3, prozor_chat_x+4);
			//
			wattron(prozor_info, COLOR_PAIR(BOJA_PROZOR_CHAT_INFO));
			mvwaddstr(prozor_info, 0, 0, poruka_info);
			wclrtobot(prozor_info);
			pthread_mutex_unlock(&monitor_ispis);
			obnovi_prozore_chat_i_info(1);
		}		
	}
	void obnovi_prozore_chat_i_info(bool zbog_promjene_velicine)
	{
		pthread_mutex_lock(&monitor_ispis);
		int lokacija_kursora_na_poruci = getcurx(prozor_poruka);
		obnovi_prozor_chat(zbog_promjene_velicine);
		obnovi_prozor_info(zbog_promjene_velicine);
		wattron(prozor_poruka, COLOR_PAIR(BOJA_PROZOR_PORUKA));
		wmove(prozor_poruka, 0, lokacija_kursora_na_poruci);
		wrefresh(prozor_poruka);
		pthread_mutex_unlock(&monitor_ispis);
	}
	void povezivanje()
	{
		memset(&hints, 0, sizeof(addrinfo));

		hints.ai_family = AF_INET; // koristi se IPv4
		hints.ai_socktype = SOCK_STREAM;

		getaddrinfo(IPV4, PORT, &hints, &server_info); // kreiranje prikljucnice (socketa)
		opisnik = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
		if (opisnik == -1)
		{
			chat_ispis('l', "Opisnik nije stvoren!", 1);
			chat_ispis('l', "Provjeriti konekciju s internetom!", 1);
			kraj(SIGNAL_RAZINA_TERMINAL);
			return;
		}
		if (connect(opisnik, server_info->ai_addr, server_info->ai_addrlen) == -1)
		{
			chat_ispis('s', "Ne mogu se povezati na server!", 1);
			chat_ispis('s', "On mora biti pokrenut prije klijenta!", 1);
			kraj(SIGNAL_RAZINA_VEZA);
			return;
		}

		// Stvaranje dretve koja iskljucivo cita dolazece pakete.
		if (pthread_create(&citaj_thr, NULL, dretva_citaj, 0))
		{
			chat_ispis('l', "Pogreska pri stvaranju dretve za citanje!", 1);
			kraj(SIGNAL_RAZINA_VEZA);
			return;
		}

		moj_paket.od_koga = KLIJENT_spajam_se;
		moj_paket.za_koga = SERVER_oznaka; // Potpuno visak, ali na serveru se lijepo vidi da je njemu namijenjeno.
		moj_paket.poruka = ime;

		int rezultat = posalji_paket(opisnik, &moj_paket);
		if (rezultat == 0)
		{
			return;
		}
		if (rezultat == -1)
		{
			chat_ispis('s', "Server ne odgovara na prvi paket!", 1);
			kraj(SIGNAL_RAZINA_DRETVA);
			return;
		}
		sem_wait(&sem_dolazak_prvog_paketa); // Pricekaj prvu poruku prije nastavka.

		if (!klijent_se_gasi) chat_ispis('s', "Unesi ?s za popis aktivnih klijenata ili pricekaj da ti se netko javi!", 0);
	}
	void odabir_sugovornika(string lista_sugovornika)
	{

		char *osobe = new char [MAX_VELICINA_PORUKE];
		for (int i = 0, j = 0; i < lista_sugovornika.length() && !klijent_se_gasi; i++, j++)
		{
			if (lista_sugovornika[i] == '\n')
			{
				chat_ispis('s', osobe, 0);
				j = -1;
				memset(osobe, 0, 500);
			}
			else osobe[j] = lista_sugovornika[i];
		}
		delete osobe;

		if (klijent_se_gasi) return;
		const char *poruka_novi_sugovornik = "Odaberi novog sugovornika (id broj): ";
        char *broj_sugovornika = new char [prozor_chat_x-strlen(poruka_novi_sugovornik)-2];

		bool exception;
		do {
			exception = 0;
			wclear(prozor_poruka_rub);

        	wattron(prozor_poruka_rub, COLOR_PAIR(BOJA_BIJELA));
			wborder(prozor_poruka_rub, '|', '|', '-','-','+','+','+','+');
			mvwaddstr(prozor_poruka_rub, 0, 1, poruka_novi_sugovornik);
			wrefresh(prozor_poruka_rub);
			wmove(prozor_poruka, 0, 0);

        	wgetnstr(prozor_poruka, broj_sugovornika, 9); // 9 jer toliko znamenki ima integer.
			try
			{
				moj_paket.za_koga = stoi(broj_sugovornika);
			}
			catch (std::invalid_argument& e)
			{
				poruka_novi_sugovornik = "Ponovi unos! Unesi samo id broj: ";
				exception = 1;
			}
		} while (exception && moj_paket.za_koga < 0);
		wclear(prozor_poruka);
		obnovi_prozor_poruke();
		obnovi_prozore_chat_i_info(0);
	}
	void ocisti_chat()
	{
		wmove(prozor_chat_sadrzaj, 0, 0);
		wclrtobot(prozor_chat_sadrzaj);
		wrefresh(prozor_chat_sadrzaj);
		y_koord_chat = 0;
	}
    void chat_ispis(char poravnanje, string poruka, bool poseban_stil)
    {
		pthread_mutex_lock(&monitor_ispis);

		wmove(prozor_chat_sadrzaj, y_koord_chat, 0);
		wclrtoeol(prozor_chat_sadrzaj);

        int x_koord;
        switch (poravnanje)
        {
            case 'l':
			{
				wattron(prozor_chat_sadrzaj, COLOR_PAIR(BOJA_SUGOVORNIK_PORUKE));
                x_koord = 3;
                break;
			}
            case 's':
			{
				if (poseban_stil) wattron(prozor_chat_sadrzaj, COLOR_PAIR(BOJA_CRVENA));
				else wattron(prozor_chat_sadrzaj, COLOR_PAIR(BOJA_PROZOR_CHAT_INFO));
                x_koord = prozor_chat_x/2 - poruka.length()/2;
                break;
			}
            case 'd':
			{
				wattron(prozor_chat_sadrzaj, COLOR_PAIR(BOJA_PROZOR_PORUKA));
				poruka = "Ja: " + poruka;
                x_koord = prozor_chat_x - poruka.length() - 3;
                break;
			}
        }
		if (x_koord < 1) x_koord = 1;

        mvwaddstr(prozor_chat_sadrzaj, y_koord_chat, x_koord, poruka.c_str());

		wattron(prozor_chat_sadrzaj, COLOR_PAIR(BOJA_PROZOR_CHAT_INFO));
		
        if (y_koord_chat >= prozor_chat_y-2)
		{
			y_koord_chat = 0;
		}
		else
		{
        	waddstr(prozor_chat_sadrzaj, "\n");
			for (int i = 0; i < prozor_chat_x; i++)
				waddstr(prozor_chat_sadrzaj, "-");
			y_koord_chat = getcury(prozor_chat_sadrzaj)-1;
		}

		pthread_mutex_unlock(&monitor_ispis);

        obnovi_prozore_chat_i_info(0); // Glavna obnova!
		
    }
    void unos_i_slanje_poruke()
    {
		najveca_poruka = prozor_chat_x - 5;
		// Moze se poslati onolika poruka koliko je velik prozor za pisanje poruke.

        char *c_poruka = new char [najveca_poruka];
        wgetnstr(prozor_poruka, c_poruka, najveca_poruka);
		wclear(prozor_poruka);

		if (klijent_se_gasi) return;

		if (!strcmp(c_poruka, "!u"))
		{
			kraj(SIGINT);
			return;
		}

		if (!strlen(c_poruka)) return;

		if (!strcmp(c_poruka, "cls"))
		{
			ocisti_chat();
			return;
		}

		moj_paket.poruka = ime + ": " + c_poruka;
		if (moj_paket.za_koga == KLIJENT_nema_sugovornika && (c_poruka[0] != '?' || c_poruka[1] != 's'))
		{
			chat_ispis('s', "Odaberi sugovornika unosom '?s' ili cekaj da ti se netko javi.", 0);
			return;
		}
		else if (!strcmp(c_poruka, "?s"))
		{
			moj_paket.za_koga = KLIJENT_posalji_listu;
		}

		
		int poslano_bajtova = posalji_paket(opisnik, &moj_paket);
		if (poslano_bajtova == -1 || !poslano_bajtova)
		{
			chat_ispis('s', "Server ne odgovara na poslanu poruku!", 1);
			kraj(SIGNAL_RAZINA_DRETVA);
			return;
		}

		if (!strcmp(c_poruka, "?s")) sem_wait(&sem_odabir_sugovornika);
		else chat_ispis('d', c_poruka, 0);
    }
	void ENTER_za_kraj()
	{
		wattron(prozor_poruka, COLOR_PAIR(BOJA_PROZOR_CHAT_INFO));
        mvwaddstr(prozor_poruka, 0, 0, "Pritisni ENTER za kraj... ");
		wgetch(prozor_poruka);
	}
} sucelje;

int klijent_primi_paket(poruka_paket *paket_za_pohranu)
{
	poruka_paket_slanje_info paket_s_info;
	int rezultat_1 = recv(opisnik, &paket_s_info, sizeof(poruka_paket_slanje_info), 0);
	if (!rezultat_1 || rezultat_1 == -1) return -1;

	char *primljena_poruka = new char [paket_s_info.velicina_poruke];
	memset(primljena_poruka, 0, paket_s_info.velicina_poruke); // Da se izbjegne razno smece.

	int rezultat_2 = recv(opisnik, primljena_poruka, paket_s_info.velicina_poruke, 0);
	if (!rezultat_2 || rezultat_2 == -1) return -1;

	paket_za_pohranu->od_koga = paket_s_info.od_koga;
	paket_za_pohranu->za_koga = paket_s_info.za_koga;
	paket_za_pohranu->poruka = primljena_poruka;
	
	return rezultat_1 + rezultat_2;
}

void *dretva_citaj(void *x)
{
	while (true)
	{
		poruka_paket paket_primljeni;
		broj_bajtova = klijent_primi_paket(&paket_primljeni);

		if (klijent_se_gasi) return x;

		if 	(broj_bajtova == -1 || !broj_bajtova)
		{
			sucelje.chat_ispis('s', "Veza sa serverom naglo izgubljena!", 1);
			kraj(SIGNAL_RAZINA_DRETVA);
			return x;
		}

		switch (paket_primljeni.od_koga)
		{
			case SERVER_spojen_si: // Server obznanio dolazak i poslao broj aktivnih.
			{
				moj_paket.od_koga = paket_primljeni.za_koga;
				moj_paket.za_koga = KLIJENT_nema_sugovornika;
				moj_paket.poruka = "";
				sem_post(&sem_dolazak_prvog_paketa);
				break;
			}
			case SERVER_sugovornici: // Novi sugovornici.
			{
				sucelje.odabir_sugovornika(paket_primljeni.poruka);
				sem_post(&sem_odabir_sugovornika);
				break;
			}
			case SERVER_gasim_se:
			{
				sucelje.chat_ispis('s', "Server je ugasen. So long.", 1);
				kraj(SIGNAL_RAZINA_DRETVA);
				return x;
			}
			case SERVER_sugovornik_nedostupan:
			{
				moj_paket.za_koga = KLIJENT_nema_sugovornika;
				// Nema break jer je serverska poruka u pitanju.
			}
			case SERVER_oznaka:
			{
				sucelje.chat_ispis('s', paket_primljeni.poruka, 0);
				break;
			}
			default:
			{
				if (moj_paket.za_koga == KLIJENT_nema_sugovornika)
				{
					moj_paket.za_koga = paket_primljeni.od_koga;
					string poruka_o_novome = "Spojen na novog sugovornika (" 
										+ to_string(moj_paket.za_koga) + ")!";
					sucelje.chat_ispis('s', poruka_o_novome.c_str(), 0);
				}
				// Na kraju ispis dobivene poruke.
				sucelje.chat_ispis('l', paket_primljeni.poruka, 0);
			}
		}
	}
}

int main()
{ // Pokrece se nakon konstruktora klase!
	sem_init(&sem_kraj, 0, 0);
	sucelje.povezivanje();
	while (!klijent_se_gasi) sucelje.unos_i_slanje_poruke();
	// Main stalno radio probleme, neka sada završavanje.
	sem_wait(&sem_kraj);
}

void promjena_velicine_terminala(int sig)
{
	pthread_mutex_lock(&monitor_ispis);
	do {
		endwin();
		refresh();
		clear();
		if (LINES < 30 || COLS < 95)
		{
			printw("%ix%i Prozor je premalen. Povecaj ga na najmanje 30x95!", LINES, COLS);
			usleep(50000);
		}
	}while ((LINES < 30 || COLS < 95) && !klijent_se_gasi);

	pthread_mutex_unlock(&monitor_ispis);
	sucelje.nove_velicine_prozora(); // Ovo se neće pokrenuti prije login prozora.
}
void kraj(int sig)
{
	if (klijent_se_gasi) return;
	klijent_se_gasi = true;
	pthread_mutex_unlock(&monitor_ispis); // U slučaju da je ispis stao na pola.
	switch (sig)
	{
		case SIGINT: // Uredno javljam serveru da klijent odlazi.
		{
			moj_paket.od_koga = KLIJENT_odlazim;
			moj_paket.za_koga = KLIJENT_odlazim;
			moj_paket.poruka = "Odjava!";
			posalji_paket(opisnik, &moj_paket); // Ne treba provjeravati vracenu vrijednost.
		}
		case SIGNAL_RAZINA_DRETVA: // Nema vise veze sa serverom - treba (od)spojiti dretvu.
		{
			pthread_join(citaj_thr, 0);
		}
		case SIGNAL_RAZINA_VEZA: // Dretva se nije stigla ni napraviti, a nesto je vec poslo po zlu.
		{
			close(opisnik);
		}
		case SIGNAL_RAZINA_TERMINAL: // Nisam se uspio spojiti na socket.
		{
			sucelje.ENTER_za_kraj();
		}
		case SIGNAL_RAZINA_POCETAK: // Tek se uslo u konstruktor klase, vec sam ja pozvan.
		{
			sem_destroy(&sem_odabir_sugovornika);
			sem_destroy(&sem_dolazak_prvog_paketa);
			pthread_mutex_destroy(&monitor_ispis);
		}
	}
	endwin();
	exit(0);
}