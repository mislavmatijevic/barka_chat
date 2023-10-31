#include <locale>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <pthread.h>
#include <unistd.h> // Za close (i sleep) na kraju i za fork-ove.
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> // Ovo je bitno
#include <semaphore.h>

using namespace std;

#define IPV4 "127.0.0.1"
#define PORT "3699"
#define VELICINA_REDA_CEKANJA 100

// KLIJENT STUFF:
addrinfo hints; /* struktura za parametriziranje getaddrinfo poziva (u engl. literaturi obicno 'hints') */
addrinfo *server_info; /* struktura koja ce sadrzavati popunjene informacije o loopback adresi servera */
int opisnik; /* socket descriptor klijenta */

// SERVER STUFF:
addrinfo *klijent_info; /* struktura koja ce sadrzavati popunjene informacije o loopback adresi servera */
sockaddr_storage adresa_klijent; /* struktura koja ce sadrzavati informacije o povezanom klijentu */
socklen_t adresa_klijent_velicina; /* velicina strukture sockaddr_storage koja se popunjava pozivom accept() */

#define MAX_VELICINA_PORUKE 4000
struct poruka_paket { // Ovdje se poruka priprema.
    int od_koga;
    int za_koga;
    string poruka;
};

// Ovo je prvi dio poruke. Drugi je tekstualni oblik poruke veličine "velicina_poruke".
struct poruka_paket_slanje_info {
    short od_koga;
    short za_koga;
    short velicina_poruke;
};

// Ispod su kodovi koji se ne interpretiraju kao pošiljatelj/primatelj kada stigne poruka.
// Uglavnom svi idu u "od_koga" (osim klijentovih nakon spajanja).

// klijent - početak veze (poruka je ime)
#define KLIJENT_spajam_se -10

// klijent - sugovornik nije odabran (ova poruka nikada ne dođe do servera)
#define KLIJENT_nema_sugovornika -11

// klijent - klijent traži listu aktivnih klijenata od servera
#define KLIJENT_posalji_listu -12

// klijent - klijent nagovještava svoj odlazak
#define KLIJENT_odlazim -13

// server - klijent ne odgovara
#define KLIJENT_prekinut -135

// server - prvi paket novome klijentu (sadrži njegov opisnik u "za_koga")
#define SERVER_spojen_si -14

// server - obavijesti klijenta da je odjavljen
#define SERVER_gasim_se -15

// server - biraj novog sugovornika (poruka je lista aktivnih)
#define SERVER_sugovornici -16

// server - sugovornik prekinuo vezu (poruka je lista aktivnih)
#define SERVER_sugovornik_nedostupan -17

// server - serverova oznaka
#define SERVER_oznaka -100

int posalji_paket(int arg_opisnik, poruka_paket *paket_za_slanje)
{
	if (paket_za_slanje->poruka == "")
    {
        return 0;
    }
    if (paket_za_slanje->poruka.length() >= MAX_VELICINA_PORUKE-1)
    {
        return 0;
    }

    poruka_paket_slanje_info odlazni_paket;
    odlazni_paket.od_koga = paket_za_slanje->od_koga;
    odlazni_paket.za_koga = paket_za_slanje->za_koga;
    odlazni_paket.velicina_poruke = strlen(paket_za_slanje->poruka.c_str())+1; // +1 za \0

    int poslano_bajtova_1 = send(arg_opisnik, &odlazni_paket, sizeof(poruka_paket_slanje_info), MSG_NOSIGNAL);
    if (poslano_bajtova_1 <= 0) return poslano_bajtova_1;

    char *odlazna_poruka = new char [odlazni_paket.velicina_poruke];
	memset(odlazna_poruka, 0, odlazni_paket.velicina_poruke); // Da se izbjegne razno smeće.
    strcpy(odlazna_poruka, paket_za_slanje->poruka.c_str());

    int poslano_bajtova_2 = send(arg_opisnik, odlazna_poruka, odlazni_paket.velicina_poruke, MSG_NOSIGNAL);
    if (poslano_bajtova_2 <= 0) return poslano_bajtova_2;

	return poslano_bajtova_1 + poslano_bajtova_2;
}
